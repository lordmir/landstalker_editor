#include <rooms/MapManagerDialog.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>

#include <wx/filedlg.h>
#include <wx/spinctrl.h>
#include <wx/textdlg.h>

#include <landstalker/misc/Labels.h>
#include <landstalker/misc/Utils.h>
#include <main/ImageBufferWx.h>
#include <rooms/MapFileIo.h>
#include <rooms/NewMapDialog.h>

namespace
{

// New and imported maps start out at this size; an import then resizes to suit the file.
constexpr uint8_t DEFAULT_MAP_SIZE = 16;

// Both lists get the same minimum so neither can be widened by its contents.
const wxSize LIST_MIN_SIZE(240, 110);
// Maps are rendered at full size then scaled to fit inside this.
const wxSize PREVIEW_SIZE(300, 150);

// The CSV exporter writes one file per layer, named after the map.
constexpr std::array<const char*, 3> CSV_SUFFIXES = { "_background", "_foreground", "_heightmap" };
constexpr std::array<const char*, 3> CSV_PROMPTS = { "Import Background Layer", "Import Foreground Layer", "Import Heightmap Data" };

std::string ToLower(const std::string& str)
{
	std::string result = str;
	std::transform(result.begin(), result.end(), result.begin(), [](const unsigned char c)
	{
		return static_cast<char>(std::tolower(c));
	});
	return result;
}

// Prompts for both of a map's names: the assembly label the disassembly is built around,
// and the friendlier label the editor shows. The two are independent - the internal name
// lives in the room data, the display name in the label table.
class RenameMapDialog : public wxDialog
{
public:
	RenameMapDialog(wxWindow* parent, const std::string& internal_name, const std::wstring& display_name)
		: wxDialog(parent, wxID_ANY, "Rename Map")
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

// A CSV import needs all three layer files. Work out which layer the chosen file is,
// pick up its siblings automatically, and ask for whatever is left over.
bool ResolveCsvSet(wxWindow* parent, const std::filesystem::path& chosen, std::array<std::string, 3>& paths)
{
	const auto directory = chosen.parent_path();
	const auto extension = chosen.extension().string();
	const auto stem = chosen.stem().string();

	std::string base = stem;
	std::size_t chosen_layer = 0;
	bool layer_known = false;
	for (std::size_t i = 0; i < CSV_SUFFIXES.size(); ++i)
	{
		const std::string suffix = CSV_SUFFIXES[i];
		if (stem.size() > suffix.size() &&
			ToLower(stem.substr(stem.size() - suffix.size())) == suffix)
		{
			base = stem.substr(0, stem.size() - suffix.size());
			chosen_layer = i;
			layer_known = true;
			break;
		}
	}

	paths.fill(std::string());
	paths[chosen_layer] = chosen.string();
	for (std::size_t i = 0; i < paths.size(); ++i)
	{
		if (i == chosen_layer)
		{
			continue;
		}
		if (layer_known)
		{
			const auto sibling = directory / (base + CSV_SUFFIXES[i] + extension);
			if (std::filesystem::is_regular_file(sibling))
			{
				paths[i] = sibling.string();
				continue;
			}
		}
		wxFileDialog fd(parent, CSV_PROMPTS[i], wxString::FromUTF8(directory.string()), "",
			"CSV File (*.csv)|*.csv|All Files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
		if (fd.ShowModal() == wxID_CANCEL)
		{
			return false;
		}
		paths[i] = fd.GetPath().ToStdString();
	}
	return true;
}

}

MapManagerDialog::MapManagerDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd, uint16_t roomnum)
	: wxDialog(parent, wxID_ANY, "Maps", wxDefaultPosition, { 640, 400 }),
	  m_gd(gd),
	  m_roomnum(roomnum),
	  m_changed(false),
	  m_room_to_open(-1),
	  m_previewed_room(-1),
	  m_map_list(nullptr),
	  m_room_list(nullptr),
	  m_preview(nullptr),
	  m_preview_message(nullptr),
	  m_detail_size(nullptr),
	  m_detail_heightmap(nullptr),
	  m_detail_tileset(nullptr),
	  m_detail_blocksets(nullptr),
	  m_detail_palette(nullptr),
	  m_add(nullptr),
	  m_duplicate(nullptr),
	  m_import(nullptr),
	  m_export(nullptr),
	  m_remove(nullptr),
	  m_move_up(nullptr),
	  m_move_down(nullptr),
	  m_rename(nullptr),
	  m_close(nullptr)
{
	auto* outer = new wxBoxSizer(wxVERTICAL);
	SetSizer(outer);

	// One grid for all four boxes: wxGridSizer forces every cell to the same width and
	// height, so the panes stay evenly proportioned no matter how long their contents are.
	// A box sizer hands out only the surplus space, which lets the busiest pane grow.
	auto* panes = new wxGridSizer(2, 2, 0, 0);
	outer->Add(panes, 1, wxEXPAND, 5);

	auto* map_box = new wxStaticBoxSizer(wxVERTICAL, this, "Maps");
	m_map_list = new wxListBox(map_box->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE);
	m_map_list->SetMinSize(LIST_MIN_SIZE);
	m_map_list->SetToolTip("Double-click a map to close this dialog and open the room previewed alongside it.");
	map_box->Add(m_map_list, 1, wxALL | wxEXPAND, 5);
	panes->Add(map_box, 1, wxALL | wxEXPAND, 5);

	auto* room_box = new wxStaticBoxSizer(wxVERTICAL, this, "Rooms Using Selected Map");
	m_room_list = new wxListBox(room_box->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE);
	m_room_list->SetMinSize(LIST_MIN_SIZE);
	m_room_list->SetToolTip("Double-click a room to close this dialog and open it.");
	room_box->Add(m_room_list, 1, wxALL | wxEXPAND, 5);
	panes->Add(room_box, 1, wxALL | wxEXPAND, 5);

	auto* detail_box = new wxStaticBoxSizer(wxVERTICAL, this, "Selected Map");
	auto* details = new wxFlexGridSizer(2, 4, 12);
	details->AddGrowableCol(1, 1);
	auto add_detail = [&](const char* label)
	{
		details->Add(new wxStaticText(detail_box->GetStaticBox(), wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
		// Ellipsize rather than let a long blockset list stretch the whole dialog.
		auto* value = new wxStaticText(detail_box->GetStaticBox(), wxID_ANY, wxEmptyString,
			wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
		value->SetMinSize(wxSize(140, -1));
		details->Add(value, 1, wxEXPAND);
		return value;
	};
	m_detail_size = add_detail("Map size");
	m_detail_heightmap = add_detail("Heightmap");
	m_detail_tileset = add_detail("Tileset");
	m_detail_blocksets = add_detail("Blocksets");
	m_detail_palette = add_detail("Palette");
	detail_box->Add(details, 0, wxALL | wxEXPAND, 5);
	detail_box->AddStretchSpacer();
	panes->Add(detail_box, 1, wxALL | wxEXPAND, 5);

	auto* preview_box = new wxStaticBoxSizer(wxVERTICAL, this, "Preview");
	// The bitmap is always exactly PREVIEW_SIZE - the map is letterboxed into it - so the
	// pane never resizes as you click between maps of different shapes.
	m_preview = new wxStaticBitmap(preview_box->GetStaticBox(), wxID_ANY, wxNullBitmap);
	m_preview->SetMinSize(PREVIEW_SIZE);
	m_preview_message = new wxStaticText(preview_box->GetStaticBox(), wxID_ANY, wxEmptyString,
		wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
	preview_box->Add(m_preview, 1, wxALL | wxALIGN_CENTER, 5);
	// Proportion 0: a proportional item makes the sizer reserve its own minimum for every
	// share, so letting the message claim an equal share would demand twice the preview
	// height from every pane in the grid.
	preview_box->Add(m_preview_message, 0, wxALL | wxALIGN_CENTER, 5);
	panes->Add(preview_box, 1, wxALL | wxEXPAND, 5);

	auto* buttons = new wxBoxSizer(wxHORIZONTAL);
	outer->Add(buttons, 0, wxALL | wxEXPAND, 5);

	m_add = new wxButton(this, wxID_ANY, "Add...");
	m_duplicate = new wxButton(this, wxID_ANY, "Duplicate...");
	m_import = new wxButton(this, wxID_ANY, "Import...");
	m_export = new wxButton(this, wxID_ANY, "Export...");
	m_remove = new wxButton(this, wxID_ANY, "Remove");
	m_move_up = new wxButton(this, wxID_ANY, "Move Up");
	m_move_down = new wxButton(this, wxID_ANY, "Move Down");
	m_rename = new wxButton(this, wxID_ANY, "Rename...");
	m_close = new wxButton(this, wxID_CANCEL, "Close");
	for (auto* button : { m_add, m_duplicate, m_import, m_export, m_remove, m_move_up, m_move_down, m_rename })
	{
		buttons->Add(button, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 5);
	}
	buttons->AddStretchSpacer();
	buttons->Add(m_close, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 5);
	m_close->SetDefault();

	// Let the grid decide the size rather than guessing pixel values, then stop the dialog
	// being shrunk below it - the four panes are only even while they all fit.
	GetSizer()->Fit(this);
	SetMinSize(GetSize());
	CentreOnParent(wxBOTH);

	const auto room_data = m_gd->GetRoomData();
	const std::string current_map = m_roomnum < room_data->GetRoomCount()
		? room_data->GetRoom(m_roomnum)->map
		: std::string();
	PopulateMapList(current_map);

	m_map_list->Connect(wxEVT_LISTBOX, wxCommandEventHandler(MapManagerDialog::OnMapSelected), nullptr, this);
	m_map_list->Connect(wxEVT_LISTBOX_DCLICK, wxCommandEventHandler(MapManagerDialog::OnRoomActivated), nullptr, this);
	m_room_list->Connect(wxEVT_LISTBOX, wxCommandEventHandler(MapManagerDialog::OnRoomSelected), nullptr, this);
	m_room_list->Connect(wxEVT_LISTBOX_DCLICK, wxCommandEventHandler(MapManagerDialog::OnRoomActivated), nullptr, this);
	m_add->Connect(wxEVT_BUTTON, wxCommandEventHandler(MapManagerDialog::OnAdd), nullptr, this);
	m_duplicate->Connect(wxEVT_BUTTON, wxCommandEventHandler(MapManagerDialog::OnDuplicate), nullptr, this);
	m_import->Connect(wxEVT_BUTTON, wxCommandEventHandler(MapManagerDialog::OnImport), nullptr, this);
	m_export->Connect(wxEVT_BUTTON, wxCommandEventHandler(MapManagerDialog::OnExport), nullptr, this);
	m_remove->Connect(wxEVT_BUTTON, wxCommandEventHandler(MapManagerDialog::OnRemove), nullptr, this);
	m_move_up->Connect(wxEVT_BUTTON, wxCommandEventHandler(MapManagerDialog::OnMoveUp), nullptr, this);
	m_move_down->Connect(wxEVT_BUTTON, wxCommandEventHandler(MapManagerDialog::OnMoveDown), nullptr, this);
	m_rename->Connect(wxEVT_BUTTON, wxCommandEventHandler(MapManagerDialog::OnRename), nullptr, this);
}

MapManagerDialog::~MapManagerDialog()
{
	m_map_list->Disconnect(wxEVT_LISTBOX, wxCommandEventHandler(MapManagerDialog::OnMapSelected), nullptr, this);
	m_map_list->Disconnect(wxEVT_LISTBOX_DCLICK, wxCommandEventHandler(MapManagerDialog::OnRoomActivated), nullptr, this);
	m_room_list->Disconnect(wxEVT_LISTBOX, wxCommandEventHandler(MapManagerDialog::OnRoomSelected), nullptr, this);
	m_room_list->Disconnect(wxEVT_LISTBOX_DCLICK, wxCommandEventHandler(MapManagerDialog::OnRoomActivated), nullptr, this);
	m_add->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(MapManagerDialog::OnAdd), nullptr, this);
	m_duplicate->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(MapManagerDialog::OnDuplicate), nullptr, this);
	m_import->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(MapManagerDialog::OnImport), nullptr, this);
	m_export->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(MapManagerDialog::OnExport), nullptr, this);
	m_remove->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(MapManagerDialog::OnRemove), nullptr, this);
	m_move_up->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(MapManagerDialog::OnMoveUp), nullptr, this);
	m_move_down->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(MapManagerDialog::OnMoveDown), nullptr, this);
	m_rename->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(MapManagerDialog::OnRename), nullptr, this);
}

