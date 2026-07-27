#include <script/CutsceneEditorFrame.h>

#include <wx/listctrl.h>
#include <wx/button.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/wrapsizer.h>
#include <wx/textdlg.h>
#include <wx/msgdlg.h>

#include <algorithm>

#include <landstalker/main/ScriptData.h>
#include <landstalker/script/AsmFunctionTable.h>
#include <landstalker/misc/Labels.h>
#include <script/CutsceneCodeEditor.h>

namespace
{
	enum
	{
		ID_MOVE_UP = wxID_HIGHEST + 1,
		ID_MOVE_DOWN,
		ID_ADD,
		ID_REMOVE,
		ID_RENAME
	};

	// Shared handler a "removed" (blanked) middle slot dispatches to - a no-op that just returns.
	const char* const NULL_HANDLER = "CSA_Unused";

	// A short, human-useful one-line summary of a handler block for the list: the cutscene the
	// handler plays if it names one, otherwise its first comment, otherwise its first code line.
	wxString SummariseBlock(const std::string& text)
	{
		std::string first_comment;
		std::string first_code;
		std::size_t start = 0;
		while (start < text.size())
		{
			std::size_t end = text.find('\n', start);
			if (end == std::string::npos)
			{
				end = text.size();
			}
			std::string line = text.substr(start, end - start);
			start = end + 1;
			while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t'))
			{
				line.pop_back();
			}
			std::size_t i = 0;
			while (i < line.size() && (line[i] == ' ' || line[i] == '\t'))
			{
				++i;
			}
			line = line.substr(i);
			if (line.empty())
			{
				continue;
			}
			if (line[0] == ';')
			{
				std::string body = line.substr(1);
				while (!body.empty() && (body.front() == ' ' || body.front() == '-'))
				{
					body.erase(body.begin());
				}
				if (body.empty())
				{
					continue; // divider line
				}
				if (body.find("Cutscene $") != std::string::npos)
				{
					return wxString::FromUTF8(body);
				}
				if (first_comment.empty())
				{
					first_comment = body;
				}
			}
			else if (line.back() != ':' && first_code.empty())
			{
				first_code = line; // a code line (labels end in ':')
			}
		}
		if (!first_comment.empty())
		{
			return wxString::FromUTF8(first_comment);
		}
		return wxString::FromUTF8(first_code);
	}

	std::shared_ptr<Landstalker::AsmFunctionTable> GetTable(const std::shared_ptr<Landstalker::GameData>& gd)
	{
		if (!gd || !gd->GetScriptData())
		{
			return nullptr;
		}
		auto table = gd->GetScriptData()->GetCutsceneActions();
		return (table && table->IsValid()) ? table : nullptr;
	}

	// The custom C_CUTSCENE label if set, else a generic "CutsceneNNN" seed so the list is never
	// blank (the seed persists only if the user edits/renames it).
	wxString CutsceneName(int index)
	{
		auto name = Landstalker::Labels::Get(Landstalker::Labels::C_CUTSCENE, index);
		if (name && !name->empty())
		{
			return wxString(*name);
		}
		return wxString::Format("Cutscene%03X", static_cast<unsigned>(index));
	}

	// The custom name only (empty when unnamed), for pre-filling the rename dialog.
	wxString CutsceneCustomName(int index)
	{
		auto name = Landstalker::Labels::Get(Landstalker::Labels::C_CUTSCENE, index);
		return name ? wxString(*name) : wxString();
	}
}

CutsceneEditorFrame::CutsceneEditorFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst)
{
	m_mgr.SetManagedWindow(this);
	BuildUI();
	m_mgr.Update();
}

CutsceneEditorFrame::~CutsceneEditorFrame()
{
	m_mgr.UnInit();
}

