#include <rooms/NewRoomDialog.h>

#include <algorithm>

#include <landstalker/misc/Labels.h>
#include <landstalker/misc/Utils.h>
#include <misc/LookupChoiceControl.h>
#include <rooms/NewMapDialog.h>

namespace
{

enum ID
{
	ID_MAP = wxID_HIGHEST + 1,
	ID_NEW_MAP,
	ID_TILESET,
	ID_PALETTE,
	ID_PRI_BLOCKSET,
	ID_SEC_BLOCKSET,
	ID_BGM
};

// A room stores its Z bounds in four bits each, and AddRoom rejects anything larger.
constexpr int MAX_ROOM_HEIGHT = 15;
// The room word holds five bits of BGM, but only the first nineteen tracks are named.
constexpr uint8_t BGM_COUNT = 32;

const wxSize FIELD_MIN_SIZE(260, -1);

}

int NewRoomDialog::Choices::IndexOf(uint8_t value) const
{
	const auto entry = std::find(values.cbegin(), values.cend(), value);
	return entry == values.cend()
		? wxNOT_FOUND
		: static_cast<int>(std::distance(values.cbegin(), entry));
}

NewRoomDialog::NewRoomDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd)
	: wxDialog(parent, wxID_ANY, "Add Room"),
	  m_gd(gd),
	  m_maps_changed(false),
	  m_name(nullptr),
	  m_display_name(nullptr),
	  m_map(nullptr),
	  m_new_map(nullptr),
	  m_tileset(nullptr),
	  m_palette(nullptr),
	  m_pri_blockset(nullptr),
	  m_sec_blockset(nullptr),
	  m_floor(nullptr),
	  m_ceiling(nullptr),
	  m_bgm(nullptr),
	  m_fields(nullptr)
{
	const auto room_data = m_gd->GetRoomData();

	BuildMapChoices();
	BuildTilesetChoices();

	m_palettes.labels.Clear();
	m_palettes.values.clear();
	for (std::size_t i = 0; i < room_data->GetRoomPalettes().size(); ++i)
	{
		m_palettes.labels.Add(wxString(room_data->GetRoomPaletteDisplayName(static_cast<uint8_t>(i))));
		m_palettes.values.push_back(static_cast<uint8_t>(i));
	}

	m_bgms.labels.Clear();
	m_bgms.values.clear();
	for (uint8_t i = 0; i < BGM_COUNT; ++i)
	{
		m_bgms.labels.Add(Landstalker::Labels::Get(Landstalker::Labels::C_BGMS, i)
			.value_or(Landstalker::StrWPrintf(L"[%02X] Track %d", i, i)));
		m_bgms.values.push_back(i);
	}

	auto* outer = new wxBoxSizer(wxVERTICAL);
	outer->Add(new wxStaticText(this, wxID_ANY,
		"The room is appended to the end of the room list, so no existing room number changes.\n"
		"The internal name is the assembly label; the display name is used only in the editor."),
		0, wxLEFT | wxRIGHT | wxTOP, 10);

	m_fields = new wxFlexGridSizer(2, 6, 6);
	m_fields->AddGrowableCol(1, 1);

	const auto add_row = [&](const char* label, wxWindow* control)
	{
		m_fields->Add(new wxStaticText(this, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
		m_fields->Add(control, 1, wxEXPAND);
	};
	const auto add_lookup = [&](const char* label, int id, const Choices& choices, int selection)
	{
		auto* control = new LookupChoiceControl(this, id, wxEmptyString, choices.labels);
		control->SetMinSize(FIELD_MIN_SIZE);
		control->SetSelection(selection);
		add_row(label, control);
		return control;
	};

	m_name = new wxTextCtrl(this, wxID_ANY, wxString::FromUTF8(SuggestRoomName()));
	m_name->SetMaxLength(30);
	add_row("Internal name", m_name);
	m_display_name = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	add_row("Display name", m_display_name);

	// The map gets its own row so the New Map button can sit beside the picker.
	m_fields->Add(new wxStaticText(this, wxID_ANY, "Map"), 0, wxALIGN_CENTER_VERTICAL);
	auto* map_row = new wxBoxSizer(wxHORIZONTAL);
	m_map = new LookupChoiceControl(this, ID_MAP, wxEmptyString, m_maps);
	m_map->SetMinSize(FIELD_MIN_SIZE);
	m_map->SetSelection(m_maps.IsEmpty() ? wxNOT_FOUND : 0);
	m_new_map = new wxButton(this, ID_NEW_MAP, "New Map...");
	map_row->Add(m_map, 1, wxEXPAND);
	map_row->Add(m_new_map, 0, wxLEFT, 6);
	m_fields->Add(map_row, 1, wxEXPAND);

	m_tileset = add_lookup("Tileset", ID_TILESET, m_tilesets, m_tilesets.labels.IsEmpty() ? wxNOT_FOUND : 0);
	m_palette = add_lookup("Palette", ID_PALETTE, m_palettes, m_palettes.labels.IsEmpty() ? wxNOT_FOUND : 0);

	const uint8_t tileset = SelectedValue(m_tileset, m_tilesets, 0);
	m_pri_blocksets = BuildPrimaryBlocksetChoices(tileset);
	m_sec_blocksets = BuildSecondaryBlocksetChoices(tileset, m_pri_blocksets.values.empty() ? 0 : m_pri_blocksets.values.front());
	m_pri_blockset = add_lookup("Primary blockset", ID_PRI_BLOCKSET, m_pri_blocksets,
		m_pri_blocksets.labels.IsEmpty() ? wxNOT_FOUND : 0);
	m_sec_blockset = add_lookup("Secondary blockset", ID_SEC_BLOCKSET, m_sec_blocksets,
		m_sec_blocksets.labels.IsEmpty() ? wxNOT_FOUND : 0);

	m_floor = new wxSpinCtrl(this, wxID_ANY);
	m_floor->SetRange(0, MAX_ROOM_HEIGHT);
	m_floor->SetValue(0);
	add_row("Floor height (Z begin)", m_floor);
	m_ceiling = new wxSpinCtrl(this, wxID_ANY);
	m_ceiling->SetRange(0, MAX_ROOM_HEIGHT);
	m_ceiling->SetValue(MAX_ROOM_HEIGHT);
	add_row("Ceiling height (Z end)", m_ceiling);

	m_bgm = add_lookup("BGM", ID_BGM, m_bgms, 0);

	outer->Add(m_fields, 1, wxALL | wxEXPAND, 10);
	outer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL), 0, wxALL | wxEXPAND, 10);
	SetSizerAndFit(outer);
	SetMinSize(GetSize());
	CentreOnParent();

	// Take the starting map's settings, so the common case of adding a room to an existing
	// map needs no further picking.
	if (!m_maps.IsEmpty())
	{
		AdoptSettingsFromMap(GetMap());
	}

	Bind(wxEVT_CHOICE, &NewRoomDialog::OnMapChanged, this, ID_MAP);
	Bind(wxEVT_CHOICE, &NewRoomDialog::OnTilesetChanged, this, ID_TILESET);
	Bind(wxEVT_CHOICE, &NewRoomDialog::OnPrimaryBlocksetChanged, this, ID_PRI_BLOCKSET);
	Bind(wxEVT_BUTTON, &NewRoomDialog::OnNewMap, this, ID_NEW_MAP);
	Bind(wxEVT_BUTTON, &NewRoomDialog::OnOk, this, wxID_OK);
}

