#include <script/ItemUseEditorFrame.h>

#include <algorithm>

#include <wx/listctrl.h>
#include <wx/button.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/wrapsizer.h>
#include <wx/msgdlg.h>
#include <wx/choice.h>
#include <wx/stattext.h>

#include <landstalker/main/ScriptData.h>
#include <landstalker/main/StringData.h>
#include <landstalker/misc/Utils.h>
#include <landstalker/script/ItemUseTable.h>
#include <misc/LookupChoiceControl.h>
#include <script/CutsceneCodeEditor.h>

namespace
{
	enum { ID_ADD_PRE = wxID_HIGHEST + 1, ID_ADD_POST, ID_REMOVE };

	std::shared_ptr<Landstalker::ItemUseTable> GetTable(const std::shared_ptr<Landstalker::GameData>& gd)
	{
		if (!gd || !gd->GetScriptData()) return nullptr;
		auto t = gd->GetScriptData()->GetItemUse();
		return (t && t->IsValid()) ? t : nullptr;
	}

	wxString ItemName(const std::shared_ptr<Landstalker::GameData>& gd, int item)
	{
		wxString name;
		if (gd && gd->GetStringData()) name = wxString::FromUTF8(Landstalker::wstr_to_utf8(gd->GetStringData()->GetItemDisplayName(item)));
		return wxString::Format("%02X: %s", static_cast<unsigned>(item), name);
	}

	// Ask for an item via a searchable name dropdown. Returns -1 if cancelled.
	int PickItem(wxWindow* parent, const std::shared_ptr<Landstalker::GameData>& gd)
	{
		wxArrayString choices;
		auto strd = gd ? gd->GetStringData() : nullptr;
		for (std::size_t i = 0; strd && i < strd->GetItemNameCount(); ++i)
			choices.Add(ItemName(gd, static_cast<int>(i)));
		if (choices.IsEmpty()) return -1;
		wxDialog dlg(parent, wxID_ANY, "Add Item Handler", wxDefaultPosition, wxSize(320, -1));
		auto* s = new wxBoxSizer(wxVERTICAL);
		s->Add(new wxStaticText(&dlg, wxID_ANY, "Item:"), 0, wxLEFT | wxTOP, 8);
		auto* pick = new LookupChoiceControl(&dlg, wxID_ANY, choices[0], choices);
		s->Add(pick, 0, wxEXPAND | wxALL, 8);
		s->Add(dlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 8);
		dlg.SetSizerAndFit(s);
		if (dlg.ShowModal() != wxID_OK) return -1;
		pick->CommitPendingSelection();
		return pick->GetSelection();
	}
}

ItemUseEditorFrame::ItemUseEditorFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst)
{
	m_mgr.SetManagedWindow(this);
	BuildUI();
	m_mgr.Update();
}

ItemUseEditorFrame::~ItemUseEditorFrame()
{
	m_mgr.UnInit();
}

void ItemUseEditorFrame::BuildUI()
{
	auto* splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3D);
	splitter->SetMinimumPaneSize(240);

	auto* left = new wxPanel(splitter, wxID_ANY);
	auto* lv = new wxBoxSizer(wxVERTICAL);
	m_list = new wxListCtrl(left, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
	m_list->InsertColumn(0, "Kind", wxLIST_FORMAT_LEFT, 70);
	m_list->InsertColumn(1, "Item", wxLIST_FORMAT_LEFT, 180);
	m_list->InsertColumn(2, "Handler", wxLIST_FORMAT_LEFT, 150);
	lv->Add(m_list, 1, wxEXPAND);
	auto* btns = new wxWrapSizer(wxHORIZONTAL);
	m_add_pre = new wxButton(left, ID_ADD_PRE, "Add Pre-use...");
	m_add_post = new wxButton(left, ID_ADD_POST, "Add Post-use...");
	m_remove = new wxButton(left, ID_REMOVE, "Remove");
	btns->Add(m_add_pre, 0, wxALL, 2);
	btns->Add(m_add_post, 0, wxALL, 2);
	btns->Add(m_remove, 0, wxALL, 2);
	lv->Add(btns, 0, wxEXPAND);
	left->SetSizer(lv);

	auto* right = new wxPanel(splitter, wxID_ANY);
	auto* rv = new wxBoxSizer(wxVERTICAL);
	m_code = new CutsceneCodeEditor(right, wxID_ANY);
	rv->Add(m_code, 1, wxEXPAND);
	right->SetSizer(rv);

	splitter->SplitVertically(left, right, 420);
	m_mgr.AddPane(splitter, wxAuiPaneInfo().CenterPane());

	m_list->Bind(wxEVT_LIST_ITEM_SELECTED, &ItemUseEditorFrame::OnRowSelected, this);
	m_add_pre->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnAdd(true); });
	m_add_post->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnAdd(false); });
	m_remove->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnRemove(); });
}

