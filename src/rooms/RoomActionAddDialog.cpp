#include <rooms/RoomActionAddDialog.h>

#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include <landstalker/main/RoomData.h>
#include <landstalker/misc/Labels.h>
#include <landstalker/misc/Utils.h>
#include <misc/LookupChoiceControl.h>

RoomActionAddDialog::RoomActionAddDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd,
	int default_room)
	: wxDialog(parent, wxID_ANY, "Add Room Action", wxDefaultPosition, wxSize(360, -1)),
	m_gd(std::move(gd))
{
	// Room choices are built in id order, so the list index IS the room number (GetValue reads it back
	// directly). The hex prefix lets the dropdown filter numerically as well as by name.
	wxArrayString room_choices;
	if (m_gd && m_gd->GetRoomData())
	{
		const auto count = m_gd->GetRoomData()->GetRoomCount();
		for (std::size_t i = 0; i < count; ++i)
		{
			room_choices.Add(wxString::Format("%03X: %s", static_cast<unsigned>(i),
				wxString::FromUTF8(Landstalker::wstr_to_utf8(
					m_gd->GetRoomData()->GetRoomDisplayName(static_cast<uint16_t>(i))))));
		}
	}
	// The BGM Labels category is sparse, so remember each row's real id for GetValue().
	wxArrayString bgm_choices;
	for (const auto& kv : Landstalker::Labels::GetCategory(Landstalker::Labels::C_BGMS))
	{
		m_bgm_ids.push_back(kv.first);
		bgm_choices.Add(wxString::Format("%02X: %s", static_cast<unsigned>(kv.first), wxString(kv.second)));
	}

	auto* sizer = new wxBoxSizer(wxVERTICAL);
	auto* grid = new wxFlexGridSizer(2, 4, 4);
	grid->AddGrowableCol(1, 1);

	grid->Add(new wxStaticText(this, wxID_ANY, "Key:"), 0, wxALIGN_CENTER_VERTICAL);
	m_type = new wxChoice(this, wxID_ANY);
	m_type->Append("Room");
	m_type->Append("BGM");
	m_type->SetSelection(0);
	grid->Add(m_type, 1, wxEXPAND);

	grid->Add(new wxStaticText(this, wxID_ANY, "Value:"), 0, wxALIGN_CENTER_VERTICAL);
	// Both pickers share the value cell; only the one matching the Key choice is shown at a time.
	auto* holder = new wxBoxSizer(wxHORIZONTAL);
	m_room_pick = new LookupChoiceControl(this, wxID_ANY,
		room_choices.IsEmpty() ? wxString() : room_choices[0], room_choices);
	m_bgm_pick = new LookupChoiceControl(this, wxID_ANY,
		bgm_choices.IsEmpty() ? wxString() : bgm_choices[0], bgm_choices);
	holder->Add(m_room_pick, 1, wxEXPAND);
	holder->Add(m_bgm_pick, 1, wxEXPAND);
	grid->Add(holder, 1, wxEXPAND);

	sizer->Add(grid, 0, wxEXPAND | wxALL, 8);
	sizer->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 8);
	SetSizerAndFit(sizer);

	if (default_room >= 0 && default_room < static_cast<int>(room_choices.GetCount()))
	{
		m_room_pick->SetSelection(default_room);
	}

	m_type->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { UpdateVisibility(); });
	// Commit whatever text is in the active picker before the modal returns (mirrors the shop picker).
	Bind(wxEVT_BUTTON, [this](wxCommandEvent& e)
	{
		m_room_pick->CommitPendingSelection();
		m_bgm_pick->CommitPendingSelection();
		e.Skip();
	}, wxID_OK);
	UpdateVisibility();
}

bool RoomActionAddDialog::IsBgm() const
{
	return m_type->GetSelection() == 1;
}

int RoomActionAddDialog::GetValue() const
{
	if (IsBgm())
	{
		const int idx = m_bgm_pick->GetSelection();
		return (idx >= 0 && idx < static_cast<int>(m_bgm_ids.size())) ? m_bgm_ids[idx] : 0;
	}
	const int idx = m_room_pick->GetSelection();
	return idx >= 0 ? idx : 0;
}

void RoomActionAddDialog::UpdateVisibility()
{
	const bool bgm = IsBgm();
	m_room_pick->Show(!bgm);
	m_bgm_pick->Show(bgm);
	Layout();
	Fit();
}
