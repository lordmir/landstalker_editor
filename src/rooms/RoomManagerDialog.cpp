#include <rooms/RoomManagerDialog.h>

#include <algorithm>
#include <cctype>
#include <filesystem>

#include <wx/filedlg.h>

#include <landstalker/3d_maps/MapToTmx.h>
#include <landstalker/3d_maps/RoomToTmx.h>
#include <landstalker/3d_maps/RoomToYaml.h>
#include <landstalker/misc/Labels.h>
#include <landstalker/misc/Utils.h>
#include <main/ImageBufferWx.h>
#include <rooms/MapFileIo.h>
#include <rooms/NewMapDialog.h>
#include <rooms/NewRoomDialog.h>

namespace
{

const wxSize LIST_MIN_SIZE(280, 320);
// Rooms are rendered at full size then scaled to fit inside this.
const wxSize PREVIEW_SIZE(300, 150);

std::string ToLower(const std::string& str)
{
	std::string result = str;
	std::transform(result.begin(), result.end(), result.begin(), [](const unsigned char c)
	{
		return static_cast<char>(std::tolower(c));
	});
	return result;
}

// Prompts for both of a room's names: the assembly label the disassembly is built around,
// and the friendlier label the editor shows. The two are independent - the internal name
// lives in the room data, the display name in the label table.
class RenameRoomDialog : public wxDialog
{
public:
	RenameRoomDialog(wxWindow* parent, const std::string& internal_name, const std::wstring& display_name)
		: wxDialog(parent, wxID_ANY, "Rename Room")
	{
		auto* outer = new wxBoxSizer(wxVERTICAL);
		outer->Add(new wxStaticText(this, wxID_ANY,
			"The internal name is the assembly label: a unique identifier of at most 30\n"
			"characters, starting with a letter. The display name is used only in the editor."),
			0, wxLEFT | wxRIGHT | wxTOP, 10);

		auto* fields = new wxFlexGridSizer(2, 6, 6);
		fields->AddGrowableCol(1, 1);
		fields->Add(new wxStaticText(this, wxID_ANY, "Internal name"), 0, wxALIGN_CENTER_VERTICAL);
		m_internal = new wxTextCtrl(this, wxID_ANY, wxString::FromUTF8(internal_name));
		m_internal->SetMaxLength(30);
		fields->Add(m_internal, 1, wxEXPAND);
		fields->Add(new wxStaticText(this, wxID_ANY, "Display name"), 0, wxALIGN_CENTER_VERTICAL);
		m_display = new wxTextCtrl(this, wxID_ANY, wxString(display_name));
		fields->Add(m_display, 1, wxEXPAND);

		outer->Add(fields, 1, wxALL | wxEXPAND, 10);
		outer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL), 0, wxALL | wxEXPAND, 10);
		SetSizerAndFit(outer);
		SetMinSize(wxSize(440, -1));
		CentreOnParent();
	}

	std::string GetInternalName() const { return m_internal->GetValue().ToStdString(); }
	std::wstring GetDisplayName() const { return m_display->GetValue().ToStdWstring(); }

private:
	wxTextCtrl* m_internal;
	wxTextCtrl* m_display;
};

// Prompts for the new room's names and whether its map should be copied too. Sharing the
// original's map is the default: two rooms drawing one map is normal, and a copy is only
// wanted when the geometry is about to diverge.
class DuplicateRoomDialog : public wxDialog
{
public:
	DuplicateRoomDialog(wxWindow* parent, const wxString& source, const std::string& suggested_room,
		const std::string& suggested_map)
		: wxDialog(parent, wxID_ANY, "Duplicate Room")
	{
		auto* outer = new wxBoxSizer(wxVERTICAL);
		outer->Add(new wxStaticText(this, wxID_ANY,
			"Duplicating '" + source + "'. The copy is appended to the end of the room list and\n"
			"takes the original's map, tileset, palette, blocksets, heights, BGM and entities.\n\n"
			"Warps, chests, doors, tile swaps, dialogue and flags are NOT copied: those either\n"
			"point at other rooms or are numbered by position, so duplicating them would alter\n"
			"rooms you did not touch."),
			0, wxLEFT | wxRIGHT | wxTOP, 10);

		auto* fields = new wxFlexGridSizer(2, 6, 6);
		fields->AddGrowableCol(1, 1);
		fields->Add(new wxStaticText(this, wxID_ANY, "Internal name"), 0, wxALIGN_CENTER_VERTICAL);
		m_name = new wxTextCtrl(this, wxID_ANY, wxString::FromUTF8(suggested_room));
		m_name->SetMaxLength(30);
		fields->Add(m_name, 1, wxEXPAND);
		fields->Add(new wxStaticText(this, wxID_ANY, "Display name"), 0, wxALIGN_CENTER_VERTICAL);
		m_display = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
		fields->Add(m_display, 1, wxEXPAND);
		outer->Add(fields, 0, wxALL | wxEXPAND, 10);

		m_copy_map = new wxCheckBox(this, wxID_ANY, "Also duplicate the map");
		m_copy_map->SetValue(false);
		outer->Add(m_copy_map, 0, wxLEFT | wxRIGHT, 10);

		auto* map_fields = new wxFlexGridSizer(2, 6, 6);
		map_fields->AddGrowableCol(1, 1);
		m_map_label = new wxStaticText(this, wxID_ANY, "New map name");
		map_fields->Add(m_map_label, 0, wxALIGN_CENTER_VERTICAL);
		m_map_name = new wxTextCtrl(this, wxID_ANY, wxString::FromUTF8(suggested_map));
		m_map_name->SetMaxLength(30);
		map_fields->Add(m_map_name, 1, wxEXPAND);
		outer->Add(map_fields, 0, wxALL | wxEXPAND, 10);

		outer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL), 0, wxALL | wxEXPAND, 10);
		SetSizerAndFit(outer);
		SetMinSize(wxSize(480, -1));
		CentreOnParent();

		UpdateMapFields();
		m_copy_map->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& evt) { UpdateMapFields(); evt.Skip(); });
	}

	std::string GetRoomName() const { return m_name->GetValue().ToStdString(); }
	std::wstring GetDisplayName() const { return m_display->GetValue().ToStdWstring(); }
	bool ShouldCopyMap() const { return m_copy_map->GetValue(); }
	std::string GetMapName() const { return m_map_name->GetValue().ToStdString(); }

