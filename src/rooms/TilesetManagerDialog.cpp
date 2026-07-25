#include <rooms/TilesetManagerDialog.h>

#include <algorithm>
#include <cctype>
#include <filesystem>

#include <wx/filedlg.h>
#include <wx/spinctrl.h>

#include <landstalker/misc/Labels.h>
#include <landstalker/misc/Utils.h>
#include <main/ImageBufferWx.h>

namespace
{

const wxSize TREE_MIN_SIZE(260, 260);
// Tilesets are rendered 16 tiles wide then scaled to fit inside this.
const wxSize PREVIEW_SIZE(300, 200);
constexpr std::size_t PREVIEW_COLUMNS = 16;

// A new tileset's default size. Most room tilesets in the stock game are around this,
// and it is a whole number of preview rows.
constexpr int DEFAULT_TILE_COUNT = 256;

std::string ToLower(const std::string& str)
{
	std::string result = str;
	std::transform(result.begin(), result.end(), result.begin(), [](const unsigned char c)
	{
		return static_cast<char>(std::tolower(c));
	});
	return result;
}

// The tree holds one of these per node so a selection resolves without re-deriving the
// item's meaning from its label or its position under its parent.
class TilesetTreeItemData : public wxTreeItemData
{
public:
	TilesetTreeItemData(bool animated, uint8_t tileset, uint8_t anim, const std::string& name)
		: m_animated(animated), m_tileset(tileset), m_anim(anim), m_name(name)
	{}

	bool IsAnimated() const { return m_animated; }
	uint8_t GetTileset() const { return m_tileset; }
	uint8_t GetAnim() const { return m_anim; }
	const std::string& GetName() const { return m_name; }

private:
	bool m_animated;
	uint8_t m_tileset;
	uint8_t m_anim;
	std::string m_name;
};

// Asks for the internal name and size of a new tileset. The name is the assembly label,
// so it follows the same rules as a map or room name.
class NewTilesetDialog : public wxDialog
{
public:
	NewTilesetDialog(wxWindow* parent, const std::string& suggested)
		: wxDialog(parent, wxID_ANY, "New Tileset")
	{
		auto* outer = new wxBoxSizer(wxVERTICAL);
		outer->Add(new wxStaticText(this, wxID_ANY,
			"The name is the assembly label: a unique identifier of at most 30 characters,\n"
			"starting with a letter. The tileset is created blank, along with the primary and\n"
			"secondary blockset a room needs in order to use it."),
			0, wxLEFT | wxRIGHT | wxTOP, 10);

		auto* fields = new wxFlexGridSizer(2, 6, 6);
		fields->AddGrowableCol(1, 1);
		fields->Add(new wxStaticText(this, wxID_ANY, "Name"), 0, wxALIGN_CENTER_VERTICAL);
		m_name = new wxTextCtrl(this, wxID_ANY, wxString::FromUTF8(suggested));
		m_name->SetMaxLength(30);
		fields->Add(m_name, 1, wxEXPAND);
		fields->Add(new wxStaticText(this, wxID_ANY, "Size in tiles"), 0, wxALIGN_CENTER_VERTICAL);
		m_tiles = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
			wxSP_ARROW_KEYS, 1, static_cast<int>(Landstalker::RoomData::MAX_TILESET_TILES),
			DEFAULT_TILE_COUNT);
		fields->Add(m_tiles, 1, wxEXPAND);

		outer->Add(fields, 1, wxALL | wxEXPAND, 10);
		outer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL), 0, wxALL | wxEXPAND, 10);
		SetSizerAndFit(outer);
		SetMinSize(wxSize(460, -1));
		CentreOnParent();
	}

	std::string GetEnteredName() const { return m_name->GetValue().ToStdString(); }
	std::size_t GetTileCount() const { return static_cast<std::size_t>(m_tiles->GetValue()); }

private:
	wxTextCtrl* m_name;
	wxSpinCtrl* m_tiles;
};

// Asks for the parameters of a new animation. These are the values the game reads straight
// out of AnimatedTilesetData, so they are offered in the units it stores them in.
class NewAnimatedTilesetDialog : public wxDialog
{
public:
	NewAnimatedTilesetDialog(wxWindow* parent, const std::string& suggested)
		: wxDialog(parent, wxID_ANY, "New Animated Tileset")
	{
		auto* outer = new wxBoxSizer(wxVERTICAL);
		outer->Add(new wxStaticText(this, wxID_ANY,
			"An animation replaces part of its tileset in VRAM as the game runs. The frames are\n"
			"created blank; edit them in the tileset editor once the animation exists."),
			0, wxLEFT | wxRIGHT | wxTOP, 10);

		auto* fields = new wxFlexGridSizer(2, 6, 6);
		fields->AddGrowableCol(1, 1);
		const auto add_field = [&](const char* label, wxWindow* control)
		{
			fields->Add(new wxStaticText(this, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
			fields->Add(control, 1, wxEXPAND);
		};

		m_name = new wxTextCtrl(this, wxID_ANY, wxString::FromUTF8(suggested));
		m_name->SetMaxLength(30);
		add_field("Name", m_name);

		// VRAM addresses are byte offsets and always tile aligned, so step by a whole tile.
		// Shown in hex, which is how the disassembly and every other tool writes them.
		m_base = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
			wxSP_ARROW_KEYS, 0, 0xFFE0, 0x7300);
		m_base->SetBase(16);
		m_base->SetIncrement(0x20);
		add_field("VRAM address", m_base);

		m_frame_tiles = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
			wxSP_ARROW_KEYS, 1, 256, 8);
		add_field("Tiles per frame", m_frame_tiles);

		m_frames = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
			wxSP_ARROW_KEYS, 1, 255, 4);
		add_field("Frame count", m_frames);