NewRoomDialog::~NewRoomDialog()
{
}

std::string NewRoomDialog::GetRoomName() const
{
	return m_name->GetValue().ToStdString();
}

std::wstring NewRoomDialog::GetDisplayName() const
{
	return m_display_name->GetValue().ToStdWstring();
}

std::string NewRoomDialog::GetMap() const
{
	const int selection = m_map->GetSelection();
	if (selection == wxNOT_FOUND || selection >= static_cast<int>(m_maps.GetCount()))
	{
		return std::string();
	}
	return m_maps[static_cast<std::size_t>(selection)].ToStdString();
}

uint8_t NewRoomDialog::GetTileset() const { return SelectedValue(m_tileset, m_tilesets, 0); }
uint8_t NewRoomDialog::GetPalette() const { return SelectedValue(m_palette, m_palettes, 0); }
uint8_t NewRoomDialog::GetPrimaryBlockset() const { return SelectedValue(m_pri_blockset, m_pri_blocksets, 0); }
uint8_t NewRoomDialog::GetSecondaryBlockset() const { return SelectedValue(m_sec_blockset, m_sec_blocksets, 0); }
uint8_t NewRoomDialog::GetFloorHeight() const { return static_cast<uint8_t>(m_floor->GetValue()); }
uint8_t NewRoomDialog::GetCeilingHeight() const { return static_cast<uint8_t>(m_ceiling->GetValue()); }
uint8_t NewRoomDialog::GetBgm() const { return SelectedValue(m_bgm, m_bgms, 0); }