private:
	void UpdateMapFields()
	{
		const bool enabled = m_copy_map->GetValue();
		m_map_label->Enable(enabled);
		m_map_name->Enable(enabled);
	}

	wxTextCtrl* m_name;
	wxTextCtrl* m_display;
	wxCheckBox* m_copy_map;
	wxStaticText* m_map_label;
	wxTextCtrl* m_map_name;
};

// One line per kind of reference, for the delete confirmation.
wxString DescribeReferences(const Landstalker::GameData::RoomReferences& refs)
{
	wxString description;
	const auto add = [&](std::size_t count, const char* singular, const char* plural)
	{
		if (count == 0)
		{
			return;
		}
		description += wxString::Format("  %d %s\n", static_cast<int>(count), count == 1 ? singular : plural);
	};
	add(refs.warps, "warp", "warps");
	add(refs.fall_climb_routes, "fall / climb route", "fall / climb routes");
	add(refs.transitions, "transition", "transitions");
	add(refs.entities, "entity", "entities");
	add(refs.flags, "flag", "flags");
	add(refs.chests, "chest", "chests");
	add(refs.shops, "shop entry", "shop entries");
	add(refs.constants, "room constant", "room constants");
	return description;
}

}

RoomManagerDialog::RoomManagerDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd, uint16_t roomnum,
	InitialAction initial_action)
	: wxDialog(parent, wxID_ANY, "Rooms", wxDefaultPosition, { 720, 460 }),
	  m_gd(gd),
	  m_changed(false),
	  m_room_to_open(-1),
	  m_previewed_room(-1),
	  m_room_list(nullptr),
	  m_preview(nullptr),
	  m_preview_message(nullptr),
	  m_detail_map(nullptr),
	  m_detail_tileset(nullptr),
	  m_detail_blocksets(nullptr),
	  m_detail_palette(nullptr),
	  m_detail_heights(nullptr),
	  m_detail_bgm(nullptr),
	  m_detail_entities(nullptr),
	  m_detail_warps(nullptr),
	  m_add(nullptr),
	  m_duplicate(nullptr),
	  m_import(nullptr),
	  m_export(nullptr),
	  m_delete(nullptr),
	  m_move_up(nullptr),
	  m_move_down(nullptr),
	  m_rename(nullptr),
	  m_close(nullptr)
{
	auto* outer = new wxBoxSizer(wxVERTICAL);
	SetSizer(outer);

	auto* panes = new wxBoxSizer(wxHORIZONTAL);
	outer->Add(panes, 1, wxEXPAND, 5);

	auto* room_box = new wxStaticBoxSizer(wxVERTICAL, this, "Rooms");
	m_room_list = new wxListBox(room_box->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE);
	m_room_list->SetMinSize(LIST_MIN_SIZE);
	m_room_list->SetToolTip("Double-click a room to close this dialog and open it.");
	room_box->Add(m_room_list, 1, wxALL | wxEXPAND, 5);
	panes->Add(room_box, 1, wxALL | wxEXPAND, 5);

	auto* right = new wxBoxSizer(wxVERTICAL);
	panes->Add(right, 1, wxEXPAND, 0);

	auto* detail_box = new wxStaticBoxSizer(wxVERTICAL, this, "Selected Room");
	auto* details = new wxFlexGridSizer(2, 4, 12);
	details->AddGrowableCol(1, 1);
	auto add_detail = [&](const char* label)
	{
		details->Add(new wxStaticText(detail_box->GetStaticBox(), wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
		// Ellipsize rather than let a long blockset list stretch the whole dialog.
		auto* value = new wxStaticText(detail_box->GetStaticBox(), wxID_ANY, wxEmptyString,
			wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
		value->SetMinSize(wxSize(160, -1));
		details->Add(value, 1, wxEXPAND);
		return value;
	};
	m_detail_map = add_detail("Map");
	m_detail_tileset = add_detail("Tileset");
	m_detail_blocksets = add_detail("Blocksets");
	m_detail_palette = add_detail("Palette");
	m_detail_heights = add_detail("Floor / ceiling");
	m_detail_bgm = add_detail("BGM");
	m_detail_entities = add_detail("Entities");
	m_detail_warps = add_detail("Warps");
	detail_box->Add(details, 0, wxALL | wxEXPAND, 5);
	detail_box->AddStretchSpacer();
	right->Add(detail_box, 1, wxALL | wxEXPAND, 5);

	auto* preview_box = new wxStaticBoxSizer(wxVERTICAL, this, "Preview");
	// The bitmap is always exactly PREVIEW_SIZE - the room is letterboxed into it - so the
	// pane never resizes as you click between rooms of different shapes.
	m_preview = new wxStaticBitmap(preview_box->GetStaticBox(), wxID_ANY, wxNullBitmap);
	m_preview->SetMinSize(PREVIEW_SIZE);
	m_preview_message = new wxStaticText(preview_box->GetStaticBox(), wxID_ANY, wxEmptyString,
		wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
	preview_box->Add(m_preview, 1, wxALL | wxALIGN_CENTER, 5);
	preview_box->Add(m_preview_message, 0, wxALL | wxALIGN_CENTER, 5);
	right->Add(preview_box, 0, wxALL | wxEXPAND, 5);

	auto* buttons = new wxBoxSizer(wxHORIZONTAL);
	outer->Add(buttons, 0, wxALL | wxEXPAND, 5);

	m_add = new wxButton(this, wxID_ANY, "Add...");
	m_duplicate = new wxButton(this, wxID_ANY, "Duplicate...");
	m_import = new wxButton(this, wxID_ANY, "Import...");
	m_export = new wxButton(this, wxID_ANY, "Export...");
	m_delete = new wxButton(this, wxID_ANY, "Delete");
	m_move_up = new wxButton(this, wxID_ANY, "Move Up");
	m_move_down = new wxButton(this, wxID_ANY, "Move Down");
	m_rename = new wxButton(this, wxID_ANY, "Rename...");
	m_close = new wxButton(this, wxID_CANCEL, "Close");
	for (auto* button : { m_add, m_duplicate, m_import, m_export, m_delete, m_move_up, m_move_down, m_rename })
	{
		buttons->Add(button, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 5);
	}
	buttons->AddStretchSpacer();
	buttons->Add(m_close, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 5);
	m_close->SetDefault();

	GetSizer()->Fit(this);
	SetMinSize(GetSize());
	CentreOnParent(wxBOTH);

	PopulateRoomList(roomnum < m_gd->GetRoomData()->GetRoomCount() ? roomnum : 0);

	m_room_list->Connect(wxEVT_LISTBOX, wxCommandEventHandler(RoomManagerDialog::OnRoomSelected), nullptr, this);
	m_room_list->Connect(wxEVT_LISTBOX_DCLICK, wxCommandEventHandler(RoomManagerDialog::OnRoomActivated), nullptr, this);
	m_add->Connect(wxEVT_BUTTON, wxCommandEventHandler(RoomManagerDialog::OnAdd), nullptr, this);
	m_duplicate->Connect(wxEVT_BUTTON, wxCommandEventHandler(RoomManagerDialog::OnDuplicate), nullptr, this);
	m_import->Connect(wxEVT_BUTTON, wxCommandEventHandler(RoomManagerDialog::OnImport), nullptr, this);
	m_export->Connect(wxEVT_BUTTON, wxCommandEventHandler(RoomManagerDialog::OnExport), nullptr, this);
	m_delete->Connect(wxEVT_BUTTON, wxCommandEventHandler(RoomManagerDialog::OnDelete), nullptr, this);
	m_move_up->Connect(wxEVT_BUTTON, wxCommandEventHandler(RoomManagerDialog::OnMoveUp), nullptr, this);
	m_move_down->Connect(wxEVT_BUTTON, wxCommandEventHandler(RoomManagerDialog::OnMoveDown), nullptr, this);
	m_rename->Connect(wxEVT_BUTTON, wxCommandEventHandler(RoomManagerDialog::OnRename), nullptr, this);

	if (initial_action != InitialAction::NONE)
	{
		// Deferred so the operation's own dialogs open over a fully shown manager.
		CallAfter([this, initial_action]() { RunInitialAction(initial_action); });
	}
}

void RoomManagerDialog::RunInitialAction(InitialAction action)
{
	wxCommandEvent dummy;
	if (action == InitialAction::ADD)
	{
		OnAdd(dummy);
		if (m_changed)
		{
			// Close and hand back the new room, as if it had been double-clicked.
			m_room_to_open = GetSelectedRoom();
			EndModal(wxID_OK);
		}
	}
	else if (action == InitialAction::REMOVE)
	{
		OnDelete(dummy);
		if (m_changed)
		{
			EndModal(wxID_OK);
		}
	}
}

RoomManagerDialog::~RoomManagerDialog()
{
	m_room_list->Disconnect(wxEVT_LISTBOX, wxCommandEventHandler(RoomManagerDialog::OnRoomSelected), nullptr, this);
	m_room_list->Disconnect(wxEVT_LISTBOX_DCLICK, wxCommandEventHandler(RoomManagerDialog::OnRoomActivated), nullptr, this);
	m_add->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(RoomManagerDialog::OnAdd), nullptr, this);
	m_duplicate->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(RoomManagerDialog::OnDuplicate), nullptr, this);
	m_import->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(RoomManagerDialog::OnImport), nullptr, this);
	m_export->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(RoomManagerDialog::OnExport), nullptr, this);
	m_delete->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(RoomManagerDialog::OnDelete), nullptr, this);
	m_move_up->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(RoomManagerDialog::OnMoveUp), nullptr, this);
	m_move_down->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(RoomManagerDialog::OnMoveDown), nullptr, this);
	m_rename->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(RoomManagerDialog::OnRename), nullptr, this);
}