		// Higher is slower: the game counts this many game frames between animation steps.
		m_speed = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
			wxSP_ARROW_KEYS, 1, 255, 0x14);
		add_field("Delay between frames", m_speed);

		outer->Add(fields, 1, wxALL | wxEXPAND, 10);
		outer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL), 0, wxALL | wxEXPAND, 10);
		SetSizerAndFit(outer);
		SetMinSize(wxSize(460, -1));
		CentreOnParent();
	}

	std::string GetEnteredName() const { return m_name->GetValue().ToStdString(); }
	uint16_t GetBase() const { return static_cast<uint16_t>(m_base->GetValue()); }
	uint8_t GetFrames() const { return static_cast<uint8_t>(m_frames->GetValue()); }
	uint8_t GetSpeed() const { return static_cast<uint8_t>(m_speed->GetValue()); }
	// The game stores the frame size in bytes; one 4bpp 8x8 tile is 32 of them.
	uint16_t GetFrameSizeBytes() const
	{
		return static_cast<uint16_t>(m_frame_tiles->GetValue() * 32);
	}

private:
	wxTextCtrl* m_name;
	wxSpinCtrl* m_base;
	wxSpinCtrl* m_frame_tiles;
	wxSpinCtrl* m_frames;
	wxSpinCtrl* m_speed;
};

// Prompts for both of an entry's names: the assembly label the disassembly is built around,
// and the friendlier label the editor shows.
class RenameTilesetDialog : public wxDialog
{
public:
	RenameTilesetDialog(wxWindow* parent, const std::string& internal_name, const std::wstring& display_name)
		: wxDialog(parent, wxID_ANY, "Rename Tileset")
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

// First unused name of the form "<prefix><nn>". Two digits to match the stock naming -
// Tileset01 through Tileset18, Tileset03Anim02 - so a new entry sits alongside rather than
// standing out. Numbers past 99 simply get wider.
std::string SuggestName(const Landstalker::RoomData& rooms, const std::string& prefix)
{
	for (unsigned int i = 1; ; ++i)
	{
		const auto candidate = Landstalker::StrPrintf("%s%02u", prefix.c_str(), i);
		if (!rooms.IsAssetNameInUse(candidate))
		{
			return candidate;
		}
	}
}

}