uint8_t NewRoomDialog::SelectedValue(const LookupChoiceControl* control, const Choices& choices, uint8_t fallback) const
{
	if (control == nullptr)
	{
		return fallback;
	}
	const int selection = control->GetSelection();
	if (selection == wxNOT_FOUND || selection >= static_cast<int>(choices.values.size()))
	{
		return fallback;
	}
	return choices.values[static_cast<std::size_t>(selection)];
}

void NewRoomDialog::BuildMapChoices()
{
	m_maps.Clear();
	for (const auto& map : m_gd->GetRoomData()->GetMapOrder())
	{
		m_maps.Add(wxString::FromUTF8(map));
	}
}

void NewRoomDialog::BuildTilesetChoices()
{
	m_tilesets.labels.Clear();
	m_tilesets.values.clear();
	for (const auto& tileset : m_gd->GetRoomData()->GetTilesets())
	{
		if (!tileset)
		{
			continue;
		}
		const auto index = static_cast<uint8_t>(tileset->GetIndex());
		m_tilesets.labels.Add(wxString::FromUTF8(tileset->GetName()));
		m_tilesets.values.push_back(index);
	}
}

NewRoomDialog::Choices NewRoomDialog::BuildPrimaryBlocksetChoices(uint8_t tileset) const
{
	// A room resolves its primary blockset as (primary, 0). AddRoom only accepts 0 or 1,
	// and refuses a room whose blocksets do not exist, so offer only the ones that do.
	Choices choices;
	const auto room_data = m_gd->GetRoomData();
	for (uint8_t pri = 0; pri <= 1; ++pri)
	{
		const auto blockset = room_data->GetBlockset(tileset, pri, 0);
		if (blockset)
		{
			choices.labels.Add(wxString::FromUTF8(blockset->GetName()));
			choices.values.push_back(pri);
		}
	}
	return choices;
}

NewRoomDialog::Choices NewRoomDialog::BuildSecondaryBlocksetChoices(uint8_t tileset, uint8_t primary) const
{
	// The secondary blockset is stored one below the index it resolves to, so a stored 0
	// means blockset 1. AddRoom accepts 0..7.
	Choices choices;
	const auto room_data = m_gd->GetRoomData();
	for (uint8_t sec = 0; sec <= 7; ++sec)
	{
		const auto blockset = room_data->GetBlockset(tileset, primary, static_cast<uint8_t>(sec + 1));
		if (blockset)
		{
			choices.labels.Add(wxString::FromUTF8(blockset->GetName()));
			choices.values.push_back(sec);
		}
	}
	return choices;
}

void NewRoomDialog::ReplaceControl(LookupChoiceControl*& control, const Choices& choices, int selection)
{
	auto* replacement = new LookupChoiceControl(this, control->GetId(), wxEmptyString, choices.labels);
	replacement->SetMinSize(FIELD_MIN_SIZE);
	replacement->SetSelection(selection);
	m_fields->Replace(control, replacement);
	control->Destroy();
	control = replacement;
}