void CutsceneEditorFrame::BuildUI()
{
	auto* splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxSP_LIVE_UPDATE | wxSP_3D);
	splitter->SetMinimumPaneSize(220);

	// Left: the dispatch slots (one per cutscene id).
	auto* left = new wxPanel(splitter, wxID_ANY);
	auto* left_sizer = new wxBoxSizer(wxVERTICAL);
	m_list = new wxListCtrl(left, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxLC_REPORT | wxLC_SINGLE_SEL);
	m_list->InsertColumn(0, "ID", wxLIST_FORMAT_LEFT, 55);
	m_list->InsertColumn(1, "Name", wxLIST_FORMAT_LEFT, 150);
	m_list->InsertColumn(2, "Handler", wxLIST_FORMAT_LEFT, 100);
	m_list->InsertColumn(3, "Summary", wxLIST_FORMAT_LEFT, 300);
	left_sizer->Add(m_list, 1, wxEXPAND);

	auto* btns = new wxWrapSizer(wxHORIZONTAL);
	m_add = new wxButton(left, ID_ADD, "Add");
	m_remove = new wxButton(left, ID_REMOVE, "Remove");
	m_rename = new wxButton(left, ID_RENAME, "Rename");
	m_move_up = new wxButton(left, ID_MOVE_UP, "Move Up");
	m_move_down = new wxButton(left, ID_MOVE_DOWN, "Move Down");
	for (wxButton* b : { m_add, m_remove, m_rename, m_move_up, m_move_down })
	{
		btns->Add(b, 0, wxALL, 2);
	}
	left_sizer->Add(btns, 0, wxEXPAND);
	left->SetSizer(left_sizer);

	// Right: the selected handler's raw m68k, edited via the reusable code editor. The cutscene
	// scripts a handler plays are reached inline (Ctrl+Click a <PlayCutscene ...> token or its hint).
	auto* right = new wxPanel(splitter, wxID_ANY);
	auto* right_sizer = new wxBoxSizer(wxVERTICAL);
	m_code = new CutsceneCodeEditor(right, wxID_ANY);
	right_sizer->Add(m_code, 1, wxEXPAND);
	right->SetSizer(right_sizer);

	splitter->SplitVertically(left, right, 520);
	m_mgr.AddPane(splitter, wxAuiPaneInfo().CenterPane());

	m_list->Bind(wxEVT_LIST_ITEM_SELECTED, &CutsceneEditorFrame::OnSlotSelected, this);
	m_add->Bind(wxEVT_BUTTON, &CutsceneEditorFrame::OnAdd, this);
	m_remove->Bind(wxEVT_BUTTON, &CutsceneEditorFrame::OnRemove, this);
	m_rename->Bind(wxEVT_BUTTON, &CutsceneEditorFrame::OnRename, this);
	m_move_up->Bind(wxEVT_BUTTON, &CutsceneEditorFrame::OnMoveUp, this);
	m_move_down->Bind(wxEVT_BUTTON, &CutsceneEditorFrame::OnMoveDown, this);
}

bool CutsceneEditorFrame::Open(int slot)
{
	if (!GetTable(m_gd))
	{
		return false;
	}
	RefreshList();
	// Default to the first slot so the code pane is populated on open.
	if (slot < 0 && m_list->GetItemCount() > 0)
	{
		slot = 0;
	}
	if (slot >= 0 && slot < m_list->GetItemCount())
	{
		m_list->SetItemState(slot, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED,
			wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
		m_list->EnsureVisible(slot);
	}
	return true;
}

void CutsceneEditorFrame::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	m_gd = gd;
	if (m_code)
	{
		m_code->SetGameData(gd);
	}
	RefreshList();
}

void CutsceneEditorFrame::ClearGameData()
{
	CommitCurrentEdit();
	m_gd.reset();
	m_current_slot = -1;
	m_current_label.clear();
	if (m_list)
	{
		m_list->DeleteAllItems();
	}
	if (m_code)
	{
		m_code->SetGameData(nullptr);
		m_code->ClearAsm();
	}
	UpdateButtons();
}