wxString RoomManagerDialog::RoomListLabel(uint16_t room) const
{
	const auto entry = m_gd->GetRoomData()->GetRoom(room);
	const auto internal = wxString::FromUTF8(entry->name);
	const auto display = entry->GetDisplayName();
	// With no label set, GetDisplayName falls back to a generated name; showing that
	// alongside the internal name would just be noise.
	if (!Landstalker::Labels::Exists(Landstalker::Labels::C_ROOMS, room))
	{
		return wxString::Format("[%03d] ", room) + internal;
	}
	return wxString::Format("[%03d] ", room) + internal + " (" + wxString(display) + ")";
}

void RoomManagerDialog::PopulateRoomList(int select)
{
	const auto room_data = m_gd->GetRoomData();
	const auto count = room_data->GetRoomCount();
	m_room_list->Freeze();
	m_room_list->Clear();
	for (std::size_t i = 0; i < count; ++i)
	{
		m_room_list->Append(RoomListLabel(static_cast<uint16_t>(i)));
	}
	m_room_list->Thaw();

	if (count > 0)
	{
		const int selection = std::clamp(select, 0, static_cast<int>(count) - 1);
		m_room_list->SetSelection(selection);
		m_room_list->EnsureVisible(selection);
	}
	// The list was rebuilt, so whatever the preview was showing may now be a different
	// room at the same number.
	m_previewed_room = -1;
	PopulateDetails();
	PopulatePreview();
	UpdateUI();
}