void MapManagerDialog::PopulateMapList(const std::string& select)
{
	const auto& order = m_gd->GetRoomData()->GetMapOrder();
	m_map_list->Freeze();
	m_map_list->Clear();
	for (const auto& name : order)
	{
		m_map_list->Append(MapListLabel(name));
	}
	m_map_list->Thaw();

	const auto selected = std::find(order.cbegin(), order.cend(), select);
	if (!order.empty())
	{
		m_map_list->SetSelection(selected == order.cend()
			? 0
			: static_cast<int>(std::distance(order.cbegin(), selected)));
	}
	PopulateRoomList();
	PopulateDetails();
	PopulatePreview();
	UpdateUI();
}

void MapManagerDialog::PopulateRoomList()
{
	const auto map = GetSelectedMap();
	const auto room_data = m_gd->GetRoomData();
	m_room_list->Freeze();
	m_room_list->Clear();
	m_listed_rooms.clear();
	int current_room_entry = wxNOT_FOUND;
	if (!map.empty())
	{
		const auto& rooms = room_data->GetRoomlist();
		for (std::size_t i = 0; i < rooms.size(); ++i)
		{
			if (rooms[i]->map != map)
			{
				continue;
			}
			if (i == m_roomnum)
			{
				current_room_entry = static_cast<int>(m_room_list->GetCount());
			}
			m_room_list->Append(wxString(rooms[i]->GetDisplayName()));
			m_listed_rooms.push_back(static_cast<uint16_t>(i));
		}
	}
	m_room_list->Thaw();
	// Fall back to the first room so the preview always has something to draw.
	if (current_room_entry == wxNOT_FOUND && !m_listed_rooms.empty())
	{
		current_room_entry = 0;
	}
	if (current_room_entry != wxNOT_FOUND)
	{
		m_room_list->SetSelection(current_room_entry);
	}
}

