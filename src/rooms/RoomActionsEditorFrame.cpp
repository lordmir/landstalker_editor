#include <rooms/RoomActionsEditorFrame.h>

#include <wx/listctrl.h>
#include <wx/button.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/wrapsizer.h>
#include <wx/msgdlg.h>

#include <landstalker/main/ScriptData.h>
#include <landstalker/script/RoomActionTable.h>
#include <rooms/RoomActionAddDialog.h>
#include <rooms/RoomActionDisplay.h>
#include <script/CutsceneCodeEditor.h>

namespace
{
	enum { ID_ADD = wxID_HIGHEST + 1, ID_REMOVE };

	std::shared_ptr<Landstalker::RoomActionTable> GetTable(const std::shared_ptr<Landstalker::GameData>& gd)
	{
		if (!gd || !gd->GetScriptData())
		{
			return nullptr;
		}
		auto t = gd->GetScriptData()->GetRoomActions();
		return (t && t->IsValid()) ? t : nullptr;
	}
}

RoomActionsEditorFrame::RoomActionsEditorFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst)
{
	m_mgr.SetManagedWindow(this);
	BuildUI();
	m_mgr.Update();
}

RoomActionsEditorFrame::~RoomActionsEditorFrame()
{
	m_mgr.UnInit();
}

void RoomActionsEditorFrame::BuildUI()
{
	auto* splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxSP_LIVE_UPDATE | wxSP_3D);
	splitter->SetMinimumPaneSize(240);

	auto* left = new wxPanel(splitter, wxID_ANY);
	auto* left_sizer = new wxBoxSizer(wxVERTICAL);
	m_list = new wxListCtrl(left, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
	m_list->InsertColumn(0, "Action", wxLIST_FORMAT_LEFT, 130);
	m_list->InsertColumn(1, "Type", wxLIST_FORMAT_LEFT, 60);
	m_list->InsertColumn(2, "Key", wxLIST_FORMAT_LEFT, 200);
	left_sizer->Add(m_list, 1, wxEXPAND);

	auto* btns = new wxWrapSizer(wxHORIZONTAL);
	m_add = new wxButton(left, ID_ADD, "Add");
	m_remove = new wxButton(left, ID_REMOVE, "Remove");
	btns->Add(m_add, 0, wxALL, 2);
	btns->Add(m_remove, 0, wxALL, 2);
	left_sizer->Add(btns, 0, wxEXPAND);
	left->SetSizer(left_sizer);

	auto* right = new wxPanel(splitter, wxID_ANY);
	auto* right_sizer = new wxBoxSizer(wxVERTICAL);
	m_code = new CutsceneCodeEditor(right, wxID_ANY);
	right_sizer->Add(m_code, 1, wxEXPAND);
	right->SetSizer(right_sizer);

	splitter->SplitVertically(left, right, 560);
	m_mgr.AddPane(splitter, wxAuiPaneInfo().CenterPane());

	m_list->Bind(wxEVT_LIST_ITEM_SELECTED, &RoomActionsEditorFrame::OnRowSelected, this);
	m_add->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnAdd(); });
	m_remove->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnRemove(); });
}

bool RoomActionsEditorFrame::Open(const std::string& select_label)
{
	if (!GetTable(m_gd))
	{
		return false;
	}
	RefreshList();
	long row = 0;
	if (!select_label.empty())
	{
		auto table = GetTable(m_gd);
		const auto& branches = table->GetBranches();
		for (std::size_t i = 0; i < branches.size(); ++i)
		{
			if (branches[i].label == select_label) { row = static_cast<long>(i); break; }
		}
	}
	if (row < m_list->GetItemCount())
	{
		m_list->SetItemState(row, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED,
			wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
		m_list->EnsureVisible(row);
	}
	return true;
}

void RoomActionsEditorFrame::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	m_gd = gd;
	if (m_code)
	{
		m_code->SetGameData(gd);
	}
	RefreshList();
}

void RoomActionsEditorFrame::ClearGameData()
{
	CommitCurrentEdit();
	m_gd.reset();
	m_current_row = -1;
	m_current_label.clear();
	if (m_list) m_list->DeleteAllItems();
	if (m_code) { m_code->SetGameData(nullptr); m_code->ClearAsm(); }
	UpdateButtons();
}