int RoomManagerDialog::GetSelectedRoom() const
{
	const int index = m_room_list->GetSelection();
	if (index == wxNOT_FOUND || index >= static_cast<int>(m_gd->GetRoomData()->GetRoomCount()))
	{
		return -1;
	}
	return index;
}

std::size_t RoomManagerDialog::CountOtherRoomsUsingMap(uint16_t room) const
{
	const auto room_data = m_gd->GetRoomData();
	const auto& map = room_data->GetRoom(room)->map;
	std::size_t count = 0;
	const auto& rooms = room_data->GetRoomlist();
	for (std::size_t i = 0; i < rooms.size(); ++i)
	{
		if (i != room && rooms[i]->map == map)
		{
			++count;
		}
	}
	return count;
}

void RoomManagerDialog::PopulateDetails()
{
	const auto unknown = wxString("-");
	const int room = GetSelectedRoom();
	if (room < 0)
	{
		for (auto* field : { m_detail_map, m_detail_tileset, m_detail_blocksets, m_detail_palette,
			m_detail_heights, m_detail_bgm, m_detail_entities, m_detail_warps })
		{
			field->SetLabel(unknown);
		}
		Layout();
		return;
	}

	const auto roomnum = static_cast<uint16_t>(room);
	const auto room_data = m_gd->GetRoomData();
	const auto entry = room_data->GetRoom(roomnum);

	m_detail_map->SetLabel(wxString(room_data->GetMapDisplayName(entry->map)));
	const auto tileset = room_data->GetTilesetForRoom(roomnum);
	m_detail_tileset->SetLabel(tileset ? wxString::FromUTF8(tileset->GetName()) : unknown);

	std::string blocksets;
	for (const auto& blockset : room_data->GetBlocksetsForRoom(roomnum))
	{
		if (!blockset)
		{
			continue;
		}
		if (!blocksets.empty())
		{
			blocksets += ", ";
		}
		blocksets += blockset->GetName();
	}
	m_detail_blocksets->SetLabel(blocksets.empty() ? unknown : wxString::FromUTF8(blocksets));

	const auto palette = room_data->GetPaletteForRoom(roomnum);
	m_detail_palette->SetLabel(palette ? wxString::FromUTF8(palette->GetName()) : unknown);
	m_detail_heights->SetLabel(wxString::Format("%d / %d", entry->room_z_begin, entry->room_z_end));
	m_detail_bgm->SetLabel(Landstalker::Labels::Get(Landstalker::Labels::C_BGMS, entry->bgm)
		.value_or(Landstalker::StrWPrintf(L"[%02X] Track %d", entry->bgm, entry->bgm)));

	const auto sprite_data = m_gd->GetSpriteData();
	m_detail_entities->SetLabel(sprite_data
		? wxString::Format("%d", static_cast<int>(sprite_data->GetRoomEntities(roomnum).size()))
		: unknown);
	m_detail_warps->SetLabel(wxString::Format("%d",
		static_cast<int>(room_data->GetWarpsForRoom(roomnum).size())));
	Layout();
}