int MapManagerDialog::GetSelectedRoom() const
{
	const int index = m_room_list->GetSelection();
	if (index == wxNOT_FOUND || index >= static_cast<int>(m_listed_rooms.size()))
	{
		return -1;
	}
	return static_cast<int>(m_listed_rooms[index]);
}

void MapManagerDialog::PopulatePreview()
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
		// A map with no room has no tileset or palette to draw it with.
		show_message("No room uses this map,\nso it cannot be drawn.");
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
	// Maps run to a few thousand pixels across, so only ever scale down, and keep the
	// aspect ratio so the isometric geometry still reads correctly.
	const double scale = std::min({ 1.0,
		static_cast<double>(PREVIEW_SIZE.GetWidth()) / width,
		static_cast<double>(PREVIEW_SIZE.GetHeight()) / height });
	if (scale < 1.0)
	{
		image.Rescale(std::max(1, static_cast<int>(width * scale)),
			std::max(1, static_cast<int>(height * scale)), wxIMAGE_QUALITY_NORMAL);
	}

	// Letterbox onto a fixed black canvas, so the pane keeps one size and the padding
	// left by the aspect ratio reads as background rather than as the pane resizing.
	wxImage canvas(PREVIEW_SIZE.GetWidth(), PREVIEW_SIZE.GetHeight());
	canvas.SetRGB(wxRect(0, 0, canvas.GetWidth(), canvas.GetHeight()), 0, 0, 0);
	canvas.Paste(image, (canvas.GetWidth() - image.GetWidth()) / 2,
		(canvas.GetHeight() - image.GetHeight()) / 2);

	m_preview_message->Hide();
	m_preview->SetBitmap(wxBitmap(canvas));
	m_preview->Show();
	Layout();
}