TilesetManagerDialog::TilesetManagerDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd,
	const std::string& select, InitialAction initial_action)
	: wxDialog(parent, wxID_ANY, "Tilesets", wxDefaultPosition, { 720, 460 }),
	  m_gd(gd),
	  m_changed(false),
	  m_to_open_animated(false),
	  m_tree(nullptr),
	  m_preview(nullptr),
	  m_preview_message(nullptr),
	  m_add(nullptr),
	  m_add_animation(nullptr),
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

	auto* panes = new wxBoxSizer(wxHORIZONTAL);
	outer->Add(panes, 1, wxEXPAND, 5);

	auto* tree_box = new wxStaticBoxSizer(wxVERTICAL, this, "Tilesets");
	m_tree = new wxTreeCtrl(tree_box->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT | wxTR_HIDE_ROOT | wxTR_SINGLE);
	m_tree->SetMinSize(TREE_MIN_SIZE);
	m_tree->SetToolTip("Animations are listed under the tileset they animate.\n"
		"Double-click an entry to close this dialog and open it in the tileset editor.");
	tree_box->Add(m_tree, 1, wxALL | wxEXPAND, 5);
	panes->Add(tree_box, 1, wxALL | wxEXPAND, 5);

	auto* right = new wxBoxSizer(wxVERTICAL);
	panes->Add(right, 1, wxEXPAND, 0);

	auto* detail_box = new wxStaticBoxSizer(wxVERTICAL, this, "Selected Item");
	auto* details = new wxFlexGridSizer(2, 4, 12);
	details->AddGrowableCol(1, 1);
	for (std::size_t row = 0; row < DETAIL_ROWS; ++row)
	{
		m_detail_captions[row] = new wxStaticText(detail_box->GetStaticBox(), wxID_ANY, wxEmptyString);
		details->Add(m_detail_captions[row], 0, wxALIGN_CENTER_VERTICAL);
		// Ellipsize rather than let a long room list stretch the whole dialog.
		m_detail_values[row] = new wxStaticText(detail_box->GetStaticBox(), wxID_ANY, wxEmptyString,
			wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
		m_detail_values[row]->SetMinSize(wxSize(160, -1));
		details->Add(m_detail_values[row], 1, wxEXPAND);
	}
	detail_box->Add(details, 0, wxALL | wxEXPAND, 5);
	right->Add(detail_box, 0, wxALL | wxEXPAND, 5);

	auto* preview_box = new wxStaticBoxSizer(wxVERTICAL, this, "Preview");
	// The bitmap is always exactly PREVIEW_SIZE - the tileset is letterboxed into it - so
	// the pane never resizes as you click between tilesets of different sizes.
	m_preview = new wxStaticBitmap(preview_box->GetStaticBox(), wxID_ANY, wxNullBitmap);
	m_preview->SetMinSize(PREVIEW_SIZE);
	m_preview_message = new wxStaticText(preview_box->GetStaticBox(), wxID_ANY, wxEmptyString,
		wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
	preview_box->Add(m_preview, 1, wxALL | wxALIGN_CENTER, 5);
	preview_box->Add(m_preview_message, 0, wxALL | wxALIGN_CENTER, 5);
	right->Add(preview_box, 1, wxALL | wxEXPAND, 5);

	auto* buttons = new wxBoxSizer(wxHORIZONTAL);
	outer->Add(buttons, 0, wxALL | wxEXPAND, 5);

	m_add = new wxButton(this, wxID_ANY, "Add...");
	m_add_animation = new wxButton(this, wxID_ANY, "Add Animation...");
	m_import = new wxButton(this, wxID_ANY, "Import...");
	m_export = new wxButton(this, wxID_ANY, "Export...");
	m_remove = new wxButton(this, wxID_ANY, "Remove");
	m_move_up = new wxButton(this, wxID_ANY, "Move Up");
	m_move_down = new wxButton(this, wxID_ANY, "Move Down");
	m_rename = new wxButton(this, wxID_ANY, "Rename...");
	m_close = new wxButton(this, wxID_CANCEL, "Close");
	for (auto* button : { m_add, m_add_animation, m_import, m_export, m_remove, m_move_up, m_move_down, m_rename })
	{
		buttons->Add(button, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 5);
	}
	buttons->AddStretchSpacer();
	buttons->Add(m_close, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 5);
	m_close->SetDefault();

	GetSizer()->Fit(this);
	SetMinSize(GetSize());
	CentreOnParent(wxBOTH);

	PopulateTree(select);

	m_tree->Connect(wxEVT_TREE_SEL_CHANGED, wxTreeEventHandler(TilesetManagerDialog::OnSelectionChanged), nullptr, this);
	m_tree->Connect(wxEVT_TREE_ITEM_ACTIVATED, wxTreeEventHandler(TilesetManagerDialog::OnItemActivated), nullptr, this);
	m_add->Connect(wxEVT_BUTTON, wxCommandEventHandler(TilesetManagerDialog::OnAdd), nullptr, this);
	m_add_animation->Connect(wxEVT_BUTTON, wxCommandEventHandler(TilesetManagerDialog::OnAddAnimation), nullptr, this);
	m_import->Connect(wxEVT_BUTTON, wxCommandEventHandler(TilesetManagerDialog::OnImport), nullptr, this);
	m_export->Connect(wxEVT_BUTTON, wxCommandEventHandler(TilesetManagerDialog::OnExport), nullptr, this);
	m_remove->Connect(wxEVT_BUTTON, wxCommandEventHandler(TilesetManagerDialog::OnRemove), nullptr, this);
	m_move_up->Connect(wxEVT_BUTTON, wxCommandEventHandler(TilesetManagerDialog::OnMoveUp), nullptr, this);
	m_move_down->Connect(wxEVT_BUTTON, wxCommandEventHandler(TilesetManagerDialog::OnMoveDown), nullptr, this);
	m_rename->Connect(wxEVT_BUTTON, wxCommandEventHandler(TilesetManagerDialog::OnRename), nullptr, this);

	if (initial_action != InitialAction::NONE)
	{
		// Deferred so the operation's own dialogs open over a fully shown manager.
		CallAfter([this, initial_action]() { RunInitialAction(initial_action); });
	}
}

void TilesetManagerDialog::RunInitialAction(InitialAction action)
{
	wxCommandEvent dummy;
	if (action == InitialAction::ADD)
	{
		OnAdd(dummy);
		if (m_changed)
		{
			// Close and hand back the new tileset, as if it had been double-clicked.
			const auto selection = GetSelection();
			if (selection.valid)
			{
				m_to_open = selection.name;
				m_to_open_animated = selection.animated;
			}
			EndModal(wxID_OK);
		}
	}
	else if (action == InitialAction::REMOVE)
	{
		OnRemove(dummy);
		if (m_changed)
		{
			EndModal(wxID_OK);
		}
	}
}

TilesetManagerDialog::~TilesetManagerDialog()
{
	m_tree->Disconnect(wxEVT_TREE_SEL_CHANGED, wxTreeEventHandler(TilesetManagerDialog::OnSelectionChanged), nullptr, this);
	m_tree->Disconnect(wxEVT_TREE_ITEM_ACTIVATED, wxTreeEventHandler(TilesetManagerDialog::OnItemActivated), nullptr, this);
	m_add->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(TilesetManagerDialog::OnAdd), nullptr, this);
	m_add_animation->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(TilesetManagerDialog::OnAddAnimation), nullptr, this);
	m_import->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(TilesetManagerDialog::OnImport), nullptr, this);
	m_export->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(TilesetManagerDialog::OnExport), nullptr, this);
	m_remove->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(TilesetManagerDialog::OnRemove), nullptr, this);
	m_move_up->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(TilesetManagerDialog::OnMoveUp), nullptr, this);
	m_move_down->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(TilesetManagerDialog::OnMoveDown), nullptr, this);
	m_rename->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(TilesetManagerDialog::OnRename), nullptr, this);
}