bool ItemUseEditorFrame::Open()
{
	if (!GetTable(m_gd)) return false;
	RefreshList();
	if (m_list->GetItemCount() > 0)
	{
		m_list->SetItemState(0, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
	}
	return true;
}

void ItemUseEditorFrame::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	m_gd = gd;
	if (m_code) m_code->SetGameData(gd);
	RefreshList();
}

void ItemUseEditorFrame::ClearGameData()
{
	CommitCurrentEdit();
	m_gd.reset();
	m_current_row = -1;
	m_current_handler.clear();
	m_rows.clear();
	if (m_list) m_list->DeleteAllItems();
	if (m_code) { m_code->SetGameData(nullptr); m_code->ClearAsm(); }
	UpdateButtons();
}

void ItemUseEditorFrame::RefreshList()
{
	if (!m_list) return;
	m_current_row = -1;
	m_current_handler.clear();
	m_rows.clear();
	m_list->DeleteAllItems();
	if (m_code) m_code->ClearAsm();
	auto table = GetTable(m_gd);
	if (!table) { UpdateButtons(); return; }
	for (int item : table->PreUseItems()) m_rows.push_back({ true, item, table->PreUseHandlerFor(item) });
	for (int item : table->PostUseItems()) m_rows.push_back({ false, item, table->PostUseHandlerFor(item) });
	for (std::size_t i = 0; i < m_rows.size(); ++i)
	{
		m_list->InsertItem(static_cast<long>(i), m_rows[i].pre ? "Pre-use" : "Post-use");
		m_list->SetItem(static_cast<long>(i), 1, ItemName(m_gd, m_rows[i].item));
		m_list->SetItem(static_cast<long>(i), 2, wxString::FromUTF8(m_rows[i].handler));
	}
	UpdateButtons();
}

void ItemUseEditorFrame::ShowRow(long row)
{
	if (row < 0 || row >= static_cast<long>(m_rows.size())) return;
	auto table = GetTable(m_gd);
	if (!table) return;
	m_current_row = row;
	m_current_handler = m_rows[static_cast<std::size_t>(row)].handler;
	m_code->LoadAsm(table->GetBlockBody(m_current_handler), true);
}

void ItemUseEditorFrame::CommitCurrentEdit()
{
	if (m_current_handler.empty() || !m_code || !m_code->IsDirty()) return;
	auto table = GetTable(m_gd);
	if (table) table->SetBlockBody(m_current_handler, m_code->GetAsm());
	m_code->MarkClean();
}

void ItemUseEditorFrame::CommitPendingEdits()
{
	CommitCurrentEdit();
}

long ItemUseEditorFrame::SelectedRow() const
{
	return m_list ? m_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED) : -1;
}

void ItemUseEditorFrame::UpdateButtons()
{
	auto table = GetTable(m_gd);
	if (m_add_pre) m_add_pre->Enable(table != nullptr);
	if (m_add_post) m_add_post->Enable(table != nullptr);
	if (m_remove) m_remove->Enable(SelectedRow() >= 0);
}

void ItemUseEditorFrame::OnAdd(bool pre)
{
	auto table = GetTable(m_gd);
	if (!table) return;
	const int item = PickItem(this, m_gd);
	if (item < 0) return;
	CommitCurrentEdit();
	const auto handler = pre ? table->AddPreUse(item) : table->AddPostUse(item);
	if (!handler)
	{
		wxMessageBox("That item already has a handler of this kind.", "Add Item Handler", wxOK | wxICON_INFORMATION, this);
		return;
	}
	RefreshList();
	for (std::size_t i = 0; i < m_rows.size(); ++i)
	{
		if (m_rows[i].handler == *handler)
		{
			m_list->SetItemState(static_cast<long>(i), wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
			m_list->EnsureVisible(static_cast<long>(i));
			break;
		}
	}
}

void ItemUseEditorFrame::OnRemove()
{
	auto table = GetTable(m_gd);
	const long row = SelectedRow();
	if (!table || row < 0 || row >= static_cast<long>(m_rows.size())) return;
	const Row r = m_rows[static_cast<std::size_t>(row)];
	if (wxMessageBox("Unbind this handler from the item?", "Remove Item Handler", wxYES_NO | wxICON_QUESTION, this) != wxYES) return;
	CommitCurrentEdit();
	if (r.pre) table->RemovePreUse(r.item); else table->RemovePostUse(r.item);
	RefreshList();
	const long reselect = std::min<long>(row, m_list->GetItemCount() - 1);
	if (reselect >= 0) m_list->SetItemState(reselect, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
}

void ItemUseEditorFrame::OnRowSelected(wxListEvent& evt)
{
	CommitCurrentEdit();
	ShowRow(evt.GetIndex());
	UpdateButtons();
}
