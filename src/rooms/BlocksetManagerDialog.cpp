#include <rooms/BlocksetManagerDialog.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <wx/filedlg.h>
#include <wx/spinctrl.h>

#include <landstalker/blockset/BlocksetCmp.h>
#include <landstalker/misc/Labels.h>
#include <landstalker/misc/Utils.h>
#include <main/ImageBufferWx.h>

namespace
{

const wxSize TREE_MIN_SIZE(280, 300);
// Blocks are rendered 8 across then scaled to fit inside this.
const wxSize PREVIEW_SIZE(300, 220);
constexpr std::size_t PREVIEW_COLUMNS = 8;

// A new blockset's default size, a whole number of preview rows.
constexpr int DEFAULT_BLOCK_COUNT = 32;

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
// item's meaning from its label or its depth, which varies with how many sets a tileset has.
class BlocksetTreeItemData : public wxTreeItemData
{
public:
	BlocksetTreeItemData(int kind, uint8_t tileset, uint8_t primary, uint8_t sec, const std::string& name)
		: m_kind(kind), m_tileset(tileset), m_primary(primary), m_sec(sec), m_name(name)
	{}

	int GetKind() const { return m_kind; }
	uint8_t GetTileset() const { return m_tileset; }
	uint8_t GetPrimary() const { return m_primary; }
	uint8_t GetSec() const { return m_sec; }
	const std::string& GetName() const { return m_name; }

private:
	int m_kind;
	uint8_t m_tileset;
	uint8_t m_primary;
	uint8_t m_sec;
	std::string m_name;
};

// Asks for the internal name and size of a new blockset. The name is the assembly label, so
// it follows the same rules as a tileset or map name.
class NewBlocksetDialog : public wxDialog
{
public:
	NewBlocksetDialog(wxWindow* parent, const wxString& title, const std::string& suggested,
		const wxString& note, int remaining_budget)
		: wxDialog(parent, wxID_ANY, title)
	{
		auto* outer = new wxBoxSizer(wxVERTICAL);
		outer->Add(new wxStaticText(this, wxID_ANY, note), 0, wxLEFT | wxRIGHT | wxTOP, 10);

		auto* fields = new wxFlexGridSizer(2, 6, 6);
		fields->AddGrowableCol(1, 1);
		fields->Add(new wxStaticText(this, wxID_ANY, "Name"), 0, wxALIGN_CENTER_VERTICAL);
		m_name = new wxTextCtrl(this, wxID_ANY, wxString::FromUTF8(suggested));
		m_name->SetMaxLength(30);
		fields->Add(m_name, 1, wxEXPAND);
		fields->Add(new wxStaticText(this, wxID_ANY, "Size in blocks"), 0, wxALIGN_CENTER_VERTICAL);
		// A room can only address so many blocks across its base and alternate together, so
		// the spinner stops where this blockset would push it over.
		const int ceiling = std::max(1, remaining_budget);
		m_blocks = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
			wxSP_ARROW_KEYS, 1, ceiling, std::min(DEFAULT_BLOCK_COUNT, ceiling));
		fields->Add(m_blocks, 1, wxEXPAND);

		outer->Add(fields, 1, wxALL | wxEXPAND, 10);
		outer->Add(new wxStaticText(this, wxID_ANY,
			wxString::Format("At most %d blocks will fit alongside what rooms in this set already draw.",
				ceiling)), 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
		outer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL), 0, wxALL | wxEXPAND, 10);
		SetSizerAndFit(outer);
		SetMinSize(wxSize(500, -1));
		CentreOnParent();
	}

	std::string GetEnteredName() const { return m_name->GetValue().ToStdString(); }
	std::size_t GetBlockCount() const { return static_cast<std::size_t>(m_blocks->GetValue()); }

private:
	wxTextCtrl* m_name;
	wxSpinCtrl* m_blocks;
};

// Prompts for both of a blockset's names: the assembly label the disassembly is built
// around, and the friendlier label the editor shows.
class RenameBlocksetDialog : public wxDialog
{
public:
	RenameBlocksetDialog(wxWindow* parent, const std::string& internal_name, const std::wstring& display_name)
		: wxDialog(parent, wxID_ANY, "Rename Blockset")
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
// BT01_00 through BT18_05 - so a new entry sits alongside rather than standing out.
// Numbers past 99 simply get wider.
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

std::string ReadTextFile(const std::string& path)
{
	std::ifstream file(path, std::ios::binary);
	std::ostringstream contents;
	contents << file.rdbuf();
	return contents.str();
}

}