void NewRoomDialog::RebuildBlocksetControls(uint8_t preferred_pri, uint8_t preferred_sec)
{
	const uint8_t tileset = GetTileset();
	m_pri_blocksets = BuildPrimaryBlocksetChoices(tileset);
	int pri_selection = m_pri_blocksets.IndexOf(preferred_pri);
	if (pri_selection == wxNOT_FOUND && !m_pri_blocksets.values.empty())
	{
		pri_selection = 0;
	}
	ReplaceControl(m_pri_blockset, m_pri_blocksets, pri_selection);

	const uint8_t primary = pri_selection == wxNOT_FOUND
		? 0
		: m_pri_blocksets.values[static_cast<std::size_t>(pri_selection)];
	m_sec_blocksets = BuildSecondaryBlocksetChoices(tileset, primary);
	int sec_selection = m_sec_blocksets.IndexOf(preferred_sec);
	if (sec_selection == wxNOT_FOUND && !m_sec_blocksets.values.empty())
	{
		sec_selection = 0;
	}
	ReplaceControl(m_sec_blockset, m_sec_blocksets, sec_selection);

	m_fields->Layout();
	Layout();
}

void NewRoomDialog::AdoptSettingsFromMap(const std::string& map)
{
	if (map.empty())
	{
		return;
	}

	// Only copy a setting if every room already drawing this map agrees on it: where they
	// differ there is no right answer, so the current choice is left alone.
	const auto& rooms = m_gd->GetRoomData()->GetRoomlist();
	const Landstalker::Room* first = nullptr;
	bool tileset_agrees = true;
	bool palette_agrees = true;
	bool blocksets_agree = true;
	bool floor_agrees = true;
	bool ceiling_agrees = true;
	bool bgm_agrees = true;
	for (const auto& room : rooms)
	{
		if (!room || room->map != map)
		{
			continue;
		}
		if (first == nullptr)
		{
			first = room.get();
			continue;
		}
		tileset_agrees = tileset_agrees && room->tileset == first->tileset;
		palette_agrees = palette_agrees && room->room_palette == first->room_palette;
		blocksets_agree = blocksets_agree && room->pri_blockset == first->pri_blockset &&
			room->sec_blockset == first->sec_blockset;
		floor_agrees = floor_agrees && room->room_z_begin == first->room_z_begin;
		ceiling_agrees = ceiling_agrees && room->room_z_end == first->room_z_end;
		bgm_agrees = bgm_agrees && room->bgm == first->bgm;
	}
	if (first == nullptr)
	{
		// A brand new map has no room to copy from, so everything stays as it is.
		return;
	}

	// The blocksets on offer depend on the tileset, so settle the tileset first and let the
	// rebuild place the blockset selection.
	const bool tileset_changed = tileset_agrees && GetTileset() != first->tileset;
	if (tileset_agrees)
	{
		const int selection = m_tilesets.IndexOf(first->tileset);
		if (selection != wxNOT_FOUND)
		{
			m_tileset->SetSelection(selection);
		}
	}
	if (tileset_changed || blocksets_agree)
	{
		RebuildBlocksetControls(
			blocksets_agree ? first->pri_blockset : GetPrimaryBlockset(),
			blocksets_agree ? first->sec_blockset : GetSecondaryBlockset());
	}
	if (palette_agrees)
	{
		const int selection = m_palettes.IndexOf(first->room_palette);
		if (selection != wxNOT_FOUND)
		{
			m_palette->SetSelection(selection);
		}
	}
	if (floor_agrees)
	{
		m_floor->SetValue(first->room_z_begin);
	}
	if (ceiling_agrees)
	{
		m_ceiling->SetValue(first->room_z_end);
	}
	if (bgm_agrees)
	{
		const int selection = m_bgms.IndexOf(first->bgm);
		if (selection != wxNOT_FOUND)
		{
			m_bgm->SetSelection(selection);
		}
	}
}