void MapManagerDialog::PopulateDetails()
{
	const auto unknown = wxString("-");
	const auto name = GetSelectedMap();
	const auto room_data = m_gd->GetRoomData();
	// GetMap dereferences an end iterator for an unknown name, so look before leaping.
	const auto entry = room_data->GetMaps().count(name) == 0 ? nullptr : room_data->GetMap(name);
	if (!entry || !entry->GetData())
	{
		m_detail_size->SetLabel(unknown);
		m_detail_heightmap->SetLabel(unknown);
		m_detail_tileset->SetLabel(unknown);
		m_detail_blocksets->SetLabel(unknown);
		m_detail_palette->SetLabel(unknown);
		Layout();
		return;
	}

	const auto map = entry->GetData();
	m_detail_size->SetLabel(wxString::Format("%d x %d blocks", map->GetWidth(), map->GetHeight()));
	m_detail_heightmap->SetLabel(wxString::Format("%d x %d cells at offset (%d, %d)",
		map->GetHeightmapWidth(), map->GetHeightmapHeight(), map->GetLeft(), map->GetTop()));

	// The tileset, blocksets and palette belong to the rooms, not the map, so they are only
	// worth reporting when every room drawing this map agrees on them.
	std::string tileset;
	std::string blocksets;
	std::string palette;
	bool first = true;
	bool tileset_varies = false;
	bool blocksets_varies = false;
	bool palette_varies = false;
	const auto& rooms = room_data->GetRoomlist();
	for (std::size_t i = 0; i < rooms.size(); ++i)
	{
		if (rooms[i]->map != name)
		{
			continue;
		}
		const auto tileset_entry = room_data->GetTilesetForRoom(static_cast<uint16_t>(i));
		const std::string room_tileset = tileset_entry ? tileset_entry->GetName() : std::string();
		std::string room_blocksets;
		for (const auto& blockset : room_data->GetBlocksetsForRoom(static_cast<uint16_t>(i)))
		{
			if (!blockset)
			{
				continue;
			}
			if (!room_blocksets.empty())
			{
				room_blocksets += ", ";
			}
			room_blocksets += blockset->GetName();
		}
		const auto palette_entry = room_data->GetPaletteForRoom(static_cast<uint16_t>(i));
		const std::string room_palette = palette_entry ? palette_entry->GetName() : std::string();

		if (first)
		{
			tileset = room_tileset;
			blocksets = room_blocksets;
			palette = room_palette;
			first = false;
		}
		else
		{
			tileset_varies = tileset_varies || room_tileset != tileset;
			blocksets_varies = blocksets_varies || room_blocksets != blocksets;
			palette_varies = palette_varies || room_palette != palette;
		}
	}

	const auto describe = [&](bool varies, const std::string& value)
	{
		if (first)
		{
			return wxString("Unknown (no rooms use this map)");
		}
		if (varies)
		{
			return wxString("Varies between rooms");
		}
		return value.empty() ? unknown : wxString::FromUTF8(value);
	};
	m_detail_tileset->SetLabel(describe(tileset_varies, tileset));
	m_detail_blocksets->SetLabel(describe(blocksets_varies, blocksets));
	m_detail_palette->SetLabel(describe(palette_varies, palette));
	Layout();
}