BlocksetManagerDialog::BlocksetManagerDialog(wxWindow* parent,
	std::shared_ptr<Landstalker::GameData> gd, const std::string& select, InitialAction initial_action)
	: wxDialog(parent, wxID_ANY, "Blocksets", wxDefaultPosition, { 760, 500 }),
	  m_gd(gd),
	  m_changed(false),
	  m_tree(nullptr),
	  m_preview(nullptr),
	  m_preview_message(nullptr),
	  m_add(nullptr),
	  m_add_set(nullptr),
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

	auto* tree_box = new wxStaticBoxSizer(wxVERTICAL, this, "Blocksets");
	m_tree = new wxTreeCtrl(tree_box->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT | wxTR_HIDE_ROOT | wxTR_SINGLE);
	m_tree->SetMinSize(TREE_MIN_SIZE);
	m_tree->SetToolTip(
		"Each tileset's blocksets are listed under it. The first is the base every room in\n"
		"the set draws; the rest are alternates a room chooses between.\n"
		"Double-click a blockset to close this dialog and open it.");
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
		m_detail_values[row]->SetMinSize(wxSize(170, -1));
		details->Add(m_detail_values[row], 1, wxEXPAND);
	}
	detail_box->Add(details, 0, wxALL | wxEXPAND, 5);
	right->Add(detail_box, 0, wxALL | wxEXPAND, 5);

	auto* preview_box = new wxStaticBoxSizer(wxVERTICAL, this, "Preview");
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
	m_add_set = new wxButton(this, wxID_ANY, "Add Set...");
	m_import = new wxButton(this, wxID_ANY, "Import...");
	m_export = new wxButton(this, wxID_ANY, "Export...");
	m_remove = new wxButton(this, wxID_ANY, "Remove");
	m_move_up = new wxButton(this, wxID_ANY, "Move Up");
	m_move_down = new wxButton(this, wxID_ANY, "Move Down");
	m_rename = new wxButton(this, wxID_ANY, "Rename...");
	m_close = new wxButton(this, wxID_CANCEL, "Close");
	for (auto* button : { m_add, m_add_set, m_import, m_export, m_remove, m_move_up, m_move_down, m_rename })
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

	m_tree->Connect(wxEVT_TREE_SEL_CHANGED, wxTreeEventHandler(BlocksetManagerDialog::OnSelectionChanged), nullptr, this);
	m_tree->Connect(wxEVT_TREE_ITEM_ACTIVATED, wxTreeEventHandler(BlocksetManagerDialog::OnItemActivated), nullptr, this);
	m_add->Connect(wxEVT_BUTTON, wxCommandEventHandler(BlocksetManagerDialog::OnAdd), nullptr, this);
	m_add_set->Connect(wxEVT_BUTTON, wxCommandEventHandler(BlocksetManagerDialog::OnAddSet), nullptr, this);
	m_import->Connect(wxEVT_BUTTON, wxCommandEventHandler(BlocksetManagerDialog::OnImport), nullptr, this);
	m_export->Connect(wxEVT_BUTTON, wxCommandEventHandler(BlocksetManagerDialog::OnExport), nullptr, this);
	m_remove->Connect(wxEVT_BUTTON, wxCommandEventHandler(BlocksetManagerDialog::OnRemove), nullptr, this);
	m_move_up->Connect(wxEVT_BUTTON, wxCommandEventHandler(BlocksetManagerDialog::OnMoveUp), nullptr, this);
	m_move_down->Connect(wxEVT_BUTTON, wxCommandEventHandler(BlocksetManagerDialog::OnMoveDown), nullptr, this);
	m_rename->Connect(wxEVT_BUTTON, wxCommandEventHandler(BlocksetManagerDialog::OnRename), nullptr, this);

	if (initial_action != InitialAction::NONE)
	{
		// Deferred so the operation's own dialogs open over a fully shown manager.
		CallAfter([this, initial_action]() { RunInitialAction(initial_action); });
	}
}

void BlocksetManagerDialog::RunInitialAction(InitialAction action)
{
	wxCommandEvent dummy;
	if (action == InitialAction::ADD)
	{
		OnAdd(dummy);
		if (m_changed)
		{
			// Close and hand back the new blockset, as if it had been double-clicked.
			const auto selection = GetSelection();
			if (selection.valid && selection.kind == Kind::BLOCKSET)
			{
				m_to_open = selection.name;
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

BlocksetManagerDialog::~BlocksetManagerDialog()
{
	m_tree->Disconnect(wxEVT_TREE_SEL_CHANGED, wxTreeEventHandler(BlocksetManagerDialog::OnSelectionChanged), nullptr, this);
	m_tree->Disconnect(wxEVT_TREE_ITEM_ACTIVATED, wxTreeEventHandler(BlocksetManagerDialog::OnItemActivated), nullptr, this);
	m_add->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(BlocksetManagerDialog::OnAdd), nullptr, this);
	m_add_set->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(BlocksetManagerDialog::OnAddSet), nullptr, this);
	m_import->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(BlocksetManagerDialog::OnImport), nullptr, this);
	m_export->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(BlocksetManagerDialog::OnExport), nullptr, this);
	m_remove->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(BlocksetManagerDialog::OnRemove), nullptr, this);
	m_move_up->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(BlocksetManagerDialog::OnMoveUp), nullptr, this);
	m_move_down->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(BlocksetManagerDialog::OnMoveDown), nullptr, this);
	m_rename->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(BlocksetManagerDialog::OnRename), nullptr, this);
}