void TilesetManagerDialog::MarkChanged()
{
	m_changed = true;
	// GameData answers name lookups from caches built when the project was opened, and the
	// navigation tree resolves what it opens through them. Rebuilding here rather than at
	// close keeps them correct for the rest of this dialog's own work too.
	m_gd->RefreshCaches();
}

void TilesetManagerDialog::PopulateTree(const std::string& select)
{
	const auto room_data = m_gd->GetRoomData();
	m_tree->Freeze();
	m_tree->DeleteAllItems();
	const auto root = m_tree->AddRoot("");
	wxTreeItemId to_select;
	wxTreeItemId first;

	for (const auto& tileset : room_data->GetTilesets())
	{
		const auto index = static_cast<uint8_t>(tileset->GetIndex());
		// GetName returns by value, so it has to be held: calling it twice for a pair of
		// iterators would walk between two unrelated temporaries.
		const auto name = tileset->GetName();
		// Asked of Labels directly rather than through GetTilesetDisplayName, which
		// synthesises a name from the slot number when none is set - that would decorate
		// every row with something the tree already shows.
		const auto display = Landstalker::Labels::Get(Landstalker::Labels::C_TILESETS, index);
		auto label = wxString::FromUTF8(name);
		if (display && !display->empty())
		{
			label += " (" + wxString(*display) + ")";
		}
		const auto node = m_tree->AppendItem(root, label, -1, -1,
			new TilesetTreeItemData(false, index, 0, name));
		if (!first.IsOk())
		{
			first = node;
		}
		if (name == select)
		{
			to_select = node;
		}

		for (const auto& anim : room_data->GetAnimatedTilesets(name))
		{
			const auto anim_index = anim->GetIndex().second;
			const auto anim_name = anim->GetName();
			const auto anim_display = Landstalker::Labels::Get(Landstalker::Labels::C_ANIM_TILESETS,
				(index << 8) | anim_index);
			auto anim_label = wxString::FromUTF8(anim_name);
			if (anim_display && !anim_display->empty())
			{
				anim_label += " (" + wxString(*anim_display) + ")";
			}
			const auto anim_node = m_tree->AppendItem(node, anim_label, -1, -1,
				new TilesetTreeItemData(true, index, anim_index, anim_name));
			if (anim_name == select)
			{
				to_select = anim_node;
			}
		}
		m_tree->Expand(node);
	}
	m_tree->Thaw();

	if (!to_select.IsOk())
	{
		to_select = first;
	}
	if (to_select.IsOk())
	{
		m_tree->SelectItem(to_select);
		m_tree->EnsureVisible(to_select);
	}
	PopulateDetails();
	PopulatePreview();
	UpdateUI();
}

TilesetManagerDialog::Selection TilesetManagerDialog::GetSelection() const
{
	Selection selection;
	const auto item = m_tree->GetSelection();
	if (!item.IsOk())
	{
		return selection;
	}
	const auto* data = dynamic_cast<TilesetTreeItemData*>(m_tree->GetItemData(item));
	if (data == nullptr)
	{
		return selection;
	}
	selection.valid = true;
	selection.animated = data->IsAnimated();
	selection.tileset = data->GetTileset();
	selection.anim = data->GetAnim();
	selection.name = data->GetName();
	return selection;
}

std::shared_ptr<Landstalker::Palette> TilesetManagerDialog::PreviewPalette(
	const Landstalker::PalettePreferences& entry) const
{
	const auto room_data = m_gd->GetRoomData();
	const auto& preferred = entry.GetDefaultPalette();
	if (!preferred.empty())
	{
		const auto palette = m_gd->GetPalette(preferred);
		if (palette)
		{
			return palette->GetData();
		}
	}
	// A tileset no room uses has no recommended palette, so fall back to any room palette
	// rather than leaving the preview blank.
	const auto& room_palettes = room_data->GetRoomPalettes();
	if (!room_palettes.empty())
	{
		return room_palettes.front()->GetData();
	}
	return nullptr;
}