void MapManagerDialog::UpdateUI()
{
	const int index = GetSelectedMapIndex();
	const int count = static_cast<int>(m_map_list->GetCount());
	m_duplicate->Enable(index != wxNOT_FOUND);
	m_export->Enable(index != wxNOT_FOUND);
	m_remove->Enable(CanDeleteSelectedMap());
	m_rename->Enable(index != wxNOT_FOUND);
	m_move_up->Enable(index > 0);
	m_move_down->Enable(index != wxNOT_FOUND && index < count - 1);
}

int MapManagerDialog::FirstRoomUsing(const std::string& map) const
{
	const auto& rooms = m_gd->GetRoomData()->GetRoomlist();
	for (std::size_t i = 0; i < rooms.size(); ++i)
	{
		if (rooms[i]->map == map)
		{
			return static_cast<int>(i);
		}
	}
	return -1;
}

std::string MapManagerDialog::GetSelectedMap() const
{
	// The list shows decorated labels, so the internal name comes from the map order,
	// which the list is always built from in step.
	const int index = GetSelectedMapIndex();
	const auto& order = m_gd->GetRoomData()->GetMapOrder();
	if (index == wxNOT_FOUND || index >= static_cast<int>(order.size()))
	{
		return std::string();
	}
	return order[index];
}