std::string NewRoomDialog::SuggestRoomName() const
{
	const auto room_data = m_gd->GetRoomData();
	for (unsigned int i = room_data->GetRoomCount(); ; ++i)
	{
		const auto candidate = Landstalker::StrPrintf("Room%03u", i);
		if (room_data->GetRoom(candidate) == nullptr)
		{
			return candidate;
		}
	}
}

bool NewRoomDialog::Validate()
{
	const auto room_data = m_gd->GetRoomData();
	const auto name = GetRoomName();
	if (!Landstalker::RoomData::IsValidRoomName(name) || room_data->GetRoom(name) != nullptr)
	{
		wxMessageBox("The internal name must be unique, start with a letter, only contain "
			"A-Z, a-z, 0-9 and _, and be at most 30 characters.",
			"Add Room", wxOK | wxICON_ERROR, this);
		return false;
	}
	const auto display_name = GetDisplayName();
	if (!display_name.empty() && !Landstalker::Labels::IsValid(display_name))
	{
		wxMessageBox("The display name must be unique and must not contain non-printable characters.",
			"Add Room", wxOK | wxICON_ERROR, this);
		return false;
	}
	if (GetMap().empty())
	{
		wxMessageBox("A room must draw a map. Pick one, or create a new one.",
			"Add Room", wxOK | wxICON_ERROR, this);
		return false;
	}
	if (m_pri_blocksets.values.empty() || m_sec_blocksets.values.empty())
	{
		wxMessageBox("The selected tileset has no usable pair of blocksets, so a room using it "
			"could not be drawn. Pick another tileset.",
			"Add Room", wxOK | wxICON_ERROR, this);
		return false;
	}
	return true;
}

void NewRoomDialog::OnMapChanged(wxCommandEvent& evt)
{
	AdoptSettingsFromMap(GetMap());
	evt.Skip();
}

void NewRoomDialog::OnTilesetChanged(wxCommandEvent& evt)
{
	// Keep the current blocksets if the new tileset offers them, rather than resetting.
	RebuildBlocksetControls(GetPrimaryBlockset(), GetSecondaryBlockset());
	evt.Skip();
}

void NewRoomDialog::OnPrimaryBlocksetChanged(wxCommandEvent& evt)
{
	const uint8_t primary = GetPrimaryBlockset();
	const uint8_t preferred = GetSecondaryBlockset();
	m_sec_blocksets = BuildSecondaryBlocksetChoices(GetTileset(), primary);
	int selection = m_sec_blocksets.IndexOf(preferred);
	if (selection == wxNOT_FOUND && !m_sec_blocksets.values.empty())
	{
		selection = 0;
	}
	ReplaceControl(m_sec_blockset, m_sec_blocksets, selection);
	m_fields->Layout();
	Layout();
	evt.Skip();
}

void NewRoomDialog::OnNewMap(wxCommandEvent& /*evt*/)
{
	const auto name = PromptCreateMap(this, m_gd);
	if (name.empty())
	{
		return;
	}
	m_maps_changed = true;
	BuildMapChoices();
	// The picker holds its list from construction, so it has to be rebuilt to see the new
	// map. Select it: the user made it for this room.
	auto* replacement = new LookupChoiceControl(this, ID_MAP, wxEmptyString, m_maps);
	replacement->SetMinSize(FIELD_MIN_SIZE);
	replacement->SetSelection(m_maps.Index(wxString::FromUTF8(name)));
	GetSizer()->Replace(m_map, replacement, true);
	m_map->Destroy();
	m_map = replacement;
	Layout();

	// A new map has no rooms to copy settings from, so this leaves everything as it was -
	// but it keeps the behaviour identical to picking a map from the list.
	AdoptSettingsFromMap(name);
}

void NewRoomDialog::OnOk(wxCommandEvent& evt)
{
	if (!Validate())
	{
		return;
	}
	evt.Skip();
}