void BlocksetManagerDialog::PopulateTree(const std::string& select)
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
		const auto tileset_node = m_tree->AppendItem(root, wxString::FromUTF8(tileset->GetName()), -1, -1,
			new BlocksetTreeItemData(static_cast<int>(Kind::TILESET), index, 0, 0, tileset->GetName()));
		if (!first.IsOk())
		{
			first = tileset_node;
		}

		// Count the sets first: with only one, its blocksets hang straight off the tileset
		// rather than behind a node that would say nothing.
		std::vector<uint8_t> sets;
		for (uint8_t primary = 0; primary < Landstalker::RoomData::MAX_PRIMARY_SETS; ++primary)
		{
			if (room_data->HasBlocksetGroup(index, primary))
			{
				sets.push_back(primary);
			}
		}

		for (const auto primary : sets)
		{
			wxTreeItemId parent = tileset_node;
			if (sets.size() > 1)
			{
				parent = m_tree->AppendItem(tileset_node, wxString::Format("Set %d", primary), -1, -1,
					new BlocksetTreeItemData(static_cast<int>(Kind::GROUP), index, primary, 0, std::string()));
			}

			const auto group = room_data->GetBlocksetGroup(index, primary);
			for (std::size_t sec = 0; sec < group.size(); ++sec)
			{
				// Asked of Labels directly rather than through GetBlocksetDisplayName, which
				// synthesises a name from the slot numbers when none is set - that would say
				// nothing the tree does not already show, on every single row.
				const auto display = Landstalker::Labels::Get(Landstalker::Labels::C_BLOCKSETS,
					(index << 16) | (primary << 8) | static_cast<int>(sec));
				auto label = wxString::FromUTF8(group[sec]->GetName());
				if (display && !display->empty())
				{
					label += " (" + wxString(*display) + ")";
				}
				label += sec == 0 ? wxString("  - base")
					: wxString::Format("  - alternate %d", static_cast<int>(sec) - 1);

				const auto node = m_tree->AppendItem(parent, label, -1, -1,
					new BlocksetTreeItemData(static_cast<int>(Kind::BLOCKSET), index, primary,
						static_cast<uint8_t>(sec), group[sec]->GetName()));
				if (group[sec]->GetName() == select)
				{
					to_select = node;
				}
			}
			if (sets.size() > 1)
			{
				m_tree->Expand(parent);
			}
		}
		if (tileset->GetName() == select)
		{
			to_select = tileset_node;
		}
	}
	m_tree->Thaw();

	if (!to_select.IsOk())
	{
		to_select = first;
	}
	if (to_select.IsOk())
	{
		// Expanding the whole tree would bury the selection under 115 entries.
		m_tree->SelectItem(to_select);
		m_tree->EnsureVisible(to_select);
	}
	PopulateDetails();
	PopulatePreview();
	UpdateUI();
}

BlocksetManagerDialog::Selection BlocksetManagerDialog::GetSelection() const
{
	Selection selection;
	const auto item = m_tree->GetSelection();
	if (!item.IsOk())
	{
		return selection;
	}
	const auto* data = dynamic_cast<BlocksetTreeItemData*>(m_tree->GetItemData(item));
	if (data == nullptr)
	{
		return selection;
	}
	selection.valid = true;
	selection.kind = static_cast<Kind>(data->GetKind());
	selection.tileset = data->GetTileset();
	selection.primary = data->GetPrimary();
	selection.sec = data->GetSec();
	selection.name = data->GetName();
	selection.is_base = selection.kind == Kind::BLOCKSET && selection.sec == 0;
	return selection;
}

std::pair<std::size_t, std::size_t> BlocksetManagerDialog::CombinedBudget(
	uint8_t tileset, uint8_t primary, uint8_t sec) const
{
	const auto room_data = m_gd->GetRoomData();
	const auto group = room_data->GetBlocksetGroup(tileset, primary);
	if (group.empty())
	{
		return { 0, Landstalker::RoomData::MAX_COMBINED_BLOCKS };
	}
	const auto base = group.front()->GetData()->size();
	// A room draws the base plus one alternate, so the total to measure is this entry
	// together with the base - or, for the base itself, with the largest alternate, since
	// that is the worst case any room in the set can reach.
	std::size_t other = 0;
	if (sec == 0)
	{
		for (std::size_t i = 1; i < group.size(); ++i)
		{
			other = std::max(other, group[i]->GetData()->size());
		}
		return { base + other, Landstalker::RoomData::MAX_COMBINED_BLOCKS };
	}
	const auto own = sec < group.size() ? group[sec]->GetData()->size() : 0;
	return { base + own, Landstalker::RoomData::MAX_COMBINED_BLOCKS };
}