void TilesetManagerDialog::PopulatePreview()
{
	const auto selection = GetSelection();
	if (selection.valid && selection.name == m_previewed)
	{
		return;
	}
	m_previewed = selection.valid ? selection.name : std::string();

	const auto show_message = [this](const wxString& message)
	{
		m_preview->SetBitmap(wxNullBitmap);
		m_preview->Hide();
		m_preview_message->SetLabel(message);
		m_preview_message->Show();
		Layout();
	};

	if (!selection.valid)
	{
		show_message("Nothing selected.");
		return;
	}

	const auto room_data = m_gd->GetRoomData();
	std::shared_ptr<Landstalker::Tileset> tiles;
	std::shared_ptr<Landstalker::Palette> palette;
	if (selection.animated)
	{
		const auto entry = room_data->GetAnimatedTileset(selection.tileset, selection.anim);
		if (entry)
		{
			tiles = entry->GetData();
			palette = PreviewPalette(*entry);
		}
	}
	else
	{
		const auto entry = room_data->GetTileset(selection.tileset);
		if (entry)
		{
			tiles = entry->GetData();
			palette = PreviewPalette(*entry);
		}
	}

	if (!tiles || !palette || tiles->GetTileCount() == 0)
	{
		show_message("Preview unavailable.");
		return;
	}

	const auto count = tiles->GetTileCount();
	const int columns = static_cast<int>(std::min(count, PREVIEW_COLUMNS));
	const int rows = static_cast<int>((count + PREVIEW_COLUMNS - 1) / PREVIEW_COLUMNS);
	const int width = columns * static_cast<int>(tiles->GetTileWidth());
	const int height = rows * static_cast<int>(tiles->GetTileHeight());
	if (width <= 0 || height <= 0)
	{
		show_message("Preview unavailable.");
		return;
	}

	ImageBufferWx buffer(width, height);
	for (std::size_t i = 0; i < count; ++i)
	{
		buffer.InsertTile(static_cast<int>((i % PREVIEW_COLUMNS) * tiles->GetTileWidth()),
			static_cast<int>((i / PREVIEW_COLUMNS) * tiles->GetTileHeight()), 0,
			Landstalker::Tile(static_cast<int>(i)), *tiles);
	}
	wxImage image = buffer.MakeImage({ palette });

	// Only ever scale down, keeping the aspect ratio so the tiles stay square.
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

void TilesetManagerDialog::SetDetail(std::size_t row, const wxString& caption, const wxString& value)
{
	if (row >= DETAIL_ROWS)
	{
		return;
	}
	m_detail_captions[row]->SetLabel(caption);
	m_detail_values[row]->SetLabel(value);
}

void TilesetManagerDialog::PopulateDetails()
{
	const auto selection = GetSelection();
	const auto room_data = m_gd->GetRoomData();
	for (std::size_t row = 0; row < DETAIL_ROWS; ++row)
	{
		SetDetail(row, wxEmptyString, wxEmptyString);
	}

	if (!selection.valid)
	{
		Layout();
		return;
	}

	if (selection.animated)
	{
		const auto entry = room_data->GetAnimatedTileset(selection.tileset, selection.anim);
		const auto parent = room_data->GetTileset(selection.tileset);
		if (!entry || !entry->GetData())
		{
			Layout();
			return;
		}
		const auto anim = entry->GetData();
		SetDetail(0, "Animates", parent ? wxString::FromUTF8(parent->GetName()) : wxString("-"));
		SetDetail(1, "Slot", wxString::Format("%d of %d", selection.anim + 1,
			static_cast<int>(Landstalker::RoomData::MAX_ANIMS_PER_TILESET)));
		SetDetail(2, "Frames", wxString::Format("%d", anim->GetAnimationFrames()));
		SetDetail(3, "Tiles per frame", wxString::Format("%d (%d bytes)",
			static_cast<int>(anim->GetFrameSizeTiles()), anim->GetFrameSizeBytes()));
		SetDetail(4, "VRAM address", wxString::Format("$%04X", anim->GetBaseBytes()));
		// Higher is slower: it is the number of game frames between animation steps.
		SetDetail(5, "Frame delay", wxString::Format("%d", anim->GetAnimationSpeed()));
		Layout();
		return;
	}

	const auto entry = room_data->GetTileset(selection.tileset);
	if (!entry || !entry->GetData())
	{
		Layout();
		return;
	}
	const auto tiles = entry->GetData();
	const auto refs = room_data->CountTilesetReferences(selection.tileset);
	SetDetail(0, "Slot", wxString::Format("%d of %d", selection.tileset,
		static_cast<int>(Landstalker::RoomData::MAX_TILESETS) - 1));
	SetDetail(1, "Tiles", wxString::Format("%d", static_cast<int>(tiles->GetTileCount())));
	SetDetail(2, "Size", wxString::Format("%d bytes uncompressed",
		static_cast<int>(tiles->GetTilesetUncompressedSizeBytes())));
	SetDetail(3, "Blocksets", wxString::Format("%d", static_cast<int>(refs.blocksets)));
	SetDetail(4, "Animations", wxString::Format("%d of %d", static_cast<int>(refs.animated_tilesets),
		static_cast<int>(Landstalker::RoomData::MAX_ANIMS_PER_TILESET)));

	if (refs.rooms == 0)
	{
		SetDetail(5, "Used by rooms", "None");
	}
	else
	{
		// Name the first few so it is obvious what a delete would be blocked on.
		wxString rooms;
		std::size_t named = 0;
		const auto& roomlist = room_data->GetRoomlist();
		for (std::size_t i = 0; i < roomlist.size() && named < 3; ++i)
		{
			if (roomlist[i]->tileset != selection.tileset)
			{
				continue;
			}
			if (!rooms.IsEmpty())
			{
				rooms += ", ";
			}
			rooms += wxString(roomlist[i]->GetDisplayName());
			++named;
		}
		if (refs.rooms > named)
		{
			rooms += wxString::Format(" and %d more", static_cast<int>(refs.rooms - named));
		}
		SetDetail(5, "Used by rooms", wxString::Format("%d (%s)", static_cast<int>(refs.rooms), rooms));
	}
	Layout();
}

void TilesetManagerDialog::UpdateUI()
{
	const auto selection = GetSelection();
	const auto room_data = m_gd->GetRoomData();
	const int count = static_cast<int>(room_data->GetTilesets().size());
	const bool is_tileset = selection.valid && !selection.animated;

	m_add->Enable(count < static_cast<int>(Landstalker::RoomData::MAX_TILESETS));
	m_add_animation->Enable(selection.valid &&
		room_data->CountTilesetReferences(selection.tileset).animated_tilesets <
			Landstalker::RoomData::MAX_ANIMS_PER_TILESET);
	m_import->Enable(selection.valid);
	m_export->Enable(selection.valid);
	// A tileset can only go once nothing draws it, and the last one can never go.
	m_remove->Enable(selection.valid && (selection.animated ||
		(count > 1 && !room_data->IsTilesetUsedByRooms(selection.tileset))));
	m_rename->Enable(selection.valid);
	// Only tilesets are ordered; an animation's position within its parent is not meaningful.
	m_move_up->Enable(is_tileset && selection.tileset > 0);
	m_move_down->Enable(is_tileset && selection.tileset < count - 1);
}

void TilesetManagerDialog::Move(int delta)
{
	const auto selection = GetSelection();
	if (!selection.valid || selection.animated)
	{
		return;
	}
	const int new_index = static_cast<int>(selection.tileset) + delta;
	if (new_index < 0 || new_index >= static_cast<int>(m_gd->GetRoomData()->GetTilesets().size()))
	{
		return;
	}
	// Swap the two tilesets' content rather than renumbering references: a room keeps its
	// tileset field, so the two tilesets change places in the rooms that use them.
	if (m_gd->GetRoomData()->SwapTilesets(selection.tileset, static_cast<uint8_t>(new_index)))
	{
		MarkChanged();
		PopulateTree(selection.name);
	}
}

void TilesetManagerDialog::OnSelectionChanged(wxTreeEvent& /*evt*/)
{
	PopulateDetails();
	PopulatePreview();
	UpdateUI();
}

void TilesetManagerDialog::OnItemActivated(wxTreeEvent& /*evt*/)
{
	const auto selection = GetSelection();
	if (!selection.valid)
	{
		return;
	}
	m_to_open = selection.name;
	m_to_open_animated = selection.animated;
	EndModal(wxID_OK);
}

void TilesetManagerDialog::OnAdd(wxCommandEvent& /*evt*/)
{
	const auto room_data = m_gd->GetRoomData();
	NewTilesetDialog dialog(this, SuggestName(*room_data, "Tileset"));
	while (dialog.ShowModal() == wxID_OK)
	{
		const auto name = dialog.GetEnteredName();
		if (!Landstalker::RoomData::IsValidTilesetName(name) || room_data->IsAssetNameInUse(name))
		{
			wxMessageBox("The name must be unique, start with a letter, only contain A-Z, a-z, "
				"0-9 and _, and be at most 30 characters.",
				"New Tileset", wxOK | wxICON_ERROR, this);
			continue;
		}
		if (!room_data->AddTileset(name, dialog.GetTileCount()))
		{
			wxMessageBox("Unable to create the tileset.", "New Tileset", wxOK | wxICON_ERROR, this);
			return;
		}
		MarkChanged();
		PopulateTree(name);
		return;
	}
}

void TilesetManagerDialog::OnAddAnimation(wxCommandEvent& /*evt*/)
{
	const auto selection = GetSelection();
	if (!selection.valid)
	{
		return;
	}
	const auto room_data = m_gd->GetRoomData();
	const auto parent = room_data->GetTileset(selection.tileset);
	const auto suggested = SuggestName(*room_data,
		(parent ? parent->GetName() : std::string("Tileset")) + "Anim");

	NewAnimatedTilesetDialog dialog(this, suggested);
	while (dialog.ShowModal() == wxID_OK)
	{
		const auto name = dialog.GetEnteredName();
		if (!Landstalker::RoomData::IsValidTilesetName(name) || room_data->IsAssetNameInUse(name))
		{
			wxMessageBox("The name must be unique, start with a letter, only contain A-Z, a-z, "
				"0-9 and _, and be at most 30 characters.",
				"New Animated Tileset", wxOK | wxICON_ERROR, this);
			continue;
		}
		if (!room_data->AddAnimatedTileset(selection.tileset, name, dialog.GetBase(),
			dialog.GetFrameSizeBytes(), dialog.GetSpeed(), dialog.GetFrames()))
		{
			wxMessageBox("Unable to create the animation.", "New Animated Tileset",
				wxOK | wxICON_ERROR, this);
			return;
		}
		MarkChanged();
		PopulateTree(name);
		return;
	}
}

void TilesetManagerDialog::OnImport(wxCommandEvent& /*evt*/)
{
	const auto selection = GetSelection();
	if (!selection.valid)
	{
		return;
	}
	const auto room_data = m_gd->GetRoomData();
	std::shared_ptr<Landstalker::Tileset> tiles;
	if (selection.animated)
	{
		const auto entry = room_data->GetAnimatedTileset(selection.tileset, selection.anim);
		tiles = entry ? entry->GetData() : nullptr;
	}
	else
	{
		const auto entry = room_data->GetTileset(selection.tileset);
		tiles = entry ? entry->GetData() : nullptr;
	}
	if (!tiles)
	{
		return;
	}

	wxFileDialog fd(this, _("Import Tileset"), "", "",
		"All Supported Formats|*.bin;*.lz77|"
		"Uncompressed Tileset (*.bin)|*.bin|"
		"Compressed Tileset (*.lz77)|*.lz77|"
		"All Files (*.*)|*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return;
	}
	const std::filesystem::path chosen(fd.GetPath().ToStdString());
	const bool compressed = ToLower(chosen.extension().string()) == ".lz77";
	const auto bytes = Landstalker::ReadBytes(chosen.string());
	if (bytes.empty())
	{
		wxMessageBox("Unable to read tileset data from the selected file.",
			"Import Tileset", wxOK | wxICON_ERROR, this);
		return;
	}
	// Replacing the graphics changes the tile count, which is what the room's blocks index
	// into - a shorter tileset leaves blocks pointing past the end.
	const auto old_count = tiles->GetTileCount();
	tiles->SetBits(bytes, compressed);
	if (tiles->GetTileCount() < old_count && !selection.animated &&
		room_data->IsTilesetUsedByRooms(selection.tileset))
	{
		wxMessageBox(wxString::Format(
			"The imported tileset has %d tiles, fewer than the %d it replaced. Blocks in rooms "
			"using this tileset may now refer to tiles that no longer exist.",
			static_cast<int>(tiles->GetTileCount()), static_cast<int>(old_count)),
			"Import Tileset", wxOK | wxICON_WARNING, this);
	}
	MarkChanged();
	// Force the preview to redraw even though the selection has not moved.
	m_previewed.clear();
	PopulateDetails();
	PopulatePreview();
}

void TilesetManagerDialog::OnExport(wxCommandEvent& /*evt*/)
{
	const auto selection = GetSelection();
	if (!selection.valid)
	{
		return;
	}
	const auto room_data = m_gd->GetRoomData();
	std::shared_ptr<Landstalker::Tileset> tiles;
	const Landstalker::PalettePreferences* preferences = nullptr;
	if (selection.animated)
	{
		const auto entry = room_data->GetAnimatedTileset(selection.tileset, selection.anim);
		if (entry)
		{
			tiles = entry->GetData();
			preferences = entry.get();
		}
	}
	else
	{
		const auto entry = room_data->GetTileset(selection.tileset);
		if (entry)
		{
			tiles = entry->GetData();
			preferences = entry.get();
		}
	}
	if (!tiles)
	{
		return;
	}

	wxFileDialog fd(this, _("Export Tileset"), "", wxString::FromUTF8(selection.name + ".lz77"),
		"Compressed Tileset (*.lz77)|*.lz77|"
		"Uncompressed Tileset (*.bin)|*.bin|"
		"PNG Image (*.png)|*.png",
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return;
	}

	const std::filesystem::path chosen(fd.GetPath().ToStdString());
	const auto extension = ToLower(chosen.extension().string());
	if (extension == ".png")
	{
		const auto palette = preferences ? PreviewPalette(*preferences) : nullptr;
		if (!palette)
		{
			wxMessageBox("No palette is available to render this tileset with.",
				"Export Tileset", wxOK | wxICON_ERROR, this);
			return;
		}
		const auto count = tiles->GetTileCount();
		const int columns = static_cast<int>(std::min(count, PREVIEW_COLUMNS));
		const int rows = static_cast<int>((count + PREVIEW_COLUMNS - 1) / PREVIEW_COLUMNS);
		ImageBufferWx buffer(columns * tiles->GetTileWidth(), rows * tiles->GetTileHeight());
		for (std::size_t i = 0; i < count; ++i)
		{
			buffer.InsertTile(static_cast<int>((i % PREVIEW_COLUMNS) * tiles->GetTileWidth()),
				static_cast<int>((i / PREVIEW_COLUMNS) * tiles->GetTileHeight()), 0,
				Landstalker::Tile(static_cast<int>(i)), *tiles);
		}
		buffer.WritePNG(chosen.string(), { palette });
		return;
	}

	const bool compressed = extension == ".lz77";
	Landstalker::WriteBytes(tiles->GetBits(compressed), chosen.string());
}

void TilesetManagerDialog::OnRemove(wxCommandEvent& /*evt*/)
{
	const auto selection = GetSelection();
	if (!selection.valid)
	{
		return;
	}
	const auto room_data = m_gd->GetRoomData();
	const auto name = wxString::FromUTF8(selection.name);

	if (selection.animated)
	{
		if (wxMessageBox("Delete animation '" + name + "'?\n\n"
			"This cannot be undone. The animation will be removed from the game data, although "
			"the existing binary file will be left on disk.",
			"Remove Animation", wxYES_NO | wxNO_DEFAULT | wxICON_WARNING, this) != wxYES)
		{
			return;
		}
		const auto parent = room_data->GetTileset(selection.tileset);
		const auto reselect = parent ? parent->GetName() : std::string();
		if (!room_data->DeleteAnimatedTileset(selection.tileset, selection.anim))
		{
			wxMessageBox("Unable to delete the selected animation.", "Remove Animation",
				wxOK | wxICON_ERROR, this);
			return;
		}
		MarkChanged();
		PopulateTree(reselect);
		return;
	}

	const auto refs = room_data->CountTilesetReferences(selection.tileset);
	if (refs.rooms > 0)
	{
		wxMessageBox(wxString::Format(
			"'%s' cannot be deleted: %d room%s still use%s it.\n\n"
			"Point those rooms at another tileset first.",
			name, static_cast<int>(refs.rooms), refs.rooms == 1 ? "" : "s", refs.rooms == 1 ? "s" : ""),
			"Remove Tileset", wxOK | wxICON_ERROR, this);
		return;
	}

	wxString warning = "Delete tileset '" + name + "'?\n\n"
		"This cannot be undone. Every tileset above it moves down a slot, and the rooms, "
		"blocksets and animations that name those slots are renumbered to follow.";
	if (refs.blocksets > 0 || refs.animated_tilesets > 0)
	{
		warning += wxString::Format("\n\nThe following will be deleted along with it:\n"
			"  %d blockset%s\n  %d animation%s",
			static_cast<int>(refs.blocksets), refs.blocksets == 1 ? "" : "s",
			static_cast<int>(refs.animated_tilesets), refs.animated_tilesets == 1 ? "" : "s");
	}
	if (wxMessageBox(warning, "Remove Tileset", wxYES_NO | wxNO_DEFAULT | wxICON_WARNING, this) != wxYES)
	{
		return;
	}

	// Work out what to land on before the slots shift under us.
	const auto tilesets = room_data->GetTilesets();
	std::string next_selection;
	if (selection.tileset + 1u < tilesets.size())
	{
		next_selection = tilesets[selection.tileset + 1u]->GetName();
	}
	else if (selection.tileset > 0)
	{
		next_selection = tilesets[selection.tileset - 1u]->GetName();
	}

	if (!room_data->DeleteTileset(selection.tileset))
	{
		wxMessageBox("Unable to delete the selected tileset.", "Remove Tileset",
			wxOK | wxICON_ERROR, this);
		return;
	}
	MarkChanged();
	PopulateTree(next_selection);
}

void TilesetManagerDialog::OnMoveUp(wxCommandEvent& /*evt*/)
{
	Move(-1);
}

void TilesetManagerDialog::OnMoveDown(wxCommandEvent& /*evt*/)
{
	Move(1);
}

void TilesetManagerDialog::OnRename(wxCommandEvent& /*evt*/)
{
	const auto selection = GetSelection();
	if (!selection.valid)
	{
		return;
	}
	const auto room_data = m_gd->GetRoomData();
	const auto& category = selection.animated
		? Landstalker::Labels::C_ANIM_TILESETS : Landstalker::Labels::C_TILESETS;
	const int label_id = selection.animated
		? ((selection.tileset << 8) | selection.anim) : selection.tileset;
	const auto old_name = selection.name;
	const auto old_display = selection.animated
		? room_data->GetAnimatedTilesetDisplayName(selection.tileset, selection.anim)
		: room_data->GetTilesetDisplayName(selection.tileset);

	RenameTilesetDialog dialog(this, old_name, old_display);
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
			(!Landstalker::RoomData::IsValidTilesetName(name) || room_data->IsAssetNameInUse(name)))
		{
			wxMessageBox("The internal name must be unique, start with a letter, only contain "
				"A-Z, a-z, 0-9 and _, and be at most 30 characters.",
				"Rename Tileset", wxOK | wxICON_ERROR, this);
			continue;
		}
		if (display_changed && !Landstalker::Labels::IsValid(display, category, label_id))
		{
			wxMessageBox("The display name must not be empty, must be unique, and must not contain "
				"non-printable characters.", "Rename Tileset", wxOK | wxICON_ERROR, this);
			continue;
		}

		const bool renamed = !name_changed || (selection.animated
			? room_data->RenameAnimatedTileset(old_name, name)
			: room_data->RenameTileset(old_name, name));
		if (!renamed)
		{
			wxMessageBox("Unable to rename the selected tileset.", "Rename Tileset",
				wxOK | wxICON_ERROR, this);
			return;
		}
		if (display_changed)
		{
			Landstalker::Labels::Update(category, label_id, display);
		}
		MarkChanged();
		PopulateTree(name);
		return;
	}
}