void CutsceneEditorFrame::RefreshRow(long slot)
{
	auto table = GetTable(m_gd);
	if (!table || slot < 0 || slot >= static_cast<long>(table->SlotCount()))
	{
		return;
	}
	const std::string& label = table->GetSlotLabel(static_cast<std::size_t>(slot));
	m_list->SetItem(slot, 1, CutsceneName(static_cast<int>(slot)));
	m_list->SetItem(slot, 2, wxString::FromUTF8(label));
	const auto* block = table->FindBlock(label);
	m_list->SetItem(slot, 3, block ? SummariseBlock(block->text) : wxString("(external)"));
}

void CutsceneEditorFrame::RefreshList()
{
	if (!m_list)
	{
		return;
	}
	m_current_slot = -1;
	m_current_label.clear();
	m_list->DeleteAllItems();
	if (m_code)
	{
		m_code->ClearAsm();
	}

	auto table = GetTable(m_gd);
	if (!table)
	{
		UpdateButtons();
		return;
	}
	for (std::size_t i = 0; i < table->SlotCount(); ++i)
	{
		m_list->InsertItem(static_cast<long>(i), wxString::Format("$%03X", static_cast<unsigned>(i)));
		RefreshRow(static_cast<long>(i));
	}
	UpdateButtons();
}

void CutsceneEditorFrame::ShowSlot(long slot)
{
	auto table = GetTable(m_gd);
	if (!table || slot < 0 || slot >= static_cast<long>(table->SlotCount()))
	{
		return;
	}
	const std::string label = table->GetSlotLabel(static_cast<std::size_t>(slot));
	const auto* block = table->FindBlock(label);
	m_current_slot = slot;
	m_current_label = label;
	if (block)
	{
		m_code->LoadAsm(block->text, true);
	}
	else
	{
		m_code->ClearAsm(); // external slot: target lives in another file
	}
}

void CutsceneEditorFrame::CommitCurrentEdit()
{
	if (m_current_slot < 0 || m_current_label.empty() || !m_code || !m_code->IsDirty())
	{
		return;
	}
	auto table = GetTable(m_gd);
	if (table)
	{
		table->SetBlockText(m_current_label, m_code->GetAsm());
	}
	m_code->MarkClean();
}

void CutsceneEditorFrame::CommitPendingEdits()
{
	CommitCurrentEdit();
}

long CutsceneEditorFrame::SelectedSlot() const
{
	return m_list ? m_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED) : -1;
}

void CutsceneEditorFrame::UpdateButtons()
{
	auto table = GetTable(m_gd);
	const long sel = SelectedSlot();
	const long count = m_list ? m_list->GetItemCount() : 0;
	if (m_add)
	{
		m_add->Enable(table != nullptr);
	}
	if (m_remove)
	{
		m_remove->Enable(sel >= 0);
	}
	if (m_rename)
	{
		m_rename->Enable(sel >= 0);
	}
	if (m_move_up)
	{
		m_move_up->Enable(sel > 0);
	}
	if (m_move_down)
	{
		m_move_down->Enable(sel >= 0 && sel < count - 1);
	}
}