std::shared_ptr<Landstalker::Palette> BlocksetManagerDialog::PreviewPalette(uint8_t tileset) const
{
	const auto room_data = m_gd->GetRoomData();
	const auto entry = room_data->GetTileset(tileset);
	if (entry)
	{
		const auto& preferred = entry->GetDefaultPalette();
		if (!preferred.empty())
		{
			const auto palette = m_gd->GetPalette(preferred);
			if (palette)
			{
				return palette->GetData();
			}
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

void BlocksetManagerDialog::PopulatePreview()
{
	const auto selection = GetSelection();
	const bool previewable = selection.valid && selection.kind == Kind::BLOCKSET;
	if (previewable && selection.name == m_previewed)
	{
		return;
	}
	m_previewed = previewable ? selection.name : std::string();

	const auto show_message = [this](const wxString& message)
	{
		m_preview->SetBitmap(wxNullBitmap);
		m_preview->Hide();
		m_preview_message->SetLabel(message);
		m_preview_message->Show();
		Layout();
	};

	if (!previewable)
	{
		show_message("Select a blockset to preview it.");
		return;
	}

	const auto room_data = m_gd->GetRoomData();
	const auto entry = room_data->GetBlockset(selection.tileset, selection.primary, selection.sec);
	const auto tileset_entry = room_data->GetTileset(selection.tileset);
	const auto palette = PreviewPalette(selection.tileset);
	if (!entry || !entry->GetData() || !tileset_entry || !tileset_entry->GetData() || !palette)
	{
		show_message("Preview unavailable.");
		return;
	}

	const auto& blocks = *entry->GetData();
	if (blocks.empty())
	{
		show_message("This blockset is empty.");
		return;
	}

	const auto tiles = tileset_entry->GetData();
	const int block_w = static_cast<int>(Landstalker::MapBlock::GetBlockWidth() * tiles->GetTileWidth());
	const int block_h = static_cast<int>(Landstalker::MapBlock::GetBlockHeight() * tiles->GetTileHeight());
	const int columns = static_cast<int>(std::min(blocks.size(), PREVIEW_COLUMNS));
	const int rows = static_cast<int>((blocks.size() + PREVIEW_COLUMNS - 1) / PREVIEW_COLUMNS);
	if (block_w <= 0 || block_h <= 0)
	{
		show_message("Preview unavailable.");
		return;
	}

	ImageBufferWx buffer(columns * block_w, rows * block_h);
	for (std::size_t i = 0; i < blocks.size(); ++i)
	{
		buffer.InsertBlock(static_cast<int>(i % PREVIEW_COLUMNS) * block_w,
			static_cast<int>(i / PREVIEW_COLUMNS) * block_h, 0, blocks[i], *tiles);
	}
	wxImage image = buffer.MakeImage({ palette });

	const double scale = std::min({ 1.0,
		static_cast<double>(PREVIEW_SIZE.GetWidth()) / image.GetWidth(),
		static_cast<double>(PREVIEW_SIZE.GetHeight()) / image.GetHeight() });
	if (scale < 1.0)
	{
		image.Rescale(std::max(1, static_cast<int>(image.GetWidth() * scale)),
			std::max(1, static_cast<int>(image.GetHeight() * scale)), wxIMAGE_QUALITY_NORMAL);
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

void BlocksetManagerDialog::SetDetail(std::size_t row, const wxString& caption, const wxString& value)
{
	if (row >= DETAIL_ROWS)
	{
		return;
	}
	m_detail_captions[row]->SetLabel(caption);
	m_detail_values[row]->SetLabel(value);
}

wxString BlocksetManagerDialog::DescribeRooms(const std::vector<uint16_t>& rooms) const
{
	const auto room_data = m_gd->GetRoomData();
	wxString described;
	std::size_t named = 0;
	for (; named < rooms.size() && named < 3; ++named)
	{
		if (!described.IsEmpty())
		{
			described += ", ";
		}
		described += wxString(room_data->GetRoomDisplayName(rooms[named]));
	}
	if (rooms.size() > named)
	{
		described += wxString::Format(" and %d more", static_cast<int>(rooms.size() - named));
	}
	return described;
}

void BlocksetManagerDialog::PopulateDetails()
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

	if (selection.kind != Kind::BLOCKSET)
	{
		const auto tileset = room_data->GetTileset(selection.tileset);
		SetDetail(0, "Tileset", tileset ? wxString::FromUTF8(tileset->GetName()) : wxString("-"));
		if (selection.kind == Kind::GROUP)
		{
			const auto group = room_data->GetBlocksetGroup(selection.tileset, selection.primary);
			const auto budget = CombinedBudget(selection.tileset, selection.primary, 0);
			SetDetail(1, "Primary set", wxString::Format("%d", selection.primary));
			SetDetail(2, "Blocksets", wxString::Format("%d of %d",
				static_cast<int>(group.size()),
				static_cast<int>(Landstalker::RoomData::MAX_BLOCKSETS_PER_GROUP)));
			SetDetail(3, "Worst-case blocks", wxString::Format("%d of %d",
				static_cast<int>(budget.first), static_cast<int>(budget.second)));
			const auto rooms = room_data->GetRoomsUsingBlockset(selection.tileset, selection.primary, 0);
			SetDetail(4, "Rooms in set", rooms.empty()
				? wxString("None") : wxString::Format("%d", static_cast<int>(rooms.size())));
		}
		else
		{
			std::size_t sets = 0;
			std::size_t total = 0;
			for (uint8_t primary = 0; primary < Landstalker::RoomData::MAX_PRIMARY_SETS; ++primary)
			{
				const auto group = room_data->GetBlocksetGroup(selection.tileset, primary);
				if (!group.empty())
				{
					++sets;
					total += group.size();
				}
			}
			SetDetail(1, "Primary sets", wxString::Format("%d of %d", static_cast<int>(sets),
				static_cast<int>(Landstalker::RoomData::MAX_PRIMARY_SETS)));
			SetDetail(2, "Blocksets", wxString::Format("%d", static_cast<int>(total)));
		}
		Layout();
		return;
	}

	const auto entry = room_data->GetBlockset(selection.tileset, selection.primary, selection.sec);
	if (!entry || !entry->GetData())
	{
		Layout();
		return;
	}
	const auto budget = CombinedBudget(selection.tileset, selection.primary, selection.sec);
	SetDetail(0, "Role", selection.is_base
		? wxString("Base - drawn by every room in the set")
		: wxString::Format("Alternate %d", selection.sec - 1));
	SetDetail(1, "Blocks", wxString::Format("%d", static_cast<int>(entry->GetData()->size())));
	// The base is what every alternate is measured against, so the worst case is what
	// matters: a room can address only MAX_COMBINED_BLOCKS across the pair.
	SetDetail(2, selection.is_base ? "Worst-case blocks" : "Combined blocks",
		wxString::Format("%d of %d%s", static_cast<int>(budget.first), static_cast<int>(budget.second),
			budget.first >= budget.second ? "  (full)" : ""));

	const auto rooms = room_data->GetRoomsUsingBlockset(selection.tileset, selection.primary, selection.sec);
	SetDetail(3, selection.is_base ? "Rooms in set" : "Rooms selecting it",
		rooms.empty() ? wxString("None")
			: wxString::Format("%d (%s)", static_cast<int>(rooms.size()), DescribeRooms(rooms)));

	const auto tileset = room_data->GetTileset(selection.tileset);
	SetDetail(4, "Tileset", tileset
		? wxString::Format("%s, set %d", wxString::FromUTF8(tileset->GetName()), selection.primary)
		: wxString("-"));
	Layout();
}

void BlocksetManagerDialog::UpdateUI()
{
	const auto selection = GetSelection();
	const auto room_data = m_gd->GetRoomData();
	const bool is_blockset = selection.valid && selection.kind == Kind::BLOCKSET;
	const auto group = selection.valid
		? room_data->GetBlocksetGroup(selection.tileset, selection.primary)
		: std::vector<std::shared_ptr<Landstalker::BlocksetEntry>>();

	// Adding needs a group to add to; a bare tileset node names one only when it has a set.
	const bool has_group = selection.valid &&
		(selection.kind == Kind::TILESET
			? room_data->HasBlocksetGroup(selection.tileset, 0)
			: !group.empty());
	m_add->Enable(has_group && group.size() < Landstalker::RoomData::MAX_BLOCKSETS_PER_GROUP);

	// A tileset can be given its second set, and a tileset with none can be given its first.
	bool can_add_set = false;
	if (selection.valid)
	{
		for (uint8_t primary = 0; primary < Landstalker::RoomData::MAX_PRIMARY_SETS; ++primary)
		{
			if (!room_data->HasBlocksetGroup(selection.tileset, primary))
			{
				can_add_set = true;
			}
		}
	}
	m_add_set->Enable(can_add_set);

	m_import->Enable(is_blockset);
	m_export->Enable(is_blockset);
	m_rename->Enable(is_blockset);

	// An alternate can go once nothing selects it; the base can only go with its whole set.
	if (is_blockset && !selection.is_base)
	{
		m_remove->Enable(room_data->GetRoomsUsingBlockset(
			selection.tileset, selection.primary, selection.sec).empty());
	}
	else if (selection.valid && (selection.kind == Kind::GROUP || selection.is_base))
	{
		m_remove->Enable(room_data->GetRoomsUsingBlockset(
			selection.tileset, selection.primary, 0).empty());
	}
	else
	{
		m_remove->Enable(false);
	}

	// Only the alternates are ordered; the base is not one of the choices.
	m_move_up->Enable(is_blockset && selection.sec > 1);
	m_move_down->Enable(is_blockset && selection.sec > 0 &&
		selection.sec + 1u < group.size());
}

void BlocksetManagerDialog::Move(int delta)
{
	const auto selection = GetSelection();
	if (!selection.valid || selection.kind != Kind::BLOCKSET || selection.sec == 0)
	{
		return;
	}
	const int new_sec = static_cast<int>(selection.sec) + delta;
	if (new_sec < 1)
	{
		return;
	}
	// Swap the two alternates' content rather than renumbering references: a room keeps its
	// sec_blockset, so the two alternates change places in the rooms that select them.
	if (m_gd->GetRoomData()->SwapBlockset(selection.tileset, selection.primary,
		selection.sec, static_cast<uint8_t>(new_sec)))
	{
		m_changed = true;
		PopulateTree(selection.name);
	}
}

void BlocksetManagerDialog::OnSelectionChanged(wxTreeEvent& /*evt*/)
{
	PopulateDetails();
	PopulatePreview();
	UpdateUI();
}

void BlocksetManagerDialog::OnItemActivated(wxTreeEvent& /*evt*/)
{
	const auto selection = GetSelection();
	if (!selection.valid || selection.kind != Kind::BLOCKSET)
	{
		return;
	}
	m_to_open = selection.name;
	EndModal(wxID_OK);
}

void BlocksetManagerDialog::OnAdd(wxCommandEvent& /*evt*/)
{
	const auto selection = GetSelection();
	if (!selection.valid)
	{
		return;
	}
	const auto room_data = m_gd->GetRoomData();
	// A bare tileset node means its only set.
	const uint8_t primary = selection.kind == Kind::TILESET ? 0 : selection.primary;
	if (!room_data->HasBlocksetGroup(selection.tileset, primary))
	{
		return;
	}
	const auto tileset = room_data->GetTileset(selection.tileset);
	const auto base = room_data->GetBlocksetGroup(selection.tileset, primary).front();
	const int remaining = static_cast<int>(Landstalker::RoomData::MAX_COMBINED_BLOCKS) -
		static_cast<int>(base->GetData()->size());

	NewBlocksetDialog dialog(this, "New Blockset",
		SuggestName(*room_data, (tileset ? tileset->GetName() : std::string("Blockset")) + "Alt"),
		"A new alternate for this set. Rooms will not draw it until one selects it, and the\n"
		"blocks start out blank - edit them in the blockset editor once it exists.",
		remaining);
	while (dialog.ShowModal() == wxID_OK)
	{
		const auto name = dialog.GetEnteredName();
		if (!Landstalker::RoomData::IsValidTilesetName(name) || room_data->IsAssetNameInUse(name))
		{
			wxMessageBox("The name must be unique, start with a letter, only contain A-Z, a-z, "
				"0-9 and _, and be at most 30 characters.",
				"New Blockset", wxOK | wxICON_ERROR, this);
			continue;
		}
		const auto added = room_data->AddBlockset(selection.tileset, primary, name, dialog.GetBlockCount());
		if (!added)
		{
			wxMessageBox("Unable to create the blockset.", "New Blockset", wxOK | wxICON_ERROR, this);
			return;
		}
		m_changed = true;
		PopulateTree(name);
		return;
	}
}

void BlocksetManagerDialog::OnAddSet(wxCommandEvent& /*evt*/)
{
	const auto selection = GetSelection();
	if (!selection.valid)
	{
		return;
	}
	const auto room_data = m_gd->GetRoomData();
	uint8_t primary = Landstalker::RoomData::MAX_PRIMARY_SETS;
	for (uint8_t candidate = 0; candidate < Landstalker::RoomData::MAX_PRIMARY_SETS; ++candidate)
	{
		if (!room_data->HasBlocksetGroup(selection.tileset, candidate))
		{
			primary = candidate;
			break;
		}
	}
	if (primary >= Landstalker::RoomData::MAX_PRIMARY_SETS)
	{
		return;
	}
	const auto tileset = room_data->GetTileset(selection.tileset);

	NewBlocksetDialog dialog(this, "New Primary Set",
		SuggestName(*room_data, (tileset ? tileset->GetName() : std::string("Blockset")) + "Base"),
		wxString::Format(
			"Creates primary set %d for this tileset, starting with its base blockset. Rooms\n"
			"choose between a tileset's sets with their primary blockset field; each set has\n"
			"its own base and its own alternates.", primary),
		static_cast<int>(Landstalker::RoomData::MAX_COMBINED_BLOCKS));
	while (dialog.ShowModal() == wxID_OK)
	{
		const auto name = dialog.GetEnteredName();
		if (!Landstalker::RoomData::IsValidTilesetName(name) || room_data->IsAssetNameInUse(name))
		{
			wxMessageBox("The name must be unique, start with a letter, only contain A-Z, a-z, "
				"0-9 and _, and be at most 30 characters.",
				"New Primary Set", wxOK | wxICON_ERROR, this);
			continue;
		}
		if (!room_data->AddBlocksetGroup(selection.tileset, primary, name, dialog.GetBlockCount()))
		{
			wxMessageBox("Unable to create the primary set.", "New Primary Set",
				wxOK | wxICON_ERROR, this);
			return;
		}
		m_changed = true;
		PopulateTree(name);
		return;
	}
}

void BlocksetManagerDialog::OnImport(wxCommandEvent& /*evt*/)
{
	const auto selection = GetSelection();
	if (!selection.valid || selection.kind != Kind::BLOCKSET)
	{
		return;
	}
	const auto room_data = m_gd->GetRoomData();
	const auto entry = room_data->GetBlockset(selection.tileset, selection.primary, selection.sec);
	if (!entry || !entry->GetData())
	{
		return;
	}

	wxFileDialog fd(this, _("Import Blockset"), "", "",
		"All Supported Formats|*.cbs;*.csv|"
		"Compressed Blockset (*.cbs)|*.cbs|"
		"CSV (*.csv)|*.csv|"
		"All Files (*.*)|*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return;
	}
	const std::filesystem::path chosen(fd.GetPath().ToStdString());
	const auto extension = ToLower(chosen.extension().string());

	Landstalker::Blockset imported;
	if (extension == ".csv")
	{
		imported = Landstalker::BlocksetCmp::FromCsv(ReadTextFile(chosen.string()));
	}
	else
	{
		const auto bytes = Landstalker::ReadBytes(chosen.string());
		if (!bytes.empty())
		{
			Landstalker::BlocksetCmp::Decode(bytes.data(), bytes.size(), imported);
		}
	}
	if (imported.empty())
	{
		wxMessageBox("Unable to read blockset data from the selected file.",
			"Import Blockset", wxOK | wxICON_ERROR, this);
		return;
	}

	// Map blocks index into the base and the alternate concatenated, so a change to the
	// base shifts every alternate underneath every map in the set - a far bigger deal than
	// replacing an alternate, which only affects the rooms that select it.
	const auto group = room_data->GetBlocksetGroup(selection.tileset, selection.primary);
	const auto old_size = entry->GetData()->size();
	const auto base_size = group.empty() ? 0 : group.front()->GetData()->size();
	// What the import would leave a room addressing at worst: the base pairs with the
	// largest alternate, an alternate pairs with the base.
	std::size_t largest_alternate = 0;
	for (std::size_t i = 1; i < group.size(); ++i)
	{
		largest_alternate = std::max(largest_alternate, group[i]->GetData()->size());
	}
	const auto combined = selection.is_base
		? imported.size() + largest_alternate : base_size + imported.size();

	wxString warning;
	if (combined > Landstalker::RoomData::MAX_COMBINED_BLOCKS)
	{
		wxMessageBox(wxString::Format(
			"The imported blockset has %d blocks. Together with the rest of this set that comes "
			"to %d, and a room can only address %d - map blocks are stored in ten bits.\n\n"
			"Nothing has been changed.",
			static_cast<int>(imported.size()), static_cast<int>(combined),
			static_cast<int>(Landstalker::RoomData::MAX_COMBINED_BLOCKS)),
			"Import Blockset", wxOK | wxICON_ERROR, this);
		return;
	}
	if (selection.is_base && imported.size() != old_size)
	{
		const auto rooms = room_data->GetRoomsUsingBlockset(selection.tileset, selection.primary, 0);
		warning = wxString::Format(
			"This is the base blockset, and the import changes its size from %d blocks to %d.\n\n"
			"Every room in this set draws the base followed by its own alternate, so the "
			"alternates' blocks all shift by %d. The maps of %d room%s will draw the wrong "
			"blocks until they are corrected.\n\nImport anyway?",
			static_cast<int>(old_size), static_cast<int>(imported.size()),
			static_cast<int>(imported.size()) - static_cast<int>(old_size),
			static_cast<int>(rooms.size()), rooms.size() == 1 ? "" : "s");
	}
	else if (imported.size() < old_size)
	{
		const auto rooms = room_data->GetRoomsUsingBlockset(
			selection.tileset, selection.primary, selection.sec);
		if (!rooms.empty())
		{
			warning = wxString::Format(
				"The imported blockset has %d blocks, fewer than the %d it replaces. Maps in "
				"%d room%s may refer to blocks that no longer exist.\n\nImport anyway?",
				static_cast<int>(imported.size()), static_cast<int>(old_size),
				static_cast<int>(rooms.size()), rooms.size() == 1 ? "" : "s");
		}
	}
	if (!warning.IsEmpty() &&
		wxMessageBox(warning, "Import Blockset", wxYES_NO | wxNO_DEFAULT | wxICON_WARNING, this) != wxYES)
	{
		return;
	}

	*entry->GetData() = imported;
	m_changed = true;
	// Force the preview to redraw even though the selection has not moved.
	m_previewed.clear();
	PopulateDetails();
	PopulatePreview();
	UpdateUI();
}

void BlocksetManagerDialog::OnExport(wxCommandEvent& /*evt*/)
{
	const auto selection = GetSelection();
	if (!selection.valid || selection.kind != Kind::BLOCKSET)
	{
		return;
	}
	const auto room_data = m_gd->GetRoomData();
	const auto entry = room_data->GetBlockset(selection.tileset, selection.primary, selection.sec);
	if (!entry || !entry->GetData())
	{
		return;
	}

	wxFileDialog fd(this, _("Export Blockset"), "", wxString::FromUTF8(selection.name + ".cbs"),
		"Compressed Blockset (*.cbs)|*.cbs|"
		"CSV (*.csv)|*.csv|"
		"PNG Image (*.png)|*.png",
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return;
	}

	const std::filesystem::path chosen(fd.GetPath().ToStdString());
	const auto extension = ToLower(chosen.extension().string());
	const auto& blocks = *entry->GetData();

	if (extension == ".csv")
	{
		std::ofstream out(chosen.string(), std::ios::binary);
		out << Landstalker::BlocksetCmp::ToCsv(blocks);
		if (!out)
		{
			wxMessageBox("Unable to write the blockset to the selected location.",
				"Export Blockset", wxOK | wxICON_ERROR, this);
		}
		return;
	}
	if (extension == ".png")
	{
		const auto tileset_entry = room_data->GetTileset(selection.tileset);
		const auto palette = PreviewPalette(selection.tileset);
		if (!tileset_entry || !tileset_entry->GetData() || !palette || blocks.empty())
		{
			wxMessageBox("No tileset and palette are available to render this blockset with.",
				"Export Blockset", wxOK | wxICON_ERROR, this);
			return;
		}
		const auto tiles = tileset_entry->GetData();
		const int block_w = static_cast<int>(Landstalker::MapBlock::GetBlockWidth() * tiles->GetTileWidth());
		const int block_h = static_cast<int>(Landstalker::MapBlock::GetBlockHeight() * tiles->GetTileHeight());
		const int columns = static_cast<int>(std::min(blocks.size(), PREVIEW_COLUMNS));
		const int rows = static_cast<int>((blocks.size() + PREVIEW_COLUMNS - 1) / PREVIEW_COLUMNS);
		ImageBufferWx buffer(columns * block_w, rows * block_h);
		for (std::size_t i = 0; i < blocks.size(); ++i)
		{
			buffer.InsertBlock(static_cast<int>(i % PREVIEW_COLUMNS) * block_w,
				static_cast<int>(i / PREVIEW_COLUMNS) * block_h, 0, blocks[i], *tiles);
		}
		buffer.WritePNG(chosen.string(), { palette });
		return;
	}

	Landstalker::ByteVector bytes(65536);
	const auto length = Landstalker::BlocksetCmp::Encode(blocks, bytes.data(), bytes.size());
	if (length == 0)
	{
		wxMessageBox("Unable to compress the blockset.", "Export Blockset", wxOK | wxICON_ERROR, this);
		return;
	}
	bytes.resize(length);
	Landstalker::WriteBytes(bytes, chosen.string());
}

void BlocksetManagerDialog::OnRemove(wxCommandEvent& /*evt*/)
{
	const auto selection = GetSelection();
	if (!selection.valid || selection.kind == Kind::TILESET)
	{
		return;
	}
	const auto room_data = m_gd->GetRoomData();

	// The base cannot go on its own - every room in the set draws it - so removing it means
	// removing the whole set.
	if (selection.kind == Kind::GROUP || selection.is_base)
	{
		const auto rooms = room_data->GetRoomsUsingBlockset(selection.tileset, selection.primary, 0);
		const auto tileset = room_data->GetTileset(selection.tileset);
		const auto tileset_name = tileset ? wxString::FromUTF8(tileset->GetName()) : wxString("?");
		if (!rooms.empty())
		{
			wxMessageBox(wxString::Format(
				"Set %d of '%s' cannot be deleted: %d room%s still use%s it.\n\n  %s\n\n"
				"Point those rooms at another tileset or set first.",
				selection.primary, tileset_name, static_cast<int>(rooms.size()),
				rooms.size() == 1 ? "" : "s", rooms.size() == 1 ? "s" : "", DescribeRooms(rooms)),
				"Remove Blockset Set", wxOK | wxICON_ERROR, this);
			return;
		}
		const auto group = room_data->GetBlocksetGroup(selection.tileset, selection.primary);
		if (wxMessageBox(wxString::Format(
			"Delete set %d of '%s', including its base and %d alternate%s?\n\n"
			"This cannot be undone. The blocksets will be removed from the game data, although "
			"the existing binary files will be left on disk.",
			selection.primary, tileset_name, static_cast<int>(group.size()) - 1,
			group.size() == 2 ? "" : "s"),
			"Remove Blockset Set", wxYES_NO | wxNO_DEFAULT | wxICON_WARNING, this) != wxYES)
		{
			return;
		}
		if (!room_data->DeleteBlocksetGroup(selection.tileset, selection.primary))
		{
			wxMessageBox("Unable to delete the selected set.", "Remove Blockset Set",
				wxOK | wxICON_ERROR, this);
			return;
		}
		m_changed = true;
		PopulateTree(tileset ? tileset->GetName() : std::string());
		return;
	}

	const auto rooms = room_data->GetRoomsUsingBlockset(selection.tileset, selection.primary, selection.sec);
	const auto name = wxString::FromUTF8(selection.name);
	if (!rooms.empty())
	{
		wxMessageBox(wxString::Format(
			"'%s' cannot be deleted: %d room%s still select%s it.\n\n  %s\n\n"
			"Point those rooms at another blockset first.",
			name, static_cast<int>(rooms.size()), rooms.size() == 1 ? "" : "s",
			rooms.size() == 1 ? "s" : "", DescribeRooms(rooms)),
			"Remove Blockset", wxOK | wxICON_ERROR, this);
		return;
	}
	if (wxMessageBox(wxString::Format(
		"Delete blockset '%s'?\n\n"
		"This cannot be undone. Every alternate above it moves down a slot, and the rooms "
		"selecting those slots are renumbered to follow.", name),
		"Remove Blockset", wxYES_NO | wxNO_DEFAULT | wxICON_WARNING, this) != wxYES)
	{
		return;
	}

	// Work out what to land on before the slots shift under us.
	const auto group = room_data->GetBlocksetGroup(selection.tileset, selection.primary);
	std::string next_selection;
	if (selection.sec + 1u < group.size())
	{
		next_selection = group[selection.sec + 1u]->GetName();
	}
	else if (selection.sec > 0)
	{
		next_selection = group[selection.sec - 1u]->GetName();
	}

	if (!room_data->DeleteBlockset(selection.tileset, selection.primary, selection.sec))
	{
		wxMessageBox("Unable to delete the selected blockset.", "Remove Blockset",
			wxOK | wxICON_ERROR, this);
		return;
	}
	m_changed = true;
	PopulateTree(next_selection);
}

void BlocksetManagerDialog::OnMoveUp(wxCommandEvent& /*evt*/)
{
	Move(-1);
}

void BlocksetManagerDialog::OnMoveDown(wxCommandEvent& /*evt*/)
{
	Move(1);
}

void BlocksetManagerDialog::OnRename(wxCommandEvent& /*evt*/)
{
	const auto selection = GetSelection();
	if (!selection.valid || selection.kind != Kind::BLOCKSET)
	{
		return;
	}
	const auto room_data = m_gd->GetRoomData();
	const int label_id = (selection.tileset << 16) | (selection.primary << 8) | selection.sec;
	const auto old_name = selection.name;
	const auto old_display = room_data->GetBlocksetDisplayName(
		selection.tileset, selection.primary, selection.sec);

	RenameBlocksetDialog dialog(this, old_name, old_display);
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
				"Rename Blockset", wxOK | wxICON_ERROR, this);
			continue;
		}
		if (display_changed &&
			!Landstalker::Labels::IsValid(display, Landstalker::Labels::C_BLOCKSETS, label_id))
		{
			wxMessageBox("The display name must not be empty, must be unique, and must not contain "
				"non-printable characters.", "Rename Blockset", wxOK | wxICON_ERROR, this);
			continue;
		}

		if (name_changed && !room_data->RenameBlockset(old_name, name))
		{
			wxMessageBox("Unable to rename the selected blockset.", "Rename Blockset",
				wxOK | wxICON_ERROR, this);
			return;
		}
		if (display_changed)
		{
			Landstalker::Labels::Update(Landstalker::Labels::C_BLOCKSETS, label_id, display);
		}
		m_changed = true;
		PopulateTree(name);
		return;
	}
}