void RoomManagerDialog::PopulatePreview()
{
	const int room = GetSelectedRoom();
	if (room == m_previewed_room)
	{
		return;
	}
	m_previewed_room = room;

	const auto show_message = [this](const wxString& message)
	{
		m_preview->SetBitmap(wxNullBitmap);
		m_preview->Hide();
		m_preview_message->SetLabel(message);
		m_preview_message->Show();
		Layout();
	};

	if (room < 0)
	{
		show_message("No room selected.");
		return;
	}

	const auto room_data = m_gd->GetRoomData();
	const auto roomnum = static_cast<uint16_t>(room);
	const auto map_entry = room_data->GetMapForRoom(roomnum);
	const auto tileset_entry = room_data->GetTilesetForRoom(roomnum);
	const auto palette_entry = room_data->GetPaletteForRoom(roomnum);
	const auto blockset = room_data->GetCombinedBlocksetForRoom(roomnum);
	if (!map_entry || !map_entry->GetData() || !tileset_entry || !palette_entry || !blockset)
	{
		show_message("Preview unavailable.");
		return;
	}

	const auto map = map_entry->GetData();
	const int width = static_cast<int>(map->GetPixelWidth());
	const int height = static_cast<int>(map->GetPixelHeight());
	if (width <= 0 || height <= 0)
	{
		show_message("Preview unavailable.");
		return;
	}

	const std::vector<std::shared_ptr<Landstalker::Palette>> palette{ palette_entry->GetData() };
	const auto tileset = tileset_entry->GetData();
	ImageBufferWx buffer(width, height);
	buffer.Insert3DMapLayer(0, 0, 0, Landstalker::Tilemap3D::Layer::BG, map, tileset, blockset);
	buffer.Insert3DMapLayer(0, 0, 0, Landstalker::Tilemap3D::Layer::FG, map, tileset, blockset);

	wxImage image = buffer.MakeImage(palette);
	// Rooms run to a few thousand pixels across, so only ever scale down, and keep the
	// aspect ratio so the isometric geometry still reads correctly.
	const double scale = std::min({ 1.0,
		static_cast<double>(PREVIEW_SIZE.GetWidth()) / width,
		static_cast<double>(PREVIEW_SIZE.GetHeight()) / height });
	if (scale < 1.0)
	{
		image.Rescale(std::max(1, static_cast<int>(width * scale)),
			std::max(1, static_cast<int>(height * scale)), wxIMAGE_QUALITY_NORMAL);
	}

	wxImage canvas(PREVIEW_SIZE.GetWidth(), PREVIEW_SIZE.GetHeight());
	canvas.SetRGB(wxRect(0, 0, canvas.GetWidth(), canvas.GetHeight()), 0, 0, 0);
	canvas.Paste(image, (canvas.GetWidth() - image.GetWidth()) / 2,
		(canvas.GetHeight() - image.GetHeight()) / 2);

	m_preview_message->Hide();
	m_preview->SetBitmap(wxBitmap(canvas));
	m_preview->Show();
	Layout();
}

void RoomManagerDialog::UpdateUI()
{
	const int room = GetSelectedRoom();
	const int count = static_cast<int>(m_gd->GetRoomData()->GetRoomCount());
	m_export->Enable(room >= 0);
	m_import->Enable(room >= 0);
	m_rename->Enable(room >= 0);
	// The room list must stay dense and non-empty: every room-indexed table in the game
	// would otherwise read a gap as a real room.
	m_delete->Enable(room >= 0 && count > 1);
	// The warp table packs a room number into 10 bits, so 1024 is a hard ceiling.
	const bool has_space = count < static_cast<int>(Landstalker::RoomData::MAX_ROOMS);
	m_add->Enable(has_space);
	m_duplicate->Enable(has_space && room >= 0);
	m_move_up->Enable(room > 0);
	m_move_down->Enable(room >= 0 && room < count - 1);
}

void RoomManagerDialog::Move(int delta)
{
	const int room = GetSelectedRoom();
	if (room < 0)
	{
		return;
	}
	const int new_index = room + delta;
	if (new_index < 0 || new_index >= static_cast<int>(m_gd->GetRoomData()->GetRoomCount()))
	{
		return;
	}
	if (!m_gd->MoveRoom(static_cast<uint16_t>(room), static_cast<uint16_t>(new_index)))
	{
		wxMessageBox("Unable to move the selected room.", "Move Room", wxOK | wxICON_ERROR, this);
		return;
	}
	m_changed = true;
	PopulateRoomList(new_index);
}

void RoomManagerDialog::OnRoomSelected(wxCommandEvent& /*evt*/)
{
	PopulateDetails();
	PopulatePreview();
	UpdateUI();
}

void RoomManagerDialog::OnRoomActivated(wxCommandEvent& /*evt*/)
{
	const int room = GetSelectedRoom();
	if (room < 0)
	{
		return;
	}
	m_room_to_open = room;
	EndModal(wxID_OK);
}

