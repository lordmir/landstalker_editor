#include <rooms/RoomActionDialog.h>

#include <wx/button.h>
#include <wx/listbox.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include <landstalker/main/RoomData.h>
#include <landstalker/main/ScriptData.h>
#include <landstalker/misc/Utils.h>
#include <landstalker/script/RoomActionTable.h>
#include <rooms/RoomActionAddDialog.h>
#include <rooms/RoomActionDisplay.h>
#include <script/CutsceneCodeEditor.h>

namespace
{
	std::shared_ptr<Landstalker::RoomActionTable> GetTable(const std::shared_ptr<Landstalker::GameData>& gd)
	{
		if (!gd || !gd->GetScriptData()) return nullptr;
		auto t = gd->GetScriptData()->GetRoomActions();
		return (t && t->IsValid()) ? t : nullptr;
	}
}

RoomActionDialog::RoomActionDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd, int room)
	: wxDialog(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(720, 520),
		wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	m_gd(std::move(gd)),
	m_room(room)
{
	wxString name;
	if (m_gd && m_gd->GetRoomData() && room >= 0 && room < static_cast<int>(m_gd->GetRoomData()->GetRoomCount()))
	{
		name = wxString::FromUTF8(Landstalker::wstr_to_utf8(
			m_gd->GetRoomData()->GetRoomDisplayName(static_cast<uint16_t>(room))));
	}
	SetTitle(wxString::Format("Room Actions - %d %s", room, name));

	auto* sizer = new wxBoxSizer(wxVERTICAL);
	auto* top = new wxBoxSizer(wxHORIZONTAL);

	auto* left = new wxBoxSizer(wxVERTICAL);
	left->Add(new wxStaticText(this, wxID_ANY, "Actions for this room:"), 0, wxBOTTOM, 2);
	m_list = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(200, -1));
	left->Add(m_list, 1, wxEXPAND);
	auto* btns = new wxBoxSizer(wxHORIZONTAL);
	auto* add = new wxButton(this, wxID_ANY, "Add...");
	m_remove = new wxButton(this, wxID_ANY, "Remove");
	btns->Add(add, 0, wxRIGHT, 2);
	btns->Add(m_remove, 0);
	left->Add(btns, 0, wxTOP, 2);
	top->Add(left, 0, wxEXPAND | wxRIGHT, 6);

	m_code = new CutsceneCodeEditor(this, wxID_ANY);
	m_code->SetGameData(m_gd);
	top->Add(m_code, 1, wxEXPAND);

	sizer->Add(top, 1, wxEXPAND | wxALL, 6);
	sizer->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 6);
	SetSizer(sizer);

	m_list->Bind(wxEVT_LISTBOX, [this](wxCommandEvent&) { CommitCurrentEdit(); ShowSelected(); UpdateButtons(); });
	add->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnAdd(); });
	m_remove->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnRemove(); });
	Bind(wxEVT_BUTTON, &RoomActionDialog::OnOk, this, wxID_OK);

	Rebuild();
}

void RoomActionDialog::Rebuild(const std::string& select_label)
{
	m_list->Clear();
	m_labels.clear();
	m_current_label.clear();
	m_code->ClearAsm();
	auto table = GetTable(m_gd);
	if (!table)
	{
		UpdateButtons();
		return;
	}
	const auto& branches = table->GetBranches();
	int select_row = -1;
	for (std::size_t idx : table->FindByRoom(m_room))
	{
		if (!select_label.empty() && branches[idx].label == select_label)
		{
			select_row = static_cast<int>(m_labels.size());
		}
		m_labels.push_back(branches[idx].label);
		m_list->Append(RoomActionDisplay::ActionName(static_cast<int>(idx)));
	}
	if (!m_labels.empty())
	{
		m_list->SetSelection(select_row >= 0 ? select_row : 0);
		ShowSelected();
	}
	UpdateButtons();
}

void RoomActionDialog::ShowSelected()
{
	const int row = m_list->GetSelection();
	if (row == wxNOT_FOUND || row >= static_cast<int>(m_labels.size()))
	{
		m_code->ClearAsm();
		m_current_label.clear();
		return;
	}
	auto table = GetTable(m_gd);
	const auto* b = table ? table->FindByLabel(m_labels[static_cast<std::size_t>(row)]) : nullptr;
	if (b)
	{
		m_current_label = b->label;
		m_code->LoadAsm(b->body, true);
	}
}

void RoomActionDialog::CommitCurrentEdit()
{
	if (m_current_label.empty() || !m_code || !m_code->IsDirty()) return;
	auto table = GetTable(m_gd);
	if (table) table->SetBody(m_current_label, m_code->GetAsm());
	m_code->MarkClean();
}

void RoomActionDialog::OnAdd()
{
	auto table = GetTable(m_gd);
	if (!table) return;
	RoomActionAddDialog dlg(this, m_gd, m_room);
	if (dlg.ShowModal() != wxID_OK) return;
	CommitCurrentEdit();
	const auto label = dlg.IsBgm() ? table->AddBgmBranch(dlg.GetValue()) : table->AddRoomBranch(dlg.GetValue());
	if (!label)
	{
		wxMessageBox("Could not add the room action.", "Add Room Action", wxOK | wxICON_ERROR, this);
		return;
	}
	// A branch added for this room shows in the list; a BGM / other-room branch is added to the table
	// but not shown here (it appears in the global Room Actions editor).
	Rebuild(*label);
}

void RoomActionDialog::OnRemove()
{
	auto table = GetTable(m_gd);
	if (!table || m_current_label.empty()) return;
	if (!table->IsRemovable(m_current_label))
	{
		wxMessageBox("This branch cannot be removed automatically.", "Remove Room Action",
			wxOK | wxICON_ERROR, this);
		return;
	}
	if (wxMessageBox("Remove this room action?", "Remove Room Action",
		wxYES_NO | wxICON_QUESTION, this) != wxYES)
	{
		return;
	}
	m_code->MarkClean(); // discard the edit for the branch being removed
	table->RemoveBranch(m_current_label);
	Rebuild();
}

void RoomActionDialog::UpdateButtons()
{
	auto table = GetTable(m_gd);
	m_remove->Enable(table && !m_current_label.empty() && table->IsRemovable(m_current_label));
}

void RoomActionDialog::OnOk(wxCommandEvent& evt)
{
	CommitCurrentEdit();
	evt.Skip();
}