void RoomActionsEditorFrame::RefreshRow(long row)
{
	auto table = GetTable(m_gd);
	if (!table || row < 0 || row >= static_cast<long>(table->GetBranches().size())) return;
	const auto& b = table->GetBranches()[static_cast<std::size_t>(row)];
	m_list->SetItem(row, 0, RoomActionDisplay::ActionName(static_cast<int>(row)));
	m_list->SetItem(row, 1, RoomActionDisplay::KeyType(b));
	m_list->SetItem(row, 2, RoomActionDisplay::KeyName(b, m_gd));
}

void RoomActionsEditorFrame::RefreshList()
{
	if (!m_list) return;
	m_current_row = -1;
	m_current_label.clear();
	m_list->DeleteAllItems();
	if (m_code) m_code->ClearAsm();
	auto table = GetTable(m_gd);
	if (!table) { UpdateButtons(); return; }
	for (std::size_t i = 0; i < table->GetBranches().size(); ++i)
	{
		if (table->GetBranches()[i].is_boilerplate) continue; // the nop sentinel is not a user action
		m_list->InsertItem(static_cast<long>(i), wxEmptyString);
		RefreshRow(static_cast<long>(i));
	}
	UpdateButtons();
}

void RoomActionsEditorFrame::ShowRow(long row)
{
	auto table = GetTable(m_gd);
	if (!table || row < 0 || row >= static_cast<long>(table->GetBranches().size())) return;
	const auto& b = table->GetBranches()[static_cast<std::size_t>(row)];
	m_current_row = row;
	m_current_label = b.label;
	m_code->LoadAsm(b.body, true);
}

void RoomActionsEditorFrame::CommitCurrentEdit()
{
	if (m_current_label.empty() || !m_code || !m_code->IsDirty()) return;
	auto table = GetTable(m_gd);
	if (table) table->SetBody(m_current_label, m_code->GetAsm());
	m_code->MarkClean();
}

void RoomActionsEditorFrame::CommitPendingEdits()
{
	CommitCurrentEdit();
}

long RoomActionsEditorFrame::SelectedRow() const
{
	return m_list ? m_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED) : -1;
}

void RoomActionsEditorFrame::UpdateButtons()
{
	auto table = GetTable(m_gd);
	const long sel = SelectedRow();
	bool removable = false;
	if (table && sel >= 0 && sel < static_cast<long>(table->GetBranches().size()))
	{
		removable = table->IsRemovable(table->GetBranches()[static_cast<std::size_t>(sel)].label);
	}
	if (m_add) m_add->Enable(table != nullptr);
	if (m_remove) m_remove->Enable(removable);
}

void RoomActionsEditorFrame::OnAdd()
{
	auto table = GetTable(m_gd);
	if (!table) return;
	RoomActionAddDialog dlg(this, m_gd);
	if (dlg.ShowModal() != wxID_OK) return;
	CommitCurrentEdit();
	const auto label = dlg.IsBgm() ? table->AddBgmBranch(dlg.GetValue()) : table->AddRoomBranch(dlg.GetValue());
	if (!label)
	{
		wxMessageBox("Could not add the room action.", "Add Room Action", wxOK | wxICON_ERROR, this);
		return;
	}
	RefreshList();
	const auto& branches = table->GetBranches();
	for (std::size_t i = 0; i < branches.size(); ++i)
	{
		if (branches[i].label == *label)
		{
			m_list->SetItemState(static_cast<long>(i), wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED,
				wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
			m_list->EnsureVisible(static_cast<long>(i));
			break;
		}
	}
}

void RoomActionsEditorFrame::OnRemove()
{
	auto table = GetTable(m_gd);
	const long sel = SelectedRow();
	if (!table || sel < 0 || sel >= static_cast<long>(table->GetBranches().size())) return;
	const std::string label = table->GetBranches()[static_cast<std::size_t>(sel)].label;
	if (wxMessageBox("Remove this room action?\n\nThe preceding branch will skip straight past it.",
		"Remove Room Action", wxYES_NO | wxICON_QUESTION, this) != wxYES)
	{
		return;
	}
	CommitCurrentEdit();
	if (!table->RemoveBranch(label))
	{
		wxMessageBox("This branch cannot be removed automatically.", "Remove Room Action",
			wxOK | wxICON_ERROR, this);
		return;
	}
	RefreshList();
	const long reselect = std::min<long>(sel, m_list->GetItemCount() - 1);
	if (reselect >= 0)
	{
		m_list->SetItemState(reselect, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED,
			wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
	}
}

void RoomActionsEditorFrame::OnRowSelected(wxListEvent& evt)
{
	CommitCurrentEdit();
	ShowRow(evt.GetIndex());
	UpdateButtons();
}