wxString MapManagerDialog::MapListLabel(const std::string& name) const
{
	const auto internal = wxString::FromUTF8(name);
	const auto display = m_gd->GetRoomData()->GetMapDisplayName(name);
	// With no label set, GetMapDisplayName echoes the internal name back; showing it twice
	// would just be noise.
	if (display == std::wstring(name.cbegin(), name.cend()))
	{
		return internal;
	}
	return internal + " (" + wxString(display) + ")";
}

int MapManagerDialog::GetSelectedMapIndex() const
{
	return m_map_list->GetSelection();
}

bool MapManagerDialog::CanDeleteSelectedMap() const
{
	const auto map = GetSelectedMap();
	return !map.empty() && !m_gd->GetRoomData()->IsMapReferenced(map);
}

std::string MapManagerDialog::SuggestMapName() const
{
	return ::SuggestMapName(*m_gd->GetRoomData());
}

bool MapManagerDialog::PromptForName(const wxString& title, const std::string& initial, std::string& name)
{
	return PromptForMapName(this, title, *m_gd->GetRoomData(), initial, name);
}

void MapManagerDialog::Move(int delta)
{
	const auto name = GetSelectedMap();
	const int index = GetSelectedMapIndex();
	if (name.empty() || index == wxNOT_FOUND)
	{
		return;
	}
	const int new_index = index + delta;
	if (new_index < 0 || new_index >= static_cast<int>(m_map_list->GetCount()))
	{
		return;
	}
	if (m_gd->GetRoomData()->ReorderMap(name, static_cast<std::size_t>(new_index)))
	{
		m_changed = true;
		PopulateMapList(name);
	}
}

void MapManagerDialog::OnMapSelected(wxCommandEvent& /*evt*/)
{
	PopulateRoomList();
	PopulateDetails();
	PopulatePreview();
	UpdateUI();
}

void MapManagerDialog::OnRoomSelected(wxCommandEvent& /*evt*/)
{
	// Rooms sharing a map can use different tilesets and palettes, so the preview follows
	// whichever room is selected.
	PopulatePreview();
}

void MapManagerDialog::OnRoomActivated(wxCommandEvent& /*evt*/)
{
	const int room = GetSelectedRoom();
	if (room < 0)
	{
		return;
	}
	m_room_to_open = room;
	EndModal(wxID_OK);
}

void MapManagerDialog::OnAdd(wxCommandEvent& /*evt*/)
{
	const auto name = PromptCreateMap(this, m_gd);
	if (!name.empty())
	{
		m_changed = true;
		PlaceAfterSelection(name);
		PopulateMapList(name);
	}
}

void MapManagerDialog::PlaceAfterSelection(const std::string& name)
{
	// CreateMap and the importers append to the end of the order. Move the new map to sit
	// directly below whatever was selected, which is where the user was looking when they
	// asked for it.
	const int index = GetSelectedMapIndex();
	if (index == wxNOT_FOUND)
	{
		return;
	}
	const auto& order = m_gd->GetRoomData()->GetMapOrder();
	const auto existing = std::find(order.cbegin(), order.cend(), name);
	if (existing == order.cend())
	{
		return;
	}
	// The selection index still refers to the list as it was before the append, so it is
	// unaffected by the new entry sitting at the end.
	m_gd->GetRoomData()->ReorderMap(name, static_cast<std::size_t>(index) + 1);
}