void RoomManagerDialog::OnAdd(wxCommandEvent& /*evt*/)
{
	// Read the selection before the dialog runs: creating a map from inside it does not
	// disturb the room list, but taking it up front keeps the intent obvious.
	const int selection = GetSelectedRoom();
	NewRoomDialog dialog(this, m_gd);
	const bool accepted = dialog.ShowModal() == wxID_OK;
	// The New Map button creates its map straight away, so a cancelled dialog can still
	// have changed the project.
	m_changed = m_changed || dialog.MapsChanged();
	if (!accepted)
	{
		if (dialog.MapsChanged())
		{
			PopulateRoomList(GetSelectedRoom());
		}
		return;
	}

	const auto room = m_gd->AddRoom(dialog.GetMap(), dialog.GetRoomName(), dialog.GetDisplayName(),
		dialog.GetTileset(), dialog.GetPalette(), dialog.GetPrimaryBlockset(),
		dialog.GetSecondaryBlockset(), dialog.GetFloorHeight(), dialog.GetCeilingHeight(),
		dialog.GetBgm());
	if (!room)
	{
		wxMessageBox("Unable to add the room. The game may have run out of room visit flags, "
			"or the chosen combination of tileset and blocksets may not exist.",
			"Add Room", wxOK | wxICON_ERROR, this);
		PopulateRoomList(GetSelectedRoom());
		return;
	}
	m_changed = true;
	PopulateRoomList(PlaceAfterSelection(room->index, selection));
}

int RoomManagerDialog::PlaceAfterSelection(uint16_t added, int selection)
{
	// GameData::AddRoom can only append, so a new room always arrives at the end. Move it
	// to sit directly below whatever was selected. This renumbers every room in between,
	// which MoveRoom handles by rewriting all their references.
	if (selection < 0 || selection >= static_cast<int>(added))
	{
		return static_cast<int>(added);
	}
	const int target = selection + 1;
	if (target == static_cast<int>(added) || !m_gd->MoveRoom(added, static_cast<uint16_t>(target)))
	{
		return static_cast<int>(added);
	}
	return target;
}

std::string RoomManagerDialog::SuggestRoomCopyName(const std::string& source) const
{
	const auto room_data = m_gd->GetRoomData();
	// Truncate before appending, so repeatedly copying a long name cannot creep past the
	// 30 character limit and start failing validation.
	const std::string base = source.size() > 24 ? source.substr(0, 24) : source;
	for (unsigned int i = 1; ; ++i)
	{
		const auto candidate = i == 1 ? base + "Copy" : base + "Copy" + std::to_string(i);
		if (room_data->GetRoom(candidate) == nullptr)
		{
			return candidate;
		}
	}
}

void RoomManagerDialog::OnDuplicate(wxCommandEvent& /*evt*/)
{
	const int room = GetSelectedRoom();
	if (room < 0)
	{
		return;
	}
	const auto source_num = static_cast<uint16_t>(room);
	const auto room_data = m_gd->GetRoomData();
	const auto source = room_data->GetRoom(source_num);
	// Copy the fields out now: the shared_ptr stays valid, but reading them after the room
	// list has grown is easy to get wrong.
	const auto source_name = source->name;
	const auto source_map = source->map;
	const auto tileset = source->tileset;
	const auto palette = source->room_palette;
	const auto pri_blockset = source->pri_blockset;
	const auto sec_blockset = source->sec_blockset;
	const auto z_begin = source->room_z_begin;
	const auto z_end = source->room_z_end;
	const auto bgm = source->bgm;

	DuplicateRoomDialog dialog(this, wxString(source->GetDisplayName()),
		SuggestRoomCopyName(source_name), SuggestMapCopyName(*room_data, source_map));
	while (dialog.ShowModal() == wxID_OK)
	{
		const auto name = dialog.GetRoomName();
		const auto display = dialog.GetDisplayName();
		const auto copy_map = dialog.ShouldCopyMap();
		const auto map_name = dialog.GetMapName();

		// Validate everything before creating anything, so a bad map name cannot leave a
		// duplicated map behind with no room using it.
		if (!Landstalker::RoomData::IsValidRoomName(name) || room_data->GetRoom(name) != nullptr)
		{
			wxMessageBox("The room's internal name must be unique, start with a letter, only contain "
				"A-Z, a-z, 0-9 and _, and be at most 30 characters.",
				"Duplicate Room", wxOK | wxICON_ERROR, this);
			continue;
		}
		if (!display.empty() && !Landstalker::Labels::IsValid(display))
		{
			wxMessageBox("The display name must be unique and must not contain non-printable characters.",
				"Duplicate Room", wxOK | wxICON_ERROR, this);
			continue;
		}
		if (copy_map && (!Landstalker::RoomData::IsValidMapName(map_name) ||
			room_data->GetMaps().count(map_name) != 0))
		{
			wxMessageBox("The map's name must be unique, start with a letter, only contain "
				"A-Z, a-z, 0-9 and _, and be at most 30 characters.",
				"Duplicate Room", wxOK | wxICON_ERROR, this);
			continue;
		}

		if (copy_map && !DuplicateMap(m_gd, source_map, map_name))
		{
			wxMessageBox("Unable to duplicate the map.", "Duplicate Room", wxOK | wxICON_ERROR, this);
			return;
		}
		const auto copy = m_gd->AddRoom(copy_map ? map_name : source_map, name, display,
			tileset, palette, pri_blockset, sec_blockset, z_begin, z_end, bgm);
		if (!copy)
		{
			// Take the map back out: nothing references it, so this always succeeds, and
			// leaving it would be an orphan the user never asked for.
			if (copy_map)
			{
				room_data->DeleteMap(map_name);
			}
			wxMessageBox("Unable to duplicate the room. The game may have run out of room visit flags.",
				"Duplicate Room", wxOK | wxICON_ERROR, this);
			return;
		}

		// Entities belong wholly to the room, so they come across. Everything else either
		// names another room or is numbered by position - see the dialog's own warning.
		const auto sprite_data = m_gd->GetSpriteData();
		if (sprite_data)
		{
			sprite_data->SetRoomEntities(copy->index, sprite_data->GetRoomEntities(source_num));
		}

		m_changed = true;
		PopulateRoomList(PlaceAfterSelection(copy->index, room));
		return;
	}
}