void CutsceneEditorFrame::MoveSelection(int direction)
{
	auto table = GetTable(m_gd);
	if (!table)
	{
		return;
	}
	const long sel = SelectedSlot();
	const long target = sel + direction;
	if (sel < 0 || target < 0 || target >= static_cast<long>(table->SlotCount()))
	{
		return;
	}
	CommitCurrentEdit();
	table->SwapSlots(static_cast<std::size_t>(sel), static_cast<std::size_t>(target));
	// The name describes the cutscene's content, so it travels with the handler that moved.
	Landstalker::Labels::Remap(Landstalker::Labels::C_CUTSCENE, { { sel, target }, { target, sel } });
	RefreshRow(sel);
	RefreshRow(target);
	m_list->SetItemState(target, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED,
		wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
	m_list->EnsureVisible(target);
}

void CutsceneEditorFrame::AddCutscene()
{
	auto table = GetTable(m_gd);
	if (!table)
	{
		return;
	}
	const long new_index = static_cast<long>(table->SlotCount());

	wxTextEntryDialog dlg(this, "Name for the new cutscene (optional):", "Add Cutscene");
	if (dlg.ShowModal() != wxID_OK)
	{
		return;
	}

	std::string label = wxString::Format("CSA_%04X", static_cast<unsigned>(new_index)).ToStdString();
	while (table->FindBlock(label))
	{
		label += "_";
	}
	CommitCurrentEdit();
	table->AddBlock(label, label + ":\n\t\trts\n; ---------------------------------------------------------------------------\n");
	table->AppendSlot(label);
	const wxString name = dlg.GetValue();
	if (!name.empty())
	{
		Landstalker::Labels::Update(Landstalker::Labels::C_CUTSCENE, static_cast<int>(new_index), name.ToStdWstring());
	}
	RefreshList();
	m_list->SetItemState(new_index, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED,
		wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
	m_list->EnsureVisible(new_index);
}

void CutsceneEditorFrame::RemoveCutscene()
{
	auto table = GetTable(m_gd);
	const long sel = SelectedSlot();
	if (!table || sel < 0)
	{
		return;
	}
	if (wxMessageBox(wxString::Format("Remove cutscene $%03X?\n\nExternal references to this index "
		"will remain but do nothing.", static_cast<unsigned>(sel)), "Remove Cutscene",
		wxYES_NO | wxICON_QUESTION, this) != wxYES)
	{
		return;
	}
	CommitCurrentEdit();
	const std::string label = table->GetSlotLabel(static_cast<std::size_t>(sel));
	if (sel == static_cast<long>(table->SlotCount()) - 1)
	{
		// Last index: drop the slot outright, and its handler too if nothing else uses it.
		table->PopSlot();
		if (!table->IsSlotTarget(label))
		{
			table->RemoveBlock(label);
		}
	}
	else
	{
		// Middle index: blank it in place so every later index stays valid.
		if (!table->FindBlock(NULL_HANDLER))
		{
			table->AddBlock(NULL_HANDLER, std::string(NULL_HANDLER) + ":\n\t\trts\n; ---------------------------------------------------------------------------\n");
		}
		table->SetSlotLabel(static_cast<std::size_t>(sel), NULL_HANDLER);
	}
	Landstalker::Labels::Remap(Landstalker::Labels::C_CUTSCENE, { { static_cast<int>(sel), -1 } });
	RefreshList();
	const long reselect = std::min<long>(sel, m_list->GetItemCount() - 1);
	if (reselect >= 0)
	{
		m_list->SetItemState(reselect, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED,
			wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
		m_list->EnsureVisible(reselect);
	}
}

void CutsceneEditorFrame::RenameCutscene()
{
	const long sel = SelectedSlot();
	if (sel < 0)
	{
		return;
	}
	wxTextEntryDialog dlg(this, wxString::Format("Name for cutscene $%03X:", static_cast<unsigned>(sel)),
		"Rename Cutscene", CutsceneCustomName(static_cast<int>(sel)));
	if (dlg.ShowModal() != wxID_OK)
	{
		return;
	}
	const wxString name = dlg.GetValue();
	if (name.empty())
	{
		Landstalker::Labels::Remap(Landstalker::Labels::C_CUTSCENE, { { static_cast<int>(sel), -1 } });
	}
	else
	{
		Landstalker::Labels::Update(Landstalker::Labels::C_CUTSCENE, static_cast<int>(sel), name.ToStdWstring());
	}
	RefreshRow(sel);
}

void CutsceneEditorFrame::OnSlotSelected(wxListEvent& evt)
{
	CommitCurrentEdit();
	ShowSlot(evt.GetIndex());
	UpdateButtons();
}

void CutsceneEditorFrame::OnMoveUp(wxCommandEvent&)
{
	MoveSelection(-1);
}

void CutsceneEditorFrame::OnMoveDown(wxCommandEvent&)
{
	MoveSelection(1);
}

void CutsceneEditorFrame::OnAdd(wxCommandEvent&)
{
	AddCutscene();
}

void CutsceneEditorFrame::OnRemove(wxCommandEvent&)
{
	RemoveCutscene();
}

void CutsceneEditorFrame::OnRename(wxCommandEvent&)
{
	RenameCutscene();
}