void MapManagerDialog::OnDuplicate(wxCommandEvent& /*evt*/)
{
	const auto source = GetSelectedMap();
	if (source.empty())
	{
		return;
	}
	const auto room_data = m_gd->GetRoomData();
	std::string name;
	if (!PromptForMapName(this, "Duplicate Map", *room_data, SuggestMapCopyName(*room_data, source), name))
	{
		return;
	}
	if (!DuplicateMap(m_gd, source, name))
	{
		wxMessageBox("Unable to duplicate the selected map.", "Duplicate Map", wxOK | wxICON_ERROR, this);
		return;
	}
	m_changed = true;
	PlaceAfterSelection(name);
	PopulateMapList(name);
}

void MapManagerDialog::OnImport(wxCommandEvent& /*evt*/)
{
	wxFileDialog fd(this, _("Import Map"), "", "",
		"All Supported Formats|*.cmp;*.tmx;*.csv|"
		"Compressed Map (*.cmp)|*.cmp|"
		"Tiled TMX Tilemap (*.tmx)|*.tmx|"
		"CSV Set (*.csv)|*.csv|"
		"All Files (*.*)|*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return;
	}

	const std::filesystem::path chosen(fd.GetPath().ToStdString());
	const auto extension = ToLower(chosen.extension().string());
	std::array<std::string, 3> csv_paths;
	if (extension == ".csv" && !ResolveCsvSet(this, chosen, csv_paths))
	{
		return;
	}

	// Offer the file's own name if it happens to be usable as a map label.
	const auto stem = chosen.stem().string();
	const auto suggested = Landstalker::RoomData::IsValidMapName(stem) &&
		m_gd->GetRoomData()->GetMaps().count(stem) == 0 ? stem : SuggestMapName();
	std::string name;
	if (!PromptForName("Import Map", suggested, name))
	{
		return;
	}

	const auto room_data = m_gd->GetRoomData();
	const auto entry = room_data->CreateMap(name, DEFAULT_MAP_SIZE, DEFAULT_MAP_SIZE,
		DEFAULT_MAP_SIZE, DEFAULT_MAP_SIZE, 0, 0);
	if (!entry)
	{
		wxMessageBox("Unable to create a map named '" + wxString::FromUTF8(name) + "'.",
			"Import Map", wxOK | wxICON_ERROR, this);
		return;
	}

	bool imported = false;
	if (extension == ".csv")
	{
		imported = MapFileIo::ImportCsv(csv_paths, *entry->GetData());
	}
	else if (extension == ".tmx")
	{
		imported = MapFileIo::ImportTmx(chosen.string(), *entry->GetData());
	}
	else
	{
		imported = MapFileIo::ImportCmp(chosen.string(), *entry->GetData());
	}

	if (!imported)
	{
		// Nothing references the placeholder yet, so it can always be taken back out.
		room_data->DeleteMap(name);
		wxMessageBox("Unable to read map data from the selected file.",
			"Import Map", wxOK | wxICON_ERROR, this);
		return;
	}
	m_changed = true;
	PlaceAfterSelection(name);
	PopulateMapList(name);
}

void MapManagerDialog::OnExport(wxCommandEvent& /*evt*/)
{
	const auto name = GetSelectedMap();
	if (name.empty())
	{
		return;
	}
	const auto room_data = m_gd->GetRoomData();
	const auto entry = room_data->GetMaps().count(name) == 0 ? nullptr : room_data->GetMap(name);
	if (!entry || !entry->GetData())
	{
		return;
	}

	wxFileDialog fd(this, _("Export Map"), "", wxString::FromUTF8(name + ".cmp"),
		"Compressed Map (*.cmp)|*.cmp|"
		"CSV Set (*.csv)|*.csv|"
		"Tiled TMX Tilemap (*.tmx)|*.tmx",
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return;
	}

	const std::filesystem::path chosen(fd.GetPath().ToStdString());
	const auto extension = ToLower(chosen.extension().string());
	bool exported = false;
	bool blockset_rendered = true;
	if (extension == ".csv")
	{
		// A CSV set is three files, named after the one the user picked.
		exported = MapFileIo::ExportCsv(MapFileIo::CsvPathsFor(chosen.string()), *entry->GetData());
	}
	else if (extension == ".tmx")
	{
		// Tiled draws the map through a blockset image, and only a room using the map can
		// say which tileset and palette to render it with.
		const auto blockset_path = (chosen.parent_path() / (name + "_blockset.png")).string();
		const int room = FirstRoomUsing(name);
		blockset_rendered = room >= 0 &&
			MapFileIo::RenderBlocksetPng(blockset_path, m_gd, static_cast<uint16_t>(room));
		exported = MapFileIo::ExportTmx(chosen.string(), *entry->GetData(), blockset_path);
	}
	else
	{
		exported = MapFileIo::ExportCmp(chosen.string(), entry);
	}

	if (!exported)
	{
		wxMessageBox("Unable to write the map to the selected location.",
			"Export Map", wxOK | wxICON_ERROR, this);
		return;
	}
	if (!blockset_rendered)
	{
		wxMessageBox("The TMX was written, but no room uses this map, so its blockset image "
			"could not be rendered. Tiled will show the tiles as missing.",
			"Export Map", wxOK | wxICON_INFORMATION, this);
	}
}