bool RoomManagerDialog::ConfirmDelete(uint16_t room, bool& delete_map)
{
	delete_map = false;
	const auto room_data = m_gd->GetRoomData();
	const auto name = wxString(room_data->GetRoom(room)->GetDisplayName());

	// 1. The operation itself cannot be undone.
	if (wxMessageBox("Delete room '" + name + "'?\n\n"
		"This cannot be undone. Every room above it moves down one place, and all room "
		"numbers stored anywhere in the project are renumbered to follow. Room numbers "
		"written by hand outside the project - patches, external tools - are not updated.",
		"Delete Room", wxYES_NO | wxNO_DEFAULT | wxICON_WARNING, this) != wxYES)
	{
		return false;
	}

	// 2. Anything still pointing at the room goes with it.
	if (m_gd->IsRoomReferenced(room))
	{
		const auto refs = m_gd->CountRoomReferences(room);
		if (wxMessageBox("'" + name + "' is still referenced by:\n\n" + DescribeReferences(refs) +
			"\nDeleting the room destroys all of these. A room constant naming it is kept but "
			"reset to room 0, since game code refers to those by name.\n\nContinue?",
			"Delete Room", wxYES_NO | wxNO_DEFAULT | wxICON_WARNING, this) != wxYES)
		{
			return false;
		}
	}

	// 3. A map left with no rooms cannot be drawn or previewed, so offer to take it too.
	if (CountOtherRoomsUsingMap(room) == 0)
	{
		const auto map = room_data->GetRoom(room)->map;
		delete_map = wxMessageBox("No other room draws map '" + wxString::FromUTF8(map) + "'.\n\n"
			"Delete the map as well?",
			"Delete Room", wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION, this) == wxYES;
	}
	return true;
}

void RoomManagerDialog::OnDelete(wxCommandEvent& /*evt*/)
{
	const int room = GetSelectedRoom();
	if (room < 0 || m_gd->GetRoomData()->GetRoomCount() < 2)
	{
		return;
	}
	const auto roomnum = static_cast<uint16_t>(room);

	bool delete_map = false;
	if (!ConfirmDelete(roomnum, delete_map))
	{
		return;
	}
	// Read the map name before the room goes, or there is nothing left to look it up on.
	const auto map = m_gd->GetRoomData()->GetRoom(roomnum)->map;

	if (!m_gd->DeleteRoom(roomnum))
	{
		wxMessageBox("Unable to delete the selected room.", "Delete Room", wxOK | wxICON_ERROR, this);
		return;
	}
	m_changed = true;
	if (delete_map && !m_gd->GetRoomData()->DeleteMap(map))
	{
		wxMessageBox("The room was deleted, but its map could not be removed.",
			"Delete Room", wxOK | wxICON_WARNING, this);
	}
	PopulateRoomList(room);
}

void RoomManagerDialog::OnMoveUp(wxCommandEvent& /*evt*/)
{
	Move(-1);
}

void RoomManagerDialog::OnMoveDown(wxCommandEvent& /*evt*/)
{
	Move(1);
}

void RoomManagerDialog::OnRename(wxCommandEvent& /*evt*/)
{
	const int room = GetSelectedRoom();
	if (room < 0)
	{
		return;
	}
	const auto roomnum = static_cast<uint16_t>(room);
	const auto room_data = m_gd->GetRoomData();
	const auto old_name = room_data->GetRoom(roomnum)->name;
	const auto old_display = room_data->GetRoom(roomnum)->GetDisplayName();

	RenameRoomDialog dialog(this, old_name, old_display);
	while (dialog.ShowModal() == wxID_OK)
	{
		const auto name = dialog.GetInternalName();
		const auto display = dialog.GetDisplayName();
		const bool name_changed = name != old_name;
		const bool display_changed = display != old_display;
		if (!name_changed && !display_changed)
		{
			return;
		}

		// Check both names before changing either, so a bad display name cannot leave the
		// internal rename half-applied.
		if (name_changed &&
			(!Landstalker::RoomData::IsValidRoomName(name) || room_data->GetRoom(name) != nullptr))
		{
			wxMessageBox("The internal name must be unique, start with a letter, only contain "
				"A-Z, a-z, 0-9 and _, and be at most 30 characters.",
				"Rename Room", wxOK | wxICON_ERROR, this);
			continue;
		}
		if (display_changed && !Landstalker::Labels::IsValid(display, Landstalker::Labels::C_ROOMS, room))
		{
			wxMessageBox("The display name must not be empty, must be unique, and must not contain "
				"non-printable characters.", "Rename Room", wxOK | wxICON_ERROR, this);
			continue;
		}

		if (name_changed && !room_data->RenameRoom(roomnum, name))
		{
			wxMessageBox("Unable to rename the selected room.", "Rename Room", wxOK | wxICON_ERROR, this);
			return;
		}
		if (display_changed)
		{
			Landstalker::Labels::Update(Landstalker::Labels::C_ROOMS, room, display);
		}
		m_changed = true;
		PopulateRoomList(room);
		return;
	}
}