void MapManagerDialog::OnRemove(wxCommandEvent& /*evt*/)
{
	const auto name = GetSelectedMap();
	if (name.empty() || !CanDeleteSelectedMap())
	{
		return;
	}
	if (wxMessageBox("Delete map '" + wxString::FromUTF8(name) + "'?\n\n"
		"This cannot be undone. The map and its contents will be removed from the game data, "
		"although the existing binary file will be left on disk.",
		"Remove Map", wxYES_NO | wxNO_DEFAULT | wxICON_WARNING, this) != wxYES)
	{
		return;
	}

	// Work out what to land on before the list shifts under us.
	const auto& order = m_gd->GetRoomData()->GetMapOrder();
	const int index = GetSelectedMapIndex();
	std::string next_selection;
	if (index + 1 < static_cast<int>(order.size()))
	{
		next_selection = order[index + 1];
	}
	else if (index > 0)
	{
		next_selection = order[index - 1];
	}

	if (!m_gd->GetRoomData()->DeleteMap(name))
	{
		wxMessageBox("Unable to delete the selected map.", "Remove Map", wxOK | wxICON_ERROR, this);
		return;
	}
	m_changed = true;
	PopulateMapList(next_selection);
}

void MapManagerDialog::OnMoveUp(wxCommandEvent& /*evt*/)
{
	Move(-1);
}

void MapManagerDialog::OnMoveDown(wxCommandEvent& /*evt*/)
{
	Move(1);
}

void MapManagerDialog::OnRename(wxCommandEvent& /*evt*/)
{
	const auto old_name = GetSelectedMap();
	const int index = GetSelectedMapIndex();
	if (old_name.empty() || index == wxNOT_FOUND)
	{
		return;
	}
	const auto room_data = m_gd->GetRoomData();
	const auto old_display = room_data->GetMapDisplayName(old_name);

	RenameMapDialog dialog(this, old_name, old_display);
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
			(!Landstalker::RoomData::IsValidMapName(name) || room_data->GetMaps().count(name) != 0))
		{
			wxMessageBox("The internal name must be unique, start with a letter, only contain "
				"A-Z, a-z, 0-9 and _, and be at most 30 characters.",
				"Rename Map", wxOK | wxICON_ERROR, this);
			continue;
		}
		if (display_changed && !Landstalker::Labels::IsValid(display, Landstalker::Labels::C_MAPS, index))
		{
			wxMessageBox("The display name must not be empty, must be unique, and must not contain "
				"non-printable characters.", "Rename Map", wxOK | wxICON_ERROR, this);
			continue;
		}

		if (name_changed && !room_data->RenameMap(old_name, name))
		{
			wxMessageBox("Unable to rename the selected map.", "Rename Map", wxOK | wxICON_ERROR, this);
			return;
		}
		if (display_changed)
		{
			Landstalker::Labels::Update(Landstalker::Labels::C_MAPS, index, display);
		}
		m_changed = true;
		PopulateMapList(name);
		return;
	}
}