void RoomManagerDialog::OnExport(wxCommandEvent& /*evt*/)
{
	const int room = GetSelectedRoom();
	if (room < 0)
	{
		return;
	}
	const auto roomnum = static_cast<uint16_t>(room);
	const auto entry = m_gd->GetRoomData()->GetRoom(roomnum);

	wxFileDialog fd(this, _("Export Room"), "", wxString::FromUTF8(entry->name + ".yaml"),
		"Room Metadata (*.yaml)|*.yaml;*.yml|"
		"Tiled TMX Tilemap (*.tmx)|*.tmx",
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return;
	}

	const std::filesystem::path chosen(fd.GetPath().ToStdString());
	try
	{
		if (ToLower(chosen.extension().string()) == ".tmx")
		{
			// Tiled draws the room through a blockset image, written alongside the TMX under
			// the name the room's own tileset, blocksets and palette give it.
			const auto blockset_name = Landstalker::StrPrintf("BT%02d_%01d%01d_p%02d.png",
				entry->tileset + 1, entry->pri_blockset, entry->sec_blockset + 1, entry->room_palette + 1);
			const auto blockset_path = (chosen.parent_path() / blockset_name).string();
			MapFileIo::RenderBlocksetPng(blockset_path, m_gd, roomnum);
			if (!Landstalker::RoomToTmx::ExportToTmx(chosen.string(), roomnum, m_gd, blockset_path))
			{
				wxMessageBox("Unable to write the room to the selected location.",
					"Export Room", wxOK | wxICON_ERROR, this);
			}
			return;
		}
		if (!Landstalker::RoomToYaml::ExportToYaml(chosen.string(), roomnum, m_gd))
		{
			wxMessageBox("Unable to write the room to the selected location.",
				"Export Room", wxOK | wxICON_ERROR, this);
		}
	}
	catch (const std::exception& e)
	{
		wxMessageBox("Unable to export the room:\n" + wxString::FromUTF8(e.what()),
			"Export Room", wxOK | wxICON_ERROR, this);
	}
}

void RoomManagerDialog::OnImport(wxCommandEvent& /*evt*/)
{
	const int room = GetSelectedRoom();
	if (room < 0)
	{
		return;
	}
	const auto roomnum = static_cast<uint16_t>(room);

	wxFileDialog fd(this, _("Import Room"), "",
		wxString::FromUTF8(m_gd->GetRoomData()->GetRoom(roomnum)->name),
		"All Supported Formats|*.yaml;*.yml;*.tmx|"
		"Room Metadata (*.yaml)|*.yaml;*.yml|"
		"Tiled TMX Tilemap (*.tmx)|*.tmx|"
		"All Files (*.*)|*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return;
	}

	const std::filesystem::path chosen(fd.GetPath().ToStdString());
	const bool is_tmx = ToLower(chosen.extension().string()) == ".tmx";
	// A TMX carries both the room settings and the tilemap. Importing the tilemap
	// overwrites the shared map, which every other room drawing it will see too.
	bool import_map = false;
	if (is_tmx)
	{
		const auto shared_with = CountOtherRoomsUsingMap(roomnum);
		wxString question = "Import the map data from this file as well as the room settings?";
		if (shared_with > 0)
		{
			question += wxString::Format("\n\n%d other room%s draw%s this map and would change too.",
				static_cast<int>(shared_with), shared_with == 1 ? "" : "s", shared_with == 1 ? "s" : "");
		}
		const int answer = wxMessageBox(question, "Import Room",
			wxYES_NO | wxCANCEL | wxICON_QUESTION, this);
		if (answer == wxCANCEL)
		{
			return;
		}
		import_map = answer == wxYES;
	}

	try
	{
		const Landstalker::RoomToYaml::RoomKey key{ roomnum };
		if (is_tmx)
		{
			if (!Landstalker::RoomToTmx::ImportFromTmx(chosen.string(), key, m_gd))
			{
				wxMessageBox("Unable to read room data from the selected file.",
					"Import Room", wxOK | wxICON_ERROR, this);
				return;
			}
			// Re-fetch the map: the room settings just imported may have repointed it.
			if (import_map)
			{
				const auto map = m_gd->GetRoomData()->GetMapForRoom(roomnum);
				if (!map || !map->GetData() ||
					!Landstalker::MapToTmx::ImportFromTmx(chosen.string(), *map->GetData()))
				{
					wxMessageBox("The room settings were imported, but the map data could not be read.",
						"Import Room", wxOK | wxICON_WARNING, this);
				}
			}
		}
		else if (!Landstalker::RoomToYaml::ImportFromYaml(chosen.string(), key, m_gd))
		{
			wxMessageBox("Unable to read room data from the selected file.",
				"Import Room", wxOK | wxICON_ERROR, this);
			return;
		}
	}
	catch (const std::exception& e)
	{
		wxMessageBox("Unable to import the room:\n" + wxString::FromUTF8(e.what()),
			"Import Room", wxOK | wxICON_ERROR, this);
		return;
	}
	m_changed = true;
	PopulateRoomList(room);
}
