#include <blockset/BlocksetEditorCtrl.h>

#include <algorithm>
#include <wx/wx.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/dcbuffer.h>
#include <main/EditorFrame.h>
#include <main/ImageBufferWx.h>

wxBEGIN_EVENT_TABLE(BlocksetEditorCtrl, wxVScrolledWindow)
EVT_PAINT(BlocksetEditorCtrl::OnPaint)
EVT_SIZE(BlocksetEditorCtrl::OnSize)
EVT_LEFT_DOWN(BlocksetEditorCtrl::OnMouseDown)
EVT_LEFT_DCLICK(BlocksetEditorCtrl::OnMouseDown)
EVT_RIGHT_DOWN(BlocksetEditorCtrl::OnMouseDown)
EVT_LEFT_UP(BlocksetEditorCtrl::OnMouseUp)
EVT_RIGHT_UP(BlocksetEditorCtrl::OnMouseUp)
EVT_MOUSE_CAPTURE_LOST(BlocksetEditorCtrl::OnCaptureLost)
EVT_MOTION(BlocksetEditorCtrl::OnMouseMove)
EVT_LEAVE_WINDOW(BlocksetEditorCtrl::OnMouseLeave)
wxEND_EVENT_TABLE()

wxDEFINE_EVENT(EVT_BLOCK_SELECT, wxCommandEvent);
wxDEFINE_EVENT(EVT_BLOCK_HOVER, wxCommandEvent);
wxDEFINE_EVENT(EVT_BLOCK_EDIT_REQUEST, wxCommandEvent);
wxDEFINE_EVENT(EVT_BLOCK_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_BLOCK_TILE_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_BLOCK_ACTIVATE, wxCommandEvent);
wxDEFINE_EVENT(EVT_TILE_SELECT, wxCommandEvent);

BlocksetEditorCtrl::BlocksetEditorCtrl(EditorFrame* parent)
	: wxVScrolledWindow(parent, wxID_ANY),
	  m_blocks(std::make_shared<Landstalker::Blockset>()),
	  m_mode(Mode::BLOCK_SELECT),
	  m_columns(0),
	  m_rows(0),
	  m_pixelsize(4),
	  m_selectedblock(-1),
	  m_hoveredblock(-1),
	  m_selectedtile(-1),
	  m_hoveredtile(-1),
	  block_width(2),
	  block_height(2),
	  m_cellwidth(0),
	  m_cellheight(0),
	  m_tilewidth(0),
	  m_tileheight(0),
	  m_ctrlwidth(0),
	  m_ctrlheight(0),
	  m_enableblocknumbers(false),
	  m_enabletilenumbers(false),
	  m_enableborders(true),
	  m_enabletileborders(true),
	  m_enableselection(true),
	  m_enablehover(true),
	  m_enablealpha(true),
	  m_drawtile(0),
	  m_frame(parent)
{
	SetRowCount(m_rows);
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE));

	InitialiseBrushesAndPens();
}

BlocksetEditorCtrl::~BlocksetEditorCtrl()
{
}

void BlocksetEditorCtrl::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	m_gd = gd;
}

void BlocksetEditorCtrl::ClearGameData()
{
	m_gd = nullptr;
}

bool BlocksetEditorCtrl::Open(const std::string& name)
{
	if (m_gd == nullptr)
	{
		return false;
	}
	m_blockset_entry = m_gd->GetRoomData()->GetBlockset(name);
	m_blocks = m_blockset_entry->GetData();
	auto tse = m_gd->GetRoomData()->GetTileset(m_blockset_entry->GetTileset());
	m_tileset = tse->GetData();
	m_pal_name = tse->GetDefaultPalette();
	m_pal = m_gd->GetPalette(m_pal_name)->GetData();
	m_mode = Mode::BLOCK_SELECT;
	m_selectedtile = -1;
	m_selectedblock = -1;
	m_hoveredtile = -1;
	m_hoveredblock = -1;
	m_drawtile = Landstalker::Tile();
	ClearHistory();
	UpdateRowCount();
	ForceRedraw();
	return true;
}

bool BlocksetEditorCtrl::OpenRoom(uint16_t roomnum)
{
	if (m_gd == nullptr)
	{
		return false;
	}
	m_selectedblock = -1;
	m_hoveredblock = -1;
	m_selectedtile = -1;
	m_hoveredtile = -1;
	m_mode = Mode::BLOCK_SELECT;
	m_drawtile = Landstalker::Tile();
	m_blocks = m_gd->GetRoomData()->GetCombinedBlocksetForRoom(roomnum);
	auto tse = m_gd->GetRoomData()->GetTilesetForRoom(roomnum);
	m_tileset = tse->GetData();
	m_pal_name = m_gd->GetRoomData()->GetPaletteForRoom(roomnum)->GetName();
	m_pal = m_gd->GetRoomData()->GetPaletteForRoom(roomnum)->GetData();
	m_enablealpha = false;
	m_enabletileborders = false;
	m_pixelsize = 2;
	ClearHistory();
	UpdateRowCount();
	ForceRedraw();
	return true;
}

void BlocksetEditorCtrl::RedrawTiles(int index)
{
	if ((index < 0) || (index >= static_cast<int>(m_tileset->GetTileCount())))
	{
		ForceRedraw();
	}
	else
	{
		// The tile's artwork changed elsewhere (e.g. edited in the tiles pane), so the
		// cached blockset bitmap is stale wherever that tile appears.
		m_tiles_bmp_dirty = true;
		Refresh(false);
	}
}

void BlocksetEditorCtrl::RedrawBlock(int index)
{
	if ((index < 0) || (index >= static_cast<int>(m_blocks->size())))
	{
		ForceRedraw();
	}
	else
	{
		// Block content changed, so the cached bitmap is stale for that cell.
		m_tiles_bmp_dirty = true;
		RefreshBlock(index);
	}
}

void BlocksetEditorCtrl::SetPixelSize(int n)
{
	m_pixelsize = n;
	UpdateRowCount();
	ForceRedraw();
}

int BlocksetEditorCtrl::GetPixelSize() const
{
	return m_pixelsize;
}

void BlocksetEditorCtrl::SetActivePalette(const std::string& name)
{
	if (m_pal_name != name)
	{
		m_pal_name = name;
		m_pal = m_gd->GetPalette(name)->GetData();
		ForceRedraw();
	}
}

std::string BlocksetEditorCtrl::GetActivePalette() const
{
	return m_pal_name;
}

int BlocksetEditorCtrl::GetBlockmapSize() const
{
	return m_blocks ? m_blocks->size() : 0;
}

int BlocksetEditorCtrl::GetBlockWidth() const
{
	return Landstalker::MapBlock::GetBlockWidth();
}

int BlocksetEditorCtrl::GetBlockHeight() const
{
	return Landstalker::MapBlock::GetBlockHeight();
}

std::shared_ptr<Landstalker::Tileset> BlocksetEditorCtrl::GetTileset()
{
	return m_tileset;
}

std::shared_ptr<Landstalker::Palette> BlocksetEditorCtrl::GetPalette()
{
	return m_pal;
}

std::shared_ptr<std::vector<Landstalker::MapBlock>> BlocksetEditorCtrl::GetBlocks()
{
	return m_blocks;
}

bool BlocksetEditorCtrl::IsShapeMode(Mode mode)
{
	return (mode == Mode::LINE) || (mode == Mode::RECTANGLE_OUTLINE) ||
	       (mode == Mode::RECTANGLE_FILLED) || (mode == Mode::CIRCLE_OUTLINE) ||
	       (mode == Mode::CIRCLE_FILLED);
}

bool BlocksetEditorCtrl::IsDrawMode(Mode mode)
{
	return (mode == Mode::PENCIL) || (mode == Mode::FILL) || IsShapeMode(mode);
}

void BlocksetEditorCtrl::SetMode(const Mode& mode)
{
	if (m_mode != mode)
	{
		// Switching mode confirms a pending paste and drops the box selection.
		if (m_sel_drag != SelDrag::None)
		{
			CancelBoxDrag();
		}
		if (m_shape_active)
		{
			CancelShapeDrag();
		}
		ClearBoxSelection(true);
		m_mode = mode;
		m_selectedtile = IsBlockSelectionValid() ? 0 : -1;
		m_selectedblock = IsBlockSelectionValid() ? m_selectedblock : -1;
		Refresh();
	}
}

BlocksetEditorCtrl::Mode BlocksetEditorCtrl::GetMode() const
{
	return m_mode;
}

void BlocksetEditorCtrl::SetDrawTile(const Landstalker::Tile& tile)
{
	m_drawtile = tile;
	// The hover and shape previews render the draw tile, so cycling it with +/- (or
	// picking a new one) has to repaint them to be visible.
	if (IsDrawMode(m_mode) && (m_hoveredblock != -1))
	{
		RefreshBlock(m_hoveredblock);
	}
	if (m_shape_active)
	{
		RefreshShapeRect();
	}
	RefreshStatusbar();
}

Landstalker::Tile BlocksetEditorCtrl::GetDrawTile() const
{
	return m_drawtile;
}

void BlocksetEditorCtrl::ToggleHFlip(int block_idx, int tile_idx)
{
	if (IsTileIndexValid(tile_idx) && IsBlockIndexValid(block_idx))
	{
		auto tile = GetTile(block_idx, tile_idx);
		tile.Attributes().toggleAttribute(Landstalker::TileAttributes::Attribute::ATTR_HFLIP);
		SetTile(block_idx, tile_idx, tile);
	}
}

void BlocksetEditorCtrl::ToggleVFlip(int block_idx, int tile_idx)
{
	if (IsTileIndexValid(tile_idx) && IsBlockIndexValid(block_idx))
	{
		auto tile = GetTile(block_idx, tile_idx);
		tile.Attributes().toggleAttribute(Landstalker::TileAttributes::Attribute::ATTR_VFLIP);
		SetTile(block_idx, tile_idx, tile);
	}
}

void BlocksetEditorCtrl::TogglePriority(int block_idx, int tile_idx)
{
	if (IsTileIndexValid(tile_idx) && IsBlockIndexValid(block_idx))
	{
		auto tile = GetTile(block_idx, tile_idx);
		tile.Attributes().toggleAttribute(Landstalker::TileAttributes::Attribute::ATTR_PRIORITY);
		SetTile(block_idx, tile_idx, tile);
	}
}

void BlocksetEditorCtrl::ToggleSelectedHFlip()
{
	if (IsTileSelectionValid())
	{
		auto tile = GetSelectedTile();
		tile.Attributes().toggleAttribute(Landstalker::TileAttributes::Attribute::ATTR_HFLIP);
		SetSelectedTile(tile);
	}
}

void BlocksetEditorCtrl::ToggleSelectedVFlip()
{
	if (IsTileSelectionValid())
	{
		auto tile = GetSelectedTile();
		tile.Attributes().toggleAttribute(Landstalker::TileAttributes::Attribute::ATTR_VFLIP);
		SetSelectedTile(tile);
	}
}

void BlocksetEditorCtrl::ToggleSelectedPriority()
{
	if (IsTileSelectionValid())
	{
		auto tile = GetSelectedTile();
		tile.Attributes().toggleAttribute(Landstalker::TileAttributes::Attribute::ATTR_PRIORITY);
		SetSelectedTile(tile);
	}
}

void BlocksetEditorCtrl::SetSelectedHFlip(bool hflip)
{
	if (IsTileSelectionValid())
	{
		auto tile = GetSelectedTile();
		if (hflip)
		{
			tile.Attributes().setAttribute(Landstalker::TileAttributes::Attribute::ATTR_HFLIP);
		}
		else
		{
			tile.Attributes().clearAttribute(Landstalker::TileAttributes::Attribute::ATTR_HFLIP);
		}
		SetSelectedTile(tile);
	}
}

void BlocksetEditorCtrl::SetSelectedVFlip(bool vflip)
{
	if (IsTileSelectionValid())
	{
		auto tile = GetSelectedTile();
		if (vflip)
		{
			tile.Attributes().setAttribute(Landstalker::TileAttributes::Attribute::ATTR_VFLIP);
		}
		else
		{
			tile.Attributes().clearAttribute(Landstalker::TileAttributes::Attribute::ATTR_VFLIP);
		}
		SetSelectedTile(tile);
	}
}

void BlocksetEditorCtrl::SetSelectedPriority(bool priority)
{
	if (IsTileSelectionValid())
	{
		auto tile = GetSelectedTile();
		if (priority)
		{
			tile.Attributes().setAttribute(Landstalker::TileAttributes::Attribute::ATTR_PRIORITY);
		}
		else
		{
			tile.Attributes().clearAttribute(Landstalker::TileAttributes::Attribute::ATTR_PRIORITY);
		}
		SetSelectedTile(tile);
	}
}

bool BlocksetEditorCtrl::GetSelectedHFlip() const
{
	if (IsTileSelectionValid())
	{
		return GetSelectedTile().Attributes().getAttribute(Landstalker::TileAttributes::Attribute::ATTR_HFLIP);
	}
	return false;
}

bool BlocksetEditorCtrl::GetSelectedVFlip() const
{
	if (IsTileSelectionValid())
	{
		return GetSelectedTile().Attributes().getAttribute(Landstalker::TileAttributes::Attribute::ATTR_VFLIP);
	}
	return false;
}

bool BlocksetEditorCtrl::GetSelectedPriority() const
{
	if (IsTileSelectionValid())
	{
		return GetSelectedTile().Attributes().getAttribute(Landstalker::TileAttributes::Attribute::ATTR_PRIORITY);
	}
	return false;
}

bool BlocksetEditorCtrl::GetTileNumbersEnabled() const
{
	return m_enableblocknumbers;
}

void BlocksetEditorCtrl::SetTileNumbersEnabled(bool enabled)
{
	m_enableblocknumbers = enabled;
	ForceRedraw();
}

bool BlocksetEditorCtrl::GetSelectionEnabled() const
{
	return m_enableselection;
}

void BlocksetEditorCtrl::SetSelectionEnabled(bool enabled)
{
	m_enableselection = enabled;
	if (m_enableselection == false)
	{
		m_selectedblock = -1;
		m_selectedtile = -1;
	}
	ForceRedraw();
}

bool BlocksetEditorCtrl::GetHoverEnabled() const
{
	return m_enablehover;
}

void BlocksetEditorCtrl::SetHoverEnabled(bool enabled)
{
	m_enablehover = enabled;
	if (m_enablehover == false)
	{
		m_hoveredblock = -1;
		m_hoveredtile = -1;
	}
	ForceRedraw();
}

bool BlocksetEditorCtrl::GetAlphaEnabled() const
{
	return m_enablealpha;
}

void BlocksetEditorCtrl::SetAlphaEnabled(bool enabled)
{
	m_enablealpha = enabled;
	ForceRedraw();
}

bool BlocksetEditorCtrl::GetBordersEnabled() const
{
	return m_enableborders;
}

void BlocksetEditorCtrl::SetBordersEnabled(bool enabled)
{
	m_enableborders = enabled;
	ForceRedraw();
}

bool BlocksetEditorCtrl::InsertBlock(int row)
{
	if (m_blocks && row >= 0 && row <= static_cast<int>(m_blocks->size()))
	{
		PushUndo();
		m_blocks->insert(m_blocks->cbegin() + row, Landstalker::MapBlock());
		// The cached bitmap holds every block at its old position; a full re-render is due.
		m_tiles_bmp_dirty = true;
		UpdateRowCount();
		ForceRedraw();
		return true;
	}
	return false;
}

bool BlocksetEditorCtrl::DeleteBlock(int row)
{
	if (m_blocks && row >= 0 && row < static_cast<int>(m_blocks->size()))
	{
		PushUndo();
		m_blocks->erase(m_blocks->cbegin() + row);
		if (m_selectedblock >= static_cast<int>(m_blocks->size()))
		{
			m_selectedblock = static_cast<int>(m_blocks->size()) - 1;
		}
		m_tiles_bmp_dirty = true;
		UpdateRowCount();
		ForceRedraw();
		return true;
	}
	return false;
}

bool BlocksetEditorCtrl::CanUndo() const
{
	return !m_undo_stack.empty();
}

bool BlocksetEditorCtrl::CanRedo() const
{
	return !m_redo_stack.empty();
}

void BlocksetEditorCtrl::Undo()
{
	if (!m_blocks || m_undo_stack.empty())
	{
		return;
	}
	m_redo_stack.push_back(*m_blocks);
	auto state = std::move(m_undo_stack.back());
	m_undo_stack.pop_back();
	RestoreHistoryState(std::move(state));
}

void BlocksetEditorCtrl::Redo()
{
	if (!m_blocks || m_redo_stack.empty())
	{
		return;
	}
	m_undo_stack.push_back(*m_blocks);
	auto state = std::move(m_redo_stack.back());
	m_redo_stack.pop_back();
	RestoreHistoryState(std::move(state));
}

void BlocksetEditorCtrl::BeginUndoGroup()
{
	if (m_undo_group_depth++ == 0)
	{
		m_undo_group_pushed = false;
	}
}

void BlocksetEditorCtrl::EndUndoGroup()
{
	if (m_undo_group_depth > 0)
	{
		--m_undo_group_depth;
	}
}

void BlocksetEditorCtrl::PushUndo()
{
	if (!m_blocks)
	{
		return;
	}
	if (m_undo_group_depth > 0)
	{
		// Only the first mutation in a group snapshots; the rest belong to the same entry.
		if (m_undo_group_pushed)
		{
			return;
		}
		m_undo_group_pushed = true;
	}
	// A new edit invalidates anything that was undone.
	m_redo_stack.clear();
	m_undo_stack.push_back(*m_blocks);
	while (m_undo_stack.size() > 100)
	{
		m_undo_stack.pop_front();
	}
}

void BlocksetEditorCtrl::RestoreHistoryState(Landstalker::Blockset&& state)
{
	// Any box selection or pending float refers to content that is about to change.
	ResetBoxSelectionState();
	*m_blocks = std::move(state);
	// A restored state can have a different block count, invalidating selection and layout.
	if (m_selectedblock >= static_cast<int>(m_blocks->size()))
	{
		m_selectedblock = static_cast<int>(m_blocks->size()) - 1;
	}
	if (m_hoveredblock >= static_cast<int>(m_blocks->size()))
	{
		m_hoveredblock = -1;
		m_hoveredtile = -1;
	}
	m_tiles_bmp_dirty = true;
	UpdateRowCount();
	ForceRedraw();
	RefreshStatusbar();
}

void BlocksetEditorCtrl::ClearHistory()
{
	m_undo_stack.clear();
	m_redo_stack.clear();
	m_undo_group_depth = 0;
	m_undo_group_pushed = false;
	// Called when a blockset is (re)opened: any selection or float belongs to the old one.
	ResetBoxSelectionState();
}

void BlocksetEditorCtrl::PushUndoState(Landstalker::Blockset&& state)
{
	// A new edit invalidates anything that was undone.
	m_redo_stack.clear();
	m_undo_stack.push_back(std::move(state));
	while (m_undo_stack.size() > 100)
	{
		m_undo_stack.pop_front();
	}
}

bool BlocksetEditorCtrl::IsBlockSelectionValid() const
{
	return IsBlockIndexValid(m_selectedblock);
}

bool BlocksetEditorCtrl::IsBlockHoverValid() const
{
	return IsBlockIndexValid(m_hoveredblock);
}

bool BlocksetEditorCtrl::IsTileSelectionValid() const
{
	return IsBlockIndexValid(m_selectedblock) && IsTileIndexValid(m_selectedtile);
}

bool BlocksetEditorCtrl::IsTileHoverValid() const
{
	return IsBlockIndexValid(m_hoveredblock) && IsTileIndexValid(m_hoveredtile);
}

uint16_t BlocksetEditorCtrl::GetBlockSelection() const
{
	if (IsBlockSelectionValid())
	{
		return m_selectedblock;
	}
	else
	{
		return 0xFFFF;
	}
}

uint16_t BlocksetEditorCtrl::GetBlockHover() const
{
	if (IsBlockHoverValid())
	{
		return m_hoveredblock;
	}
	else
	{
		return 0xFFFF;
	}
}

void BlocksetEditorCtrl::SetBlockSelection(int block)
{
	int b = -1;
	if (m_mode != Mode::BLOCK_SELECT)
	{
		return;
	}
	if (IsBlockIndexValid(block))
	{
		b = block;
	}
	if (b != m_selectedblock)
	{
		const int old = m_selectedblock;
		m_selectedblock = b;
		if (m_selectedblock != -1)
		{
			std::size_t row = m_selectedblock / m_columns;
			if (row >= (GetVisibleRowsEnd() - 1) || row < GetVisibleRowsBegin())
			{
				ScrollToRow(row);
			}
		}
		FireBlockEvent(EVT_BLOCK_SELECT, "");
		RefreshStatusbar();
		RefreshBlock(old);
		RefreshBlock(m_selectedblock);
	}
}

void BlocksetEditorCtrl::SetTileHover(int block, int tile)
{
	int b = -1;
	int t = -1;
	if (m_mode == Mode::BLOCK_SELECT)
	{
		return;
	}
	if (IsBlockIndexValid(block) && IsTileIndexValid(tile))
	{
		b = block;
		t = tile;
	}
	if (b != m_hoveredblock || t != m_hoveredtile)
	{
		const int old = m_hoveredblock;
		m_hoveredblock = b;
		m_hoveredtile = t;
		if (m_hoveredblock != -1)
		{
			std::size_t row = m_hoveredblock / m_columns;
			if (row >= (GetVisibleRowsEnd() - 1) || row < GetVisibleRowsBegin())
			{
				ScrollToRow(row);
			}
		}
		RefreshStatusbar();
		RefreshBlock(old);
		RefreshBlock(m_hoveredblock);
	}
}

void BlocksetEditorCtrl::SetTileSelection(int block, int tile)
{
	int b = -1;
	int t = -1;
	if (m_mode != Mode::TILE_SELECT)
	{
		return;
	}
	if (IsBlockIndexValid(block) && IsTileIndexValid(tile))
	{
		b = block;
		t = tile;
	}
	if (b != m_selectedblock || t != m_selectedtile)
	{
		const int old = m_selectedblock;
		m_selectedblock = b;
		m_selectedtile = t;
		if (m_selectedblock != -1)
		{
			std::size_t row = m_selectedblock / m_columns;
			if (row >= (GetVisibleRowsEnd() - 1) || row < GetVisibleRowsBegin())
			{
				ScrollToRow(row);
			}
		}
		FireEvent(EVT_BLOCK_SELECT, "");
		RefreshStatusbar();
		RefreshBlock(old);
		RefreshBlock(m_selectedblock);
	}
}

uint16_t BlocksetEditorCtrl::GetTileSelection() const
{
	if (IsTileSelectionValid())
	{
		return m_selectedtile;
	}
	else
	{
		return 0xFFFF;
	}
}

uint16_t BlocksetEditorCtrl::GetTileHover() const
{
	if (IsTileHoverValid())
	{
		return m_hoveredtile;
	}
	else
	{
		return 0xFFFF;
	}
}

Landstalker::MapBlock BlocksetEditorCtrl::GetSelectedBlock() const
{
	if (IsBlockSelectionValid())
	{
		return m_blocks->at(m_selectedblock);
	}
	else
	{
		return Landstalker::MapBlock();
	}
}

Landstalker::Tile BlocksetEditorCtrl::GetSelectedTile() const
{
	if (m_blocks && IsTileSelectionValid())
	{
		return GetTile(m_selectedblock, m_selectedtile);
	}
	return Landstalker::Tile();
}

void BlocksetEditorCtrl::SetSelectedBlock(const Landstalker::MapBlock& block)
{
	if (IsBlockSelectionValid())
	{
		SetBlock(m_selectedblock, block);
	}
}

void BlocksetEditorCtrl::SetSelectedTile(const Landstalker::Tile& tile)
{
	if (IsTileSelectionValid() && tile != GetSelectedTile())
	{
		SetTile(m_selectedblock, m_selectedtile, tile);
	}
}

Landstalker::MapBlock BlocksetEditorCtrl::GetHoveredBlock() const
{
	if (IsBlockHoverValid())
	{
		return m_blocks->at(m_hoveredblock);
	}
	else
	{
		return Landstalker::MapBlock();
	}
}

Landstalker::Tile BlocksetEditorCtrl::GetHoveredTile() const
{
	if (IsTileHoverValid())
	{
		return GetTile(m_hoveredblock, m_hoveredtile);
	}
	else
	{
		return Landstalker::Tile();
	}
}

void BlocksetEditorCtrl::SetHoveredTile(const Landstalker::Tile& tile)
{
	if (IsTileHoverValid() && tile != GetHoveredTile())
	{
		SetTile(m_hoveredblock, m_hoveredtile, tile);
	}
}

Landstalker::MapBlock BlocksetEditorCtrl::GetBlock(int index) const
{
	if (m_blocks && IsBlockIndexValid(index))
	{
		return m_blocks->at(index);
	}
	return Landstalker::MapBlock();
}

Landstalker::Tile BlocksetEditorCtrl::GetTile(int block_idx, int tile_idx) const
{
	if (m_blocks && IsBlockIndexValid(block_idx) && IsTileIndexValid(tile_idx))
	{
		return m_blocks->at(block_idx).GetTile(tile_idx);
	}
	return Landstalker::Tile();
}

void BlocksetEditorCtrl::SetBlock(int block, const Landstalker::MapBlock& new_block)
{
	if (m_blocks && IsBlockIndexValid(block))
	{
		PushUndo();
		m_blocks->at(block) = new_block;
		m_tiles_bmp_dirty = true;
		RefreshBlock(block);
	}
}

void BlocksetEditorCtrl::SetTile(int block_idx, int tile_idx, const Landstalker::Tile& new_tile)
{
	if (m_blocks && IsBlockIndexValid(block_idx) && IsTileIndexValid(tile_idx))
	{
		PushUndo();
		m_blocks->at(block_idx).SetTile(tile_idx, new_tile);
		m_tiles_bmp_dirty = true;
		RefreshBlock(block_idx);
	}
}

bool BlocksetEditorCtrl::IsBlockIndexValid(int block_index) const
{
	return (m_blocks && (block_index >= 0) && (block_index < static_cast<int>(m_blocks->size())));
}

bool BlocksetEditorCtrl::IsTileIndexValid(int tile_index) const
{
	return (tile_index >= 0 && tile_index < static_cast<int>(Landstalker::MapBlock::GetBlockSize()));
}

int BlocksetEditorCtrl::GetControlBlockWidth() const
{
	return m_columns;
}

void BlocksetEditorCtrl::RefreshStatusbar()
{
	switch (m_mode)
	{
	case Mode::BLOCK_SELECT:
		if (IsBlockHoverValid())
		{
			FireUpdateStatusEvent(Landstalker::StrPrintf("Hovered: %04d", GetBlockHover()), 0);
		}
		else
		{
			FireUpdateStatusEvent("", 0);
		}
		if (IsBlockSelectionValid())
		{
			FireUpdateStatusEvent(Landstalker::StrPrintf("Selected: %04d", GetBlockSelection()), 1);
		}
		else
		{
			FireUpdateStatusEvent("", 1);
		}
		break;
	case Mode::TILE_SELECT:
	case Mode::PENCIL:
	case Mode::LINE:
	case Mode::RECTANGLE_OUTLINE:
	case Mode::RECTANGLE_FILLED:
	case Mode::CIRCLE_OUTLINE:
	case Mode::CIRCLE_FILLED:
	case Mode::FILL:
		if (IsBlockHoverValid())
		{
			const auto& tile = GetHoveredTile();
			FireUpdateStatusEvent(Landstalker::StrPrintf("Hovered: %04d:%01d (%03d%s%s%s)", GetBlockHover(), GetTileHover(),
				tile.GetIndex(), tile.Attributes().getAttribute(Landstalker::TileAttributes::Attribute::ATTR_HFLIP) ? "H" : "",
				tile.Attributes().getAttribute(Landstalker::TileAttributes::Attribute::ATTR_VFLIP) ? "V" : "",
				tile.Attributes().getAttribute(Landstalker::TileAttributes::Attribute::ATTR_PRIORITY) ? "P" : ""), 0);
		}
		else
		{
			FireUpdateStatusEvent("", 0);
		}
		if (m_mode == Mode::TILE_SELECT && IsTileSelectionValid())
		{
			const auto& tile = GetSelectedTile();
			FireUpdateStatusEvent(Landstalker::StrPrintf("Selected: %04d:%01d (%03d%s%s%s)", GetBlockSelection(), GetTileSelection(),
				tile.GetIndex(), tile.Attributes().getAttribute(Landstalker::TileAttributes::Attribute::ATTR_HFLIP) ? "H" : "",
				tile.Attributes().getAttribute(Landstalker::TileAttributes::Attribute::ATTR_VFLIP) ? "V" : "",
				tile.Attributes().getAttribute(Landstalker::TileAttributes::Attribute::ATTR_PRIORITY) ? "P" : ""), 1);
		}
		else if (IsDrawMode(m_mode) && m_drawtile >= 0)
		{
			FireUpdateStatusEvent(Landstalker::StrPrintf("Selected Tile: %03d", m_drawtile.GetIndex()), 1);
		}
		else
		{
			FireUpdateStatusEvent("", 1);
		}
		break;
	default:
		break;
	}
}

wxCoord BlocksetEditorCtrl::OnGetRowHeight(size_t /*row*/) const
{
	return wxCoord(m_pixelsize * m_tileset->GetTileHeight() * GetBlockHeight());
}

bool BlocksetEditorCtrl::UpdateRowCount()
{
	if (m_tileset == nullptr)
	{
		return false;
	}
	m_tilewidth = m_pixelsize * m_tileset->GetTileWidth();
	m_tileheight = m_pixelsize * m_tileset->GetTileHeight();
	m_cellwidth = m_tilewidth * GetBlockWidth();
	m_cellheight = m_tileheight * GetBlockHeight();
	int columns = std::max<int>(1, m_ctrlwidth / m_cellwidth);
	int rows = std::max<int>(1, (m_blocks->size() + columns - 1) / columns);
	if ((columns != m_columns) || (rows != m_rows))
	{
		m_columns = columns;
		m_rows = rows;
		SetRowCount(m_rows);
		return true;
	}
	return false;
}

void BlocksetEditorCtrl::RenderTilesBitmap()
{
	// The whole blockset at native resolution in one image: block content only changes on
	// explicit edits, so this renders once and every paint just blits from it. Building a
	// bitmap per tile per paint is what made opening the editor crawl.
	if ((m_tileset == nullptr) || (m_pal == nullptr) || (m_columns <= 0))
	{
		return;
	}
	const int tw = static_cast<int>(m_tileset->GetTileWidth());
	const int th = static_cast<int>(m_tileset->GetTileHeight());
	const int bw = static_cast<int>(Landstalker::MapBlock::GetBlockWidth());
	const int bh = static_cast<int>(Landstalker::MapBlock::GetBlockHeight());
	ImageBufferWx buf(m_columns * bw * tw, m_rows * bh * th);
	for (std::size_t i = 0; i < m_blocks->size(); ++i)
	{
		const int bx = (i % m_columns) * bw;
		const int by = (i / m_columns) * bh;
		for (int t = 0; t < static_cast<int>(Landstalker::MapBlock::GetBlockSize()); ++t)
		{
			buf.InsertTile((bx + t % bw) * tw, (by + t / bw) * th, 0,
				m_blocks->at(i).GetTile(t % bw, t / bw), *m_tileset, false);
		}
	}
	m_tiles_bmp = std::make_unique<wxBitmap>(buf.MakeImage({ m_pal }, true));
	m_tiles_bmp_dirty = false;
}

bool BlocksetEditorCtrl::DrawBlockPriority(wxDC& dc, int x, int y, const Landstalker::MapBlock& block)
{
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.SetPen(*m_priority_pen);

	int pri_tile_count = 0;
	std::array<bool, Landstalker::MapBlock::GetBlockSize()> tile_priorities = {false, false, false, false};
	for (int i = 0; i < static_cast<int>(block.GetBlockSize()); ++i)
	{
		if (block.GetTile(i).Attributes().getAttribute(Landstalker::TileAttributes::Attribute::ATTR_PRIORITY))
		{
			++pri_tile_count;
			tile_priorities[i] = true;
		}
	}
	if (pri_tile_count == 4)
	{
		dc.DrawRectangle({
			x * m_cellwidth + 1,
			y * m_cellheight + 1,
			m_cellwidth - 2,
			m_cellheight - 2
		});
	}
	else if (pri_tile_count == 3)
	{
		int empty_cell = std::distance(tile_priorities.cbegin(),
			std::find_if(tile_priorities.cbegin(), tile_priorities.cend(), [](const bool cell)
				{
					return !cell;
				}
		));
		std::array<wxPoint, 7> shape_L = { { {0, 0}, {2, 0}, {2, 1}, {1, 1}, {1, 2}, {0, 2}, {0, 0} } };
		std::transform(shape_L.cbegin(), shape_L.cend(), shape_L.begin(), [empty_cell](const wxPoint& point)
			{
				return wxPoint{
					(empty_cell % 2 == 0) ? 2 - point.x : point.x,
					(empty_cell < 2) ? 2 - point.y : point.y
				};
			});
		std::transform(shape_L.cbegin(), shape_L.cend(), shape_L.begin(), [this](const wxPoint& point)
			{
				return wxPoint{
					point.x == 0 ? 0 : point.x * m_tilewidth - 2,
					point.y == 0 ? 0 : point.y * m_tileheight - 2
				};
			});
		dc.DrawPolygon(shape_L.size(), shape_L.data(), x * m_cellwidth + 1, y * m_cellheight + 1);
	}
	else if ((pri_tile_count == 2) && (tile_priorities[0] != tile_priorities[3]))
	{
		if (tile_priorities[0] == tile_priorities[1])
		{
			dc.DrawRectangle({
				x * m_cellwidth + 1,
				y * m_cellheight + 1 + (tile_priorities[2] ? m_tileheight : 0),
				m_cellwidth - 2,
				m_tileheight - 2
			});
		}
		else
		{
			dc.DrawRectangle({
				x * m_cellwidth + 1 + (tile_priorities[1] ? m_tilewidth : 0),
				y * m_cellheight + 1,
				m_tilewidth - 2,
				m_cellheight - 2
			});
		}
	}
	else if (pri_tile_count > 0)
	{
		for (int i = 0; i < static_cast<int>(block.GetBlockSize()); ++i)
		{
			if (tile_priorities.at(i))
			{
				dc.DrawRectangle({
					x * m_cellwidth + 1 + ((i % static_cast<int>(Landstalker::MapBlock::GetBlockWidth())) * m_tilewidth),
					y * m_cellheight + 1 + ((i / static_cast<int>(Landstalker::MapBlock::GetBlockWidth())) * m_tileheight),
					m_tilewidth - 2,
					m_tileheight - 2
				});
			}
		}
	}
	return true;
}

void BlocksetEditorCtrl::DrawSelectionBorders(wxDC& dc)
{
	if (m_columns == 0)
	{
		return;
	}
	int hbx = (m_hoveredblock % m_columns) * m_cellwidth;
	int hby = (m_hoveredblock / m_columns) * m_cellheight;
	int sbx = (m_selectedblock % m_columns) * m_cellwidth;
	int sby = (m_selectedblock / m_columns) * m_cellheight;
	int htx = (m_hoveredtile % Landstalker::MapBlock::GetBlockWidth()) * m_tilewidth;
	int hty = (m_hoveredtile / Landstalker::MapBlock::GetBlockWidth()) * m_tileheight;
	int stx = (m_selectedtile % Landstalker::MapBlock::GetBlockWidth()) * m_tilewidth;
	int sty = (m_selectedtile / Landstalker::MapBlock::GetBlockWidth()) * m_tileheight;
	switch (m_mode)
	{
	case Mode::BLOCK_SELECT:
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		if (m_hoveredblock != -1 && m_hoveredblock != m_selectedblock)
		{
			dc.SetPen(*wxWHITE_PEN);
			dc.DrawRectangle({ hbx, hby, m_cellwidth, m_cellheight });
		}
		if (m_selectedblock != -1 && m_hoveredblock != m_selectedblock)
		{
			dc.SetPen(*wxYELLOW_PEN);
			dc.DrawRectangle({ sbx, sby, m_cellwidth, m_cellheight });
		}
		if (m_selectedblock != -1 && m_hoveredblock == m_selectedblock)
		{
			dc.SetPen(wxPen(wxColor(255, 255, 128)));
			dc.DrawRectangle({ sbx, sby, m_cellwidth, m_cellheight });
		}
		break;
	case Mode::TILE_SELECT:
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		if (m_hoveredblock != -1 && m_hoveredtile != -1 && (m_hoveredblock != m_selectedblock || m_hoveredtile != m_selectedtile))
		{
			dc.SetPen(*wxWHITE_PEN);
			dc.DrawRectangle({ hbx + htx, hby + hty, m_tilewidth, m_tileheight });
		}
		if (m_selectedblock != -1 && m_selectedtile != -1 && (m_hoveredblock != m_selectedblock || m_hoveredtile != m_selectedtile)) 
		{
			dc.SetPen(*wxYELLOW_PEN);
			dc.DrawRectangle({ sbx + stx, sby + sty, m_tilewidth, m_tileheight });
		}
		if (m_selectedblock != -1 && m_selectedtile != -1 && (m_hoveredblock == m_selectedblock && m_hoveredtile == m_selectedtile))
		{
			dc.SetPen(wxPen(wxColor(255, 255, 128)));
			dc.DrawRectangle({ sbx + stx, sby + sty, m_tilewidth, m_tileheight });
		}
		break;
	case Mode::PENCIL:
	case Mode::LINE:
	case Mode::RECTANGLE_OUTLINE:
	case Mode::RECTANGLE_FILLED:
	case Mode::CIRCLE_OUTLINE:
	case Mode::CIRCLE_FILLED:
	case Mode::FILL:
		if (m_hoveredblock != -1 && m_hoveredtile != -1 && !m_shape_active)
		{
			// Preview of the draw tile under the cursor, as in the 2D map editor.
			// Backdrop first, so its transparent pixels show the checkerboard rather
			// than the slot content underneath.
			const int ntw = static_cast<int>(m_tileset->GetTileWidth());
			const int nth = static_cast<int>(m_tileset->GetTileHeight());
			wxBitmap bmp = MakeTileBitmap(m_tileset->GetTileBGRA(m_drawtile, GetSelectedPalette()), ntw, nth);
			wxMemoryDC tdc(bmp);
			dc.SetPen(*wxTRANSPARENT_PEN);
			dc.SetBrush(m_enablealpha ? *m_alpha_brush : *wxBLACK_BRUSH);
			dc.DrawRectangle(hbx + htx, hby + hty, m_tilewidth, m_tileheight);
			dc.StretchBlit({ hbx + htx, hby + hty }, { m_tilewidth, m_tileheight },
				&tdc, { 0, 0 }, { ntw, nth }, wxCOPY, true, { 0, 0 });
			tdc.SelectObject(wxNullBitmap);
			dc.SetBrush(*wxTRANSPARENT_BRUSH);
			dc.SetPen(*wxWHITE_PEN);
			dc.DrawRectangle({ hbx + htx, hby + hty, m_tilewidth, m_tileheight });
		}
		break;
	default:
		break;
	}
}

void BlocksetEditorCtrl::InitialiseBrushesAndPens()
{
	m_alpha_brush = std::make_unique<wxBrush>();
	m_stipple = std::make_unique<wxBitmap>(6, 6);
	std::unique_ptr<wxMemoryDC> imagememDC(new wxMemoryDC());
	imagememDC->SelectObject(*m_stipple);
	imagememDC->SetBackground(*wxGREY_BRUSH);
	imagememDC->Clear();
	imagememDC->SetBrush(*wxLIGHT_GREY_BRUSH);
	imagememDC->SetPen(*wxTRANSPARENT_PEN);
	imagememDC->DrawRectangle(0, 0, 3, 3);
	imagememDC->DrawRectangle(3, 3, 5, 5);
	imagememDC->SelectObject(wxNullBitmap);
	m_alpha_brush->SetStyle(wxBRUSHSTYLE_STIPPLE_MASK);
	m_alpha_brush->SetStipple(*m_stipple);
	m_border_pen = std::make_unique<wxPen>(*wxMEDIUM_GREY_PEN);
	m_tile_border_pen = std::make_unique<wxPen>(wxColour(65, 65, 65));
	m_selected_border_pen = std::make_unique<wxPen>(*wxYELLOW_PEN);
	m_highlighted_border_pen = std::make_unique<wxPen>(*wxBLUE_PEN);
	m_highlighted_brush = std::make_unique<wxBrush>(*wxTRANSPARENT_BRUSH);
	m_priority_pen = std::make_unique<wxPen>(*wxCYAN, 1, wxPENSTYLE_SHORT_DASH);
}

void BlocksetEditorCtrl::ForceRedraw()
{
	m_tiles_bmp_dirty = true;
	wxVarVScrollHelper::RefreshAll();
	Refresh();
}

void BlocksetEditorCtrl::RefreshBlock(int block)
{
	// Repaints a single cell. Hover and selection changes happen on every mouse move, and a
	// full-window Refresh for each is what made the cursor lag.
	if ((block < 0) || (m_columns <= 0))
	{
		return;
	}
	const int s = GetVisibleRowsBegin();
	wxRect rect((block % m_columns) * m_cellwidth, (block / m_columns - s) * m_cellheight,
	            m_cellwidth + 1, m_cellheight + 1);
	rect.Inflate(1, 1);
	RefreshRect(rect);
}

Landstalker::Palette& BlocksetEditorCtrl::GetSelectedPalette()
{
	return *m_pal;
}

int BlocksetEditorCtrl::ConvertXYToBlockIdx(const wxPoint& point) const
{
	if (m_tileset == nullptr)
	{
		return -1;
	}
	int s = GetVisibleRowsBegin();
	int x = point.x / (m_pixelsize * m_tileset->GetTileWidth() * Landstalker::MapBlock::GetBlockWidth());
	int y = s + point.y / (m_pixelsize * m_tileset->GetTileHeight() * Landstalker::MapBlock::GetBlockHeight());
	int sel = x + y * m_columns;
	if ((sel >= static_cast<int>(m_blocks->size())) || (x < 0) || (y < 0) || (x >= m_columns))
	{
		sel = -1;
	}
	return sel;
}

int BlocksetEditorCtrl::ConvertXYToTileIdx(const wxPoint& point) const
{
	if (m_tileset == nullptr)
	{
		return -1;
	}
	int xx = point.x % (m_pixelsize * m_tileset->GetTileWidth() * Landstalker::MapBlock::GetBlockWidth());
	int yy = point.y % (m_pixelsize * m_tileset->GetTileHeight() * Landstalker::MapBlock::GetBlockHeight());
	int x = xx / (m_pixelsize * m_tileset->GetTileWidth());
	int y = yy / (m_pixelsize * m_tileset->GetTileHeight());
	int sel = x + y * Landstalker::MapBlock::GetBlockWidth();
	if ((sel >= static_cast<int>(Landstalker::MapBlock::GetBlockSize()) || (x < 0) || (y < 0) ||
		(x >= static_cast<int>(Landstalker::MapBlock::GetBlockWidth()))) || (y >= static_cast<int>(Landstalker::MapBlock::GetBlockHeight())))
	{
		sel = -1;
	}
	return sel;
}

bool BlocksetEditorCtrl::HandleKeyDown(int key, int modifiers)
{
	// Esc abandons an in-progress shape drag or pencil stroke.
	if (m_shape_active && (key == WXK_ESCAPE))
	{
		CancelShapeDrag();
		return true;
	}
	if (m_stroke_active && (key == WXK_ESCAPE))
	{
		CancelStroke();
		return true;
	}
	const bool ctrl = (modifiers == wxMOD_CONTROL);
	// The H/V/P attribute keys work in more than box-select mode; unhandled combinations
	// fall through to the frame's per-tile handling. E is a full alias for vflip: Ctrl+V
	// stays paste, so the tristate variant is only reachable as Ctrl+E.
	switch (key)
	{
	case 'h':
	case 'H':
		if (HandleAttributeKey(Landstalker::TileAttributes::Attribute::ATTR_HFLIP, modifiers))
		{
			return true;
		}
		break;
	case 'v':
	case 'V':
		if (!ctrl && HandleAttributeKey(Landstalker::TileAttributes::Attribute::ATTR_VFLIP, modifiers))
		{
			return true;
		}
		break;
	case 'e':
	case 'E':
		if (HandleAttributeKey(Landstalker::TileAttributes::Attribute::ATTR_VFLIP, modifiers))
		{
			return true;
		}
		break;
	case 'p':
	case 'P':
		if (HandleAttributeKey(Landstalker::TileAttributes::Attribute::ATTR_PRIORITY, modifiers))
		{
			return true;
		}
		break;
	default:
		break;
	}
	if (m_mode != Mode::BOX_SELECT)
	{
		return false;
	}
	switch (key)
	{
	case WXK_ESCAPE:
		return CancelActiveBoxOp();
	case WXK_DELETE:
		if ((modifiers == 0) && HasBoxSelection() && !m_sel_floating)
		{
			ClearBoxCells();
			return true;
		}
		return false;
	case 'c':
	case 'C':
		if (ctrl)
		{
			CopyBoxSelection();
			return true;
		}
		return false;
	case 'x':
	case 'X':
		if (ctrl)
		{
			CutBoxSelection();
			return true;
		}
		return false;
	case 'v':
	case 'V':
		if (ctrl)
		{
			PasteBoxCells();
			return true;
		}
		return false;
	case 'a':
	case 'A':
		if (ctrl)
		{
			SelectAllBoxCells();
			return true;
		}
		return false;
	// With a selection, +/- shifts every selected tile ID (+/-10 with Shift, +/-100 with
	// Ctrl); without one the keys fall through to the frame's draw-tile cycling.
	case '+':
	case '=':
	case WXK_NUMPAD_ADD:
		if (HasBoxSelection())
		{
			AdjustBoxTileIds((modifiers & wxMOD_CONTROL) ? 100 : ((modifiers & wxMOD_SHIFT) ? 10 : 1));
			return true;
		}
		return false;
	case '-':
	case '_':
	case WXK_NUMPAD_SUBTRACT:
		if (HasBoxSelection())
		{
			AdjustBoxTileIds((modifiers & wxMOD_CONTROL) ? -100 : ((modifiers & wxMOD_SHIFT) ? -10 : -1));
			return true;
		}
		return false;
	default:
		return false;
	}
}

bool BlocksetEditorCtrl::HandleAttributeKey(Landstalker::TileAttributes::Attribute attr, int modifiers)
{
	const bool plain = (modifiers == 0);
	const bool ctrl = (modifiers == wxMOD_CONTROL);
	const bool alt = (modifiers == wxMOD_ALT);
	if ((m_mode == Mode::BOX_SELECT) && HasBoxSelection())
	{
		// Plain = full flip (tristate + mirrored layout), Ctrl = tristate set/clear in
		// place, Alt = invert every slot's own bit.
		if (plain)
		{
			ToggleBoxAttribute(attr, AttrToggleMode::FlipMirror);
			return true;
		}
		if (ctrl)
		{
			ToggleBoxAttribute(attr, AttrToggleMode::TriState);
			return true;
		}
		if (alt)
		{
			ToggleBoxAttribute(attr, AttrToggleMode::Toggle);
			return true;
		}
		return false;
	}
	if (IsDrawMode(m_mode) && plain)
	{
		// In the draw modes the keys flip the draw tile itself.
		Landstalker::Tile t = m_drawtile;
		t.Attributes().toggleAttribute(attr);
		SetDrawTile(t);
		return true;
	}
	return false;
}

void BlocksetEditorCtrl::AdjustBoxTileIds(int delta)
{
	if (!HasBoxSelection())
	{
		return;
	}
	const auto adjust = [&](Landstalker::Tile t)
	{
		t.SetIndex(static_cast<uint16_t>(((t.GetIndex() + delta) % 1024 + 1024) % 1024));
		return t;
	};
	if (m_sel_floating)
	{
		for (auto& t : m_float_tiles)
		{
			if (t.GetTileValue() != SEL_INVALID_TILE)
			{
				t = adjust(t);
			}
		}
		RenderBoxFloatBitmap();
		RefreshBoxRect(m_sel_rect);
		return;
	}
	auto snapshot = Landstalker::Blockset(*m_blocks);
	bool changed = false;
	for (int y = 0; y < m_sel_rect.height; ++y)
	{
		for (int x = 0; x < m_sel_rect.width; ++x)
		{
			const auto t = GetSlotTile(m_sel_rect.x + x, m_sel_rect.y + y);
			if (t.GetTileValue() == SEL_INVALID_TILE)
			{
				continue;
			}
			changed |= SetSlotTile(m_sel_rect.x + x, m_sel_rect.y + y, adjust(t));
		}
	}
	if (changed)
	{
		PushUndoState(std::move(snapshot));
		m_tiles_bmp_dirty = true;
		RefreshBoxRect(m_sel_rect);
		FireBlockEvent(EVT_BLOCK_SELECT, "");
	}
}

bool BlocksetEditorCtrl::HasBoxSelection() const
{
	return (m_sel_rect.width > 0) && (m_sel_rect.height > 0);
}

wxPoint BlocksetEditorCtrl::RawSlotFromPoint(const wxPoint& point) const
{
	const int bh = static_cast<int>(Landstalker::MapBlock::GetBlockHeight());
	return { point.x / m_tilewidth,
	         static_cast<int>(GetVisibleRowsBegin()) * bh + point.y / m_tileheight };
}

bool BlocksetEditorCtrl::IsSlotValid(int sx, int sy) const
{
	const int bw = static_cast<int>(Landstalker::MapBlock::GetBlockWidth());
	const int bh = static_cast<int>(Landstalker::MapBlock::GetBlockHeight());
	if ((sx < 0) || (sy < 0) || (sx >= m_columns * bw))
	{
		return false;
	}
	const int block = (sx / bw) + (sy / bh) * m_columns;
	return IsBlockIndexValid(block);
}

Landstalker::Tile BlocksetEditorCtrl::GetSlotTile(int sx, int sy) const
{
	if (!IsSlotValid(sx, sy))
	{
		return Landstalker::Tile(SEL_INVALID_TILE);
	}
	const int bw = static_cast<int>(Landstalker::MapBlock::GetBlockWidth());
	const int bh = static_cast<int>(Landstalker::MapBlock::GetBlockHeight());
	const int block = (sx / bw) + (sy / bh) * m_columns;
	return m_blocks->at(block).GetTile((sx % bw) + (sy % bh) * bw);
}

bool BlocksetEditorCtrl::SetSlotTile(int sx, int sy, const Landstalker::Tile& tile)
{
	if (!IsSlotValid(sx, sy) || (tile.GetTileValue() == SEL_INVALID_TILE))
	{
		return false;
	}
	const int bw = static_cast<int>(Landstalker::MapBlock::GetBlockWidth());
	const int bh = static_cast<int>(Landstalker::MapBlock::GetBlockHeight());
	const int block = (sx / bw) + (sy / bh) * m_columns;
	const int sub = (sx % bw) + (sy % bh) * bw;
	if (m_blocks->at(block).GetTile(sub) == tile)
	{
		return false;
	}
	m_blocks->at(block).SetTile(sub, tile);
	return true;
}

void BlocksetEditorCtrl::BeginBoxAction(int sx, int sy)
{
	if (m_blocks == nullptr)
	{
		return;
	}
	const int bw = static_cast<int>(Landstalker::MapBlock::GetBlockWidth());
	const int bh = static_cast<int>(Landstalker::MapBlock::GetBlockHeight());
	const int ws = m_columns * bw;
	const int hs = m_rows * bh;
	if (HasBoxSelection() && m_sel_rect.Contains(wxPoint(sx, sy)))
	{
		m_sel_anchor = wxPoint(sx - m_sel_rect.x, sy - m_sel_rect.y);
		m_sel_op_changed = false;
		if (m_sel_floating)
		{
			// Dragging a pending paste just moves the float; it stays unconfirmed.
			m_sel_drag = SelDrag::Move;
		}
		else
		{
			m_sel_snapshot = std::make_unique<Landstalker::Blockset>(*m_blocks);
			if (wxGetKeyState(WXK_CONTROL))
			{
				m_sel_drag = SelDrag::Stamp;
			}
			else if (wxGetKeyState(WXK_SHIFT))
			{
				m_sel_drag = SelDrag::Duplicate;
			}
			else
			{
				m_sel_drag = SelDrag::Move;
			}
			LiftBoxSelection(m_sel_drag == SelDrag::Move);
		}
		CaptureMouse();
	}
	else
	{
		// Clicking outside confirms a pending paste, clears the selection and starts a
		// fresh marquee from here.
		ClearBoxSelection(true);
		if ((sx >= 0) && (sy >= 0) && (sx < ws) && (sy < hs))
		{
			m_sel_drag = SelDrag::Marquee;
			m_sel_anchor = wxPoint(sx, sy);
			m_sel_rect = wxRect(sx, sy, 1, 1);
			RefreshBoxRect(m_sel_rect);
			CaptureMouse();
		}
	}
}

void BlocksetEditorCtrl::UpdateBoxDrag(int sx, int sy)
{
	const int bw = static_cast<int>(Landstalker::MapBlock::GetBlockWidth());
	const int bh = static_cast<int>(Landstalker::MapBlock::GetBlockHeight());
	const int ws = m_columns * bw;
	const int hs = m_rows * bh;
	switch (m_sel_drag)
	{
	case SelDrag::Marquee:
	{
		const int px = std::clamp(sx, 0, ws - 1);
		const int py = std::clamp(sy, 0, hs - 1);
		const wxRect next(wxPoint(std::min(m_sel_anchor.x, px), std::min(m_sel_anchor.y, py)),
		                  wxSize(std::abs(px - m_sel_anchor.x) + 1, std::abs(py - m_sel_anchor.y) + 1));
		if (next != m_sel_rect)
		{
			RefreshBoxRect(m_sel_rect);
			m_sel_rect = next;
			RefreshBoxRect(m_sel_rect);
		}
		break;
	}
	case SelDrag::Move:
	case SelDrag::Duplicate:
	case SelDrag::Stamp:
	{
		wxPoint tl(sx - m_sel_anchor.x, sy - m_sel_anchor.y);
		tl.x = std::clamp(tl.x, 0, ws - m_sel_rect.width);
		tl.y = std::clamp(tl.y, 0, hs - m_sel_rect.height);
		if (tl != m_sel_rect.GetTopLeft())
		{
			RefreshBoxRect(m_sel_rect);
			m_sel_rect.x = tl.x;
			m_sel_rect.y = tl.y;
			if (m_sel_drag == SelDrag::Stamp)
			{
				// Continuous duplication: every step leaves a copy on the blockset.
				m_sel_op_changed |= StampBoxFloating();
			}
			RefreshBoxRect(m_sel_rect);
		}
		break;
	}
	default:
		break;
	}
}

void BlocksetEditorCtrl::FinishBoxDrag()
{
	if (HasCapture())
	{
		ReleaseMouse();
	}
	if ((m_sel_drag == SelDrag::Move) || (m_sel_drag == SelDrag::Duplicate) ||
	    (m_sel_drag == SelDrag::Stamp))
	{
		if (!m_sel_from_paste)
		{
			if (m_sel_drag != SelDrag::Stamp)
			{
				m_sel_op_changed |= StampBoxFloating();
			}
			m_sel_floating = false;
			m_float_bmp.reset();
			// A drag that ends where it started leaves the blockset untouched (erase and
			// re-stamp cancel out); comparing avoids a junk undo entry for that case.
			bool push = false;
			if (m_sel_snapshot && m_sel_op_changed)
			{
				push = !(*m_blocks == *m_sel_snapshot);
			}
			if (push)
			{
				PushUndoState(std::move(*m_sel_snapshot));
				FireBlockEvent(EVT_BLOCK_SELECT, "");
			}
			RefreshBoxRect(m_sel_rect);
		}
	}
	m_sel_drag = SelDrag::None;
	m_sel_snapshot.reset();
	m_sel_op_changed = false;
}

void BlocksetEditorCtrl::CancelBoxDrag()
{
	if (HasCapture())
	{
		ReleaseMouse();
	}
	switch (m_sel_drag)
	{
	case SelDrag::Marquee:
	{
		const wxRect old = m_sel_rect;
		m_sel_rect = wxRect();
		RefreshBoxRect(old);
		break;
	}
	case SelDrag::Move:
	case SelDrag::Duplicate:
	case SelDrag::Stamp:
		if (m_sel_from_paste)
		{
			// Cancelling mid-drag drops the pending paste entirely.
			DiscardBoxFloating();
			const wxRect old = m_sel_rect;
			m_sel_rect = wxRect();
			RefreshBoxRect(old);
		}
		else if (m_sel_snapshot)
		{
			// Puts the blockset back exactly as it was before the lift.
			m_sel_floating = false;
			m_float_bmp.reset();
			RestoreHistoryState(std::move(*m_sel_snapshot));
		}
		break;
	default:
		break;
	}
	m_sel_drag = SelDrag::None;
	m_sel_snapshot.reset();
	m_sel_op_changed = false;
}

bool BlocksetEditorCtrl::CancelActiveBoxOp()
{
	if (m_sel_drag != SelDrag::None)
	{
		CancelBoxDrag();
		return true;
	}
	if (m_sel_floating)
	{
		// Esc cancels a pending paste outright rather than confirming it.
		DiscardBoxFloating();
		const wxRect old = m_sel_rect;
		m_sel_rect = wxRect();
		RefreshBoxRect(old);
		return true;
	}
	if (HasBoxSelection())
	{
		const wxRect old = m_sel_rect;
		m_sel_rect = wxRect();
		RefreshBoxRect(old);
		return true;
	}
	return false;
}

void BlocksetEditorCtrl::ClearBoxSelection(bool confirm_floating)
{
	if (m_sel_floating)
	{
		if (confirm_floating)
		{
			ConfirmBoxFloating();
		}
		else
		{
			DiscardBoxFloating();
		}
	}
	if (HasBoxSelection())
	{
		const wxRect old = m_sel_rect;
		m_sel_rect = wxRect();
		RefreshBoxRect(old);
	}
}

void BlocksetEditorCtrl::ConfirmBoxFloating()
{
	if (!m_sel_floating)
	{
		return;
	}
	auto snapshot = Landstalker::Blockset(*m_blocks);
	const bool changed = StampBoxFloating();
	m_sel_floating = false;
	m_sel_from_paste = false;
	m_float_bmp.reset();
	if (changed)
	{
		PushUndoState(std::move(snapshot));
		FireBlockEvent(EVT_BLOCK_SELECT, "");
	}
	RefreshBoxRect(m_sel_rect);
}

void BlocksetEditorCtrl::DiscardBoxFloating()
{
	if (!m_sel_floating)
	{
		return;
	}
	m_sel_floating = false;
	m_sel_from_paste = false;
	m_float_bmp.reset();
	m_float_tiles.clear();
	RefreshBoxRect(m_sel_rect);
}

void BlocksetEditorCtrl::LiftBoxSelection(bool erase_source)
{
	m_float_tiles = ReadBoxRect(m_sel_rect);
	if (erase_source)
	{
		bool changed = false;
		for (int y = 0; y < m_sel_rect.height; ++y)
		{
			for (int x = 0; x < m_sel_rect.width; ++x)
			{
				changed |= SetSlotTile(m_sel_rect.x + x, m_sel_rect.y + y, Landstalker::Tile());
			}
		}
		if (changed)
		{
			m_tiles_bmp_dirty = true;
			RefreshBoxRect(m_sel_rect);
			m_sel_op_changed = true;
		}
	}
	m_sel_floating = true;
	m_sel_from_paste = false;
	RenderBoxFloatBitmap();
	RefreshBoxRect(m_sel_rect);
}

bool BlocksetEditorCtrl::StampBoxFloating()
{
	if (m_float_tiles.size() !=
	    static_cast<std::size_t>(m_sel_rect.width) * static_cast<std::size_t>(m_sel_rect.height))
	{
		return false;
	}
	bool changed = false;
	for (int y = 0; y < m_sel_rect.height; ++y)
	{
		for (int x = 0; x < m_sel_rect.width; ++x)
		{
			changed |= SetSlotTile(m_sel_rect.x + x, m_sel_rect.y + y,
			                       m_float_tiles[x + y * m_sel_rect.width]);
		}
	}
	if (changed)
	{
		m_tiles_bmp_dirty = true;
		RefreshBoxRect(m_sel_rect);
	}
	return changed;
}

void BlocksetEditorCtrl::RenderBoxFloatBitmap()
{
	const int ntw = static_cast<int>(m_tileset->GetTileWidth());
	const int nth = static_cast<int>(m_tileset->GetTileHeight());
	ImageBufferWx buf(m_sel_rect.width * ntw, m_sel_rect.height * nth);
	for (int y = 0; y < m_sel_rect.height; ++y)
	{
		for (int x = 0; x < m_sel_rect.width; ++x)
		{
			const auto& t = m_float_tiles[x + y * m_sel_rect.width];
			if (t.GetTileValue() == SEL_INVALID_TILE)
			{
				continue;
			}
			buf.InsertTile(x * ntw, y * nth, 0, t, *m_tileset, false);
		}
	}
	m_float_bmp = std::make_unique<wxBitmap>(buf.MakeImage({ m_pal }, true));
}

void BlocksetEditorCtrl::ClearBoxCells()
{
	if (!HasBoxSelection() || m_sel_floating)
	{
		return;
	}
	auto snapshot = Landstalker::Blockset(*m_blocks);
	bool changed = false;
	for (int y = 0; y < m_sel_rect.height; ++y)
	{
		for (int x = 0; x < m_sel_rect.width; ++x)
		{
			changed |= SetSlotTile(m_sel_rect.x + x, m_sel_rect.y + y, Landstalker::Tile());
		}
	}
	if (changed)
	{
		PushUndoState(std::move(snapshot));
		m_tiles_bmp_dirty = true;
		RefreshBoxRect(m_sel_rect);
		FireBlockEvent(EVT_BLOCK_SELECT, "");
	}
}

void BlocksetEditorCtrl::ToggleBoxAttribute(Landstalker::TileAttributes::Attribute attr,
	AttrToggleMode toggle_mode)
{
	if (!HasBoxSelection())
	{
		return;
	}
	const int w = m_sel_rect.width;
	const int h = m_sel_rect.height;
	// In FlipMirror mode H/V flips mirror the block's slot layout too, so the result reads
	// as a true mirror image of the selection; priority has no spatial meaning and leaves
	// positions alone, as do the other toggle modes.
	const bool mirror_x = (toggle_mode == AttrToggleMode::FlipMirror) &&
		(attr == Landstalker::TileAttributes::Attribute::ATTR_HFLIP);
	const bool mirror_y = (toggle_mode == AttrToggleMode::FlipMirror) &&
		(attr == Landstalker::TileAttributes::Attribute::ATTR_VFLIP);
	const auto transform = [&](const std::vector<Landstalker::Tile>& src)
	{
		// Tristate: if any slot lacks the bit, set it everywhere; only clear once all
		// have it. Toggle mode instead inverts each slot's own bit. Slots over missing
		// blocks don't take part.
		bool all_set = true;
		for (const auto& t : src)
		{
			if (t.GetTileValue() != SEL_INVALID_TILE)
			{
				all_set = all_set && t.Attributes().getAttribute(attr);
			}
		}
		std::vector<Landstalker::Tile> out(src.size());
		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				const int sx = mirror_x ? (w - 1 - x) : x;
				const int sy = mirror_y ? (h - 1 - y) : y;
				Landstalker::Tile t = src[sx + sy * w];
				if (t.GetTileValue() != SEL_INVALID_TILE)
				{
					if (toggle_mode == AttrToggleMode::Toggle)
					{
						t.Attributes().toggleAttribute(attr);
					}
					else if (all_set)
					{
						t.Attributes().clearAttribute(attr);
					}
					else
					{
						t.Attributes().setAttribute(attr);
					}
				}
				out[x + y * w] = t;
			}
		}
		return out;
	};
	if (m_sel_floating)
	{
		m_float_tiles = transform(m_float_tiles);
		RenderBoxFloatBitmap();
		RefreshBoxRect(m_sel_rect);
		return;
	}
	auto snapshot = Landstalker::Blockset(*m_blocks);
	const auto result = transform(ReadBoxRect(m_sel_rect));
	bool changed = false;
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			changed |= SetSlotTile(m_sel_rect.x + x, m_sel_rect.y + y, result[x + y * w]);
		}
	}
	if (changed)
	{
		PushUndoState(std::move(snapshot));
		m_tiles_bmp_dirty = true;
		RefreshBoxRect(m_sel_rect);
		FireBlockEvent(EVT_BLOCK_SELECT, "");
	}
}

void BlocksetEditorCtrl::CopyBoxSelection()
{
	if (!HasBoxSelection())
	{
		return;
	}
	m_slot_clipboard.rect = m_sel_rect;
	m_slot_clipboard.tiles = m_sel_floating ? m_float_tiles : ReadBoxRect(m_sel_rect);
}

void BlocksetEditorCtrl::CutBoxSelection()
{
	if (!HasBoxSelection())
	{
		return;
	}
	CopyBoxSelection();
	if (m_sel_floating)
	{
		// Cutting a pending paste just removes the float; the blockset never had it.
		DiscardBoxFloating();
	}
	else
	{
		ClearBoxCells();
	}
}

void BlocksetEditorCtrl::PasteBoxCells()
{
	if (m_slot_clipboard.tiles.empty())
	{
		return;
	}
	ClearBoxSelection(true);
	const int ws = m_columns * static_cast<int>(Landstalker::MapBlock::GetBlockWidth());
	const int hs = m_rows * static_cast<int>(Landstalker::MapBlock::GetBlockHeight());
	wxRect r = m_slot_clipboard.rect;
	if ((r.width > ws) || (r.height > hs))
	{
		return;
	}
	r.x = std::clamp(r.x, 0, ws - r.width);
	r.y = std::clamp(r.y, 0, hs - r.height);
	m_sel_rect = r;
	m_float_tiles = m_slot_clipboard.tiles;
	m_sel_floating = true;
	m_sel_from_paste = true;
	RenderBoxFloatBitmap();
	RefreshBoxRect(m_sel_rect);
}

void BlocksetEditorCtrl::SelectAllBoxCells()
{
	if ((m_blocks == nullptr) || m_blocks->empty())
	{
		return;
	}
	ClearBoxSelection(true);
	m_sel_rect = wxRect(0, 0, m_columns * static_cast<int>(Landstalker::MapBlock::GetBlockWidth()),
	                    m_rows * static_cast<int>(Landstalker::MapBlock::GetBlockHeight()));
	RefreshBoxRect(m_sel_rect);
}

std::vector<Landstalker::Tile> BlocksetEditorCtrl::ReadBoxRect(const wxRect& rect) const
{
	std::vector<Landstalker::Tile> out;
	out.reserve(static_cast<std::size_t>(rect.width) * static_cast<std::size_t>(rect.height));
	for (int y = 0; y < rect.height; ++y)
	{
		for (int x = 0; x < rect.width; ++x)
		{
			out.push_back(GetSlotTile(rect.x + x, rect.y + y));
		}
	}
	return out;
}

void BlocksetEditorCtrl::RefreshBoxRect(const wxRect& rect)
{
	if ((rect.width <= 0) || (rect.height <= 0))
	{
		return;
	}
	const int bh = static_cast<int>(Landstalker::MapBlock::GetBlockHeight());
	wxRect r(rect.x * m_tilewidth,
	         (rect.y - static_cast<int>(GetVisibleRowsBegin()) * bh) * m_tileheight,
	         rect.width * m_tilewidth + 1, rect.height * m_tileheight + 1);
	r.Inflate(2, 2);
	RefreshRect(r);
}

void BlocksetEditorCtrl::DrawBoxSelection(wxDC& dc)
{
	if (!HasBoxSelection())
	{
		return;
	}
	if (m_sel_floating && (m_float_bmp != nullptr))
	{
		const int ntw = static_cast<int>(m_tileset->GetTileWidth());
		const int nth = static_cast<int>(m_tileset->GetTileHeight());
		wxMemoryDC mem(*m_float_bmp);
		dc.StretchBlit(m_sel_rect.x * m_tilewidth, m_sel_rect.y * m_tileheight,
		               m_sel_rect.width * m_tilewidth, m_sel_rect.height * m_tileheight,
		               &mem, 0, 0, m_sel_rect.width * ntw, m_sel_rect.height * nth, wxCOPY, true);
		mem.SelectObject(wxNullBitmap);
	}
	// White underlay + black dashes stays visible over any artwork.
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.SetPen(*wxWHITE_PEN);
	dc.DrawRectangle(m_sel_rect.x * m_tilewidth, m_sel_rect.y * m_tileheight,
	                 m_sel_rect.width * m_tilewidth + 1, m_sel_rect.height * m_tileheight + 1);
	dc.SetPen(wxPen(*wxBLACK, 1, wxPENSTYLE_SHORT_DASH));
	dc.DrawRectangle(m_sel_rect.x * m_tilewidth, m_sel_rect.y * m_tileheight,
	                 m_sel_rect.width * m_tilewidth + 1, m_sel_rect.height * m_tileheight + 1);
}

void BlocksetEditorCtrl::ResetBoxSelectionState()
{
	m_sel_rect = wxRect();
	m_sel_drag = SelDrag::None;
	m_sel_floating = false;
	m_sel_from_paste = false;
	m_sel_op_changed = false;
	m_sel_snapshot.reset();
	m_float_bmp.reset();
	m_float_tiles.clear();
	// The slot clipboard survives on purpose, so content can be pasted across blocksets.
	m_shape_active = false;
	m_stroke_active = false;
	m_stroke_snapshot.reset();
	m_stroke_changed = false;
}

void BlocksetEditorCtrl::BeginShapeDrag(int sx, int sy)
{
	// Shapes must start on a real slot; the drag itself may then wander and clamp.
	if (!IsSlotValid(sx, sy))
	{
		return;
	}
	m_shape_active = true;
	m_shape_anchor = wxPoint(sx, sy);
	m_shape_current = m_shape_anchor;
	CaptureMouse();
	RefreshShapeRect();
}

void BlocksetEditorCtrl::UpdateShapeDrag(int sx, int sy)
{
	const int bw = static_cast<int>(Landstalker::MapBlock::GetBlockWidth());
	const int bh = static_cast<int>(Landstalker::MapBlock::GetBlockHeight());
	const wxPoint next(std::clamp(sx, 0, m_columns * bw - 1),
	                   std::clamp(sy, 0, m_rows * bh - 1));
	if (next != m_shape_current)
	{
		RefreshShapeRect();
		m_shape_current = next;
		RefreshShapeRect();
	}
}

void BlocksetEditorCtrl::CommitShapeDrag()
{
	if (HasCapture())
	{
		ReleaseMouse();
	}
	if (!m_shape_active)
	{
		return;
	}
	m_shape_active = false;
	auto snapshot = Landstalker::Blockset(*m_blocks);
	bool changed = false;
	for (const auto& pt : MakeShapeSlots())
	{
		// Slots over missing blocks in a partial last row are skipped.
		changed |= SetSlotTile(pt.x, pt.y, m_drawtile);
	}
	if (changed)
	{
		PushUndoState(std::move(snapshot));
		m_tiles_bmp_dirty = true;
		// Nudges the frame to refresh undo/redo enablement after the edit.
		FireBlockEvent(EVT_BLOCK_SELECT, "");
	}
	// Repaint regardless, to clear the preview overlay.
	RefreshShapeRect();
}

void BlocksetEditorCtrl::CancelShapeDrag()
{
	if (HasCapture())
	{
		ReleaseMouse();
	}
	if (!m_shape_active)
	{
		return;
	}
	m_shape_active = false;
	RefreshShapeRect();
}

std::vector<wxPoint> BlocksetEditorCtrl::MakeShapeSlots() const
{
	ShapeTool tool = ShapeTool::Line;
	switch (m_mode)
	{
	case Mode::LINE:              tool = ShapeTool::Line;             break;
	case Mode::RECTANGLE_OUTLINE: tool = ShapeTool::RectangleOutline; break;
	case Mode::RECTANGLE_FILLED:  tool = ShapeTool::RectangleFilled;  break;
	case Mode::CIRCLE_OUTLINE:    tool = ShapeTool::CircleOutline;    break;
	case Mode::CIRCLE_FILLED:     tool = ShapeTool::CircleFilled;     break;
	default:
		return {};
	}
	return MakeShapeToolPoints(tool, m_shape_anchor, m_shape_current);
}

void BlocksetEditorCtrl::RefreshShapeRect()
{
	// Every shape lies within the drag's bounding rectangle, so that is all that repaints.
	RefreshBoxRect(wxRect(wxPoint(std::min(m_shape_anchor.x, m_shape_current.x),
	                              std::min(m_shape_anchor.y, m_shape_current.y)),
	                      wxSize(std::abs(m_shape_current.x - m_shape_anchor.x) + 1,
	                             std::abs(m_shape_current.y - m_shape_anchor.y) + 1)));
}

void BlocksetEditorCtrl::DrawShapePreview(wxDC& dc)
{
	if (!m_shape_active)
	{
		return;
	}
	const int ntw = static_cast<int>(m_tileset->GetTileWidth());
	const int nth = static_cast<int>(m_tileset->GetTileHeight());
	// One native-resolution bitmap of the draw tile, blitted per slot: a filled shape can
	// cover hundreds of slots and building a bitmap for each is visibly slow.
	wxBitmap bmp = MakeTileBitmap(m_tileset->GetTileBGRA(m_drawtile, GetSelectedPalette()), ntw, nth);
	wxMemoryDC tdc(bmp);
	dc.SetPen(*wxTRANSPARENT_PEN);
	for (const auto& pt : MakeShapeSlots())
	{
		if (!IsSlotValid(pt.x, pt.y))
		{
			continue;
		}
		// Backdrop first, so the tile's transparent pixels show the checkerboard rather
		// than the slot content underneath.
		dc.SetBrush(m_enablealpha ? *m_alpha_brush : *wxBLACK_BRUSH);
		dc.DrawRectangle(pt.x * m_tilewidth, pt.y * m_tileheight, m_tilewidth, m_tileheight);
		dc.StretchBlit({ pt.x * m_tilewidth, pt.y * m_tileheight }, { m_tilewidth, m_tileheight },
			&tdc, { 0, 0 }, { ntw, nth }, wxCOPY, true, { 0, 0 });
	}
	tdc.SelectObject(wxNullBitmap);
}

void BlocksetEditorCtrl::FloodFillAt(int sx, int sy)
{
	if (!IsSlotValid(sx, sy))
	{
		return;
	}
	const Landstalker::Tile target = GetSlotTile(sx, sy);
	if (target == m_drawtile)
	{
		return;
	}
	auto snapshot = Landstalker::Blockset(*m_blocks);
	// Painted slots no longer match the target, which doubles as the visited check.
	// Missing blocks in a partial last row read as the invalid sentinel and never match.
	std::vector<wxPoint> stack{ { sx, sy } };
	while (!stack.empty())
	{
		const wxPoint p = stack.back();
		stack.pop_back();
		if (!IsSlotValid(p.x, p.y) || (GetSlotTile(p.x, p.y) != target))
		{
			continue;
		}
		SetSlotTile(p.x, p.y, m_drawtile);
		stack.push_back({ p.x + 1, p.y });
		stack.push_back({ p.x - 1, p.y });
		stack.push_back({ p.x, p.y + 1 });
		stack.push_back({ p.x, p.y - 1 });
	}
	PushUndoState(std::move(snapshot));
	m_tiles_bmp_dirty = true;
	Refresh();
	// Nudges the frame to refresh undo/redo enablement after the edit.
	FireBlockEvent(EVT_BLOCK_SELECT, "");
}

void BlocksetEditorCtrl::BeginStroke(int sx, int sy)
{
	// Strokes must start on a real slot; the drag itself may then wander and clamp.
	if (!IsSlotValid(sx, sy))
	{
		return;
	}
	m_stroke_active = true;
	m_stroke_changed = false;
	m_stroke_snapshot = std::make_unique<Landstalker::Blockset>(*m_blocks);
	m_stroke_last = wxPoint(sx, sy);
	CaptureMouse();
	if (SetSlotTile(sx, sy, m_drawtile))
	{
		m_stroke_changed = true;
		m_tiles_bmp_dirty = true;
		RefreshBoxRect(wxRect(sx, sy, 1, 1));
	}
}

void BlocksetEditorCtrl::StrokeTo(int sx, int sy)
{
	const int bw = static_cast<int>(Landstalker::MapBlock::GetBlockWidth());
	const int bh = static_cast<int>(Landstalker::MapBlock::GetBlockHeight());
	const wxPoint next(std::clamp(sx, 0, m_columns * bw - 1),
	                   std::clamp(sy, 0, m_rows * bh - 1));
	if (next == m_stroke_last)
	{
		return;
	}
	// Mouse moves arrive coalesced, so consecutive samples can be several slots apart;
	// joining them with a line keeps fast strokes continuous, as in the pixel editors.
	PlotShapeLine(m_stroke_last, next, [&](int px, int py)
	{
		// Slots over missing blocks in a partial last row are skipped.
		if (SetSlotTile(px, py, m_drawtile))
		{
			m_stroke_changed = true;
			m_tiles_bmp_dirty = true;
			RefreshBoxRect(wxRect(px, py, 1, 1));
		}
	});
	m_stroke_last = next;
}

void BlocksetEditorCtrl::EndStroke()
{
	if (HasCapture())
	{
		ReleaseMouse();
	}
	if (!m_stroke_active)
	{
		return;
	}
	m_stroke_active = false;
	// Repainting slots with the tiles they already held leaves nothing to undo.
	const bool push = m_stroke_changed && m_stroke_snapshot && !(*m_blocks == *m_stroke_snapshot);
	if (push)
	{
		PushUndoState(std::move(*m_stroke_snapshot));
		// Nudges the frame to refresh undo/redo enablement after the edit.
		FireBlockEvent(EVT_BLOCK_SELECT, "");
	}
	m_stroke_snapshot.reset();
	m_stroke_changed = false;
}

void BlocksetEditorCtrl::CancelStroke()
{
	if (HasCapture())
	{
		ReleaseMouse();
	}
	if (!m_stroke_active)
	{
		return;
	}
	m_stroke_active = false;
	if (m_stroke_snapshot && m_stroke_changed)
	{
		// Puts the blockset back exactly as it was before the stroke started.
		*m_blocks = std::move(*m_stroke_snapshot);
		m_tiles_bmp_dirty = true;
		Refresh();
	}
	m_stroke_snapshot.reset();
	m_stroke_changed = false;
}

void BlocksetEditorCtrl::OnDraw(wxDC& dc)
{
	// Same pipeline as the tileset editor: everything renders at native resolution into
	// m_tiles_bmp when data changes, and each paint is one scaled blit of the damaged
	// cells plus overlays, all clipped to the damaged area. The previous design - a
	// zoom-scaled cache bitmap filled with per-tile draws - took seconds to open a
	// blockset and grew with the square of the zoom factor.
	dc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE)));
	if ((m_blocks == nullptr) || (m_tileset == nullptr) || (m_pal == nullptr) || (m_columns <= 0))
	{
		dc.Clear();
		return;
	}
	if (m_tiles_bmp_dirty || (m_tiles_bmp == nullptr))
	{
		RenderTilesBitmap();
	}
	if (m_tiles_bmp == nullptr)
	{
		dc.Clear();
		return;
	}

	wxRect damage = GetUpdateRegion().GetBox();
	damage.Offset(0, GetVisibleRowsBegin() * m_cellheight);
	dc.SetClippingRegion(damage);
	dc.Clear();

	int s = GetVisibleRowsBegin();
	int e = std::min(static_cast<int>(GetVisibleRowsEnd()) + 1, m_rows);
	s = std::max(s, damage.GetTop() / m_cellheight);
	e = std::min(e, damage.GetBottom() / m_cellheight + 1);
	const int c0 = std::max(0, damage.GetLeft() / m_cellwidth);
	const int c1 = std::min(m_columns, damage.GetRight() / m_cellwidth + 1);
	if ((e <= s) || (c1 <= c0))
	{
		return;
	}
	const int count = static_cast<int>(m_blocks->size());

	// Backdrop for the transparent colour: full-width rows in one rectangle, plus the
	// partial last row if it is in view.
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(m_enablealpha ? *m_alpha_brush : *wxBLACK_BRUSH);
	const int last_full_row = count / m_columns;
	const int full_rows_end = std::min(e, last_full_row);
	if (full_rows_end > s)
	{
		dc.DrawRectangle(0, s * m_cellheight, m_columns * m_cellwidth, (full_rows_end - s) * m_cellheight);
	}
	const int remainder = count % m_columns;
	if ((remainder > 0) && (last_full_row >= s) && (last_full_row < e))
	{
		dc.DrawRectangle(0, last_full_row * m_cellheight, remainder * m_cellwidth, m_cellheight);
	}

	// One scaled blit of the damaged cells out of the native-resolution blockset bitmap.
	const int ntw = static_cast<int>(m_tileset->GetTileWidth()) * GetBlockWidth();
	const int nth = static_cast<int>(m_tileset->GetTileHeight()) * GetBlockHeight();
	wxMemoryDC tiles(*m_tiles_bmp);
	dc.StretchBlit({ c0 * m_cellwidth, s * m_cellheight }, { (c1 - c0) * m_cellwidth, (e - s) * m_cellheight },
		&tiles, { c0 * ntw, s * nth }, { (c1 - c0) * ntw, (e - s) * nth },
		wxCOPY, true, { c0 * ntw, s * nth });
	tiles.SelectObject(wxNullBitmap);

	DrawOverlays(dc, s, e, c0, c1);
	DrawSelectionBorders(dc);
	DrawBoxSelection(dc);
	DrawShapePreview(dc);
}

void BlocksetEditorCtrl::DrawOverlays(wxDC& dc, int s, int e, int c0, int c1)
{
	dc.SetTextForeground(wxColour(255, 255, 255));
	dc.SetTextBackground(wxColour(150, 150, 150));
	dc.SetBackgroundMode(wxSOLID);

	if (m_pixelsize > 1)
	{
		m_border_pen->SetStyle(wxPENSTYLE_SOLID);
	}
	else
	{
		m_border_pen->SetStyle(wxPENSTYLE_TRANSPARENT);
	}

	const int count = static_cast<int>(m_blocks->size());
	const int bw = static_cast<int>(Landstalker::MapBlock::GetBlockWidth());
	const int bh = static_cast<int>(Landstalker::MapBlock::GetBlockHeight());
	for (int y = s; y < e; ++y)
	{
		for (int x = c0; x < c1; ++x)
		{
			const int i = x + y * m_columns;
			if (i >= count)
			{
				break;
			}
			const auto& block = m_blocks->at(i);
			if (m_enableborders)
			{
				dc.SetBrush(*wxTRANSPARENT_BRUSH);
				dc.SetPen(*m_tile_border_pen);
				for (int t = 0; t < bw * bh; ++t)
				{
					dc.DrawRectangle({ x * m_cellwidth + (t % bw) * m_tilewidth,
					                   y * m_cellheight + (t / bw) * m_tileheight,
					                   m_tilewidth + 1, m_tileheight + 1 });
				}
				dc.SetPen(*m_border_pen);
				dc.DrawRectangle({ x * m_cellwidth, y * m_cellheight, m_cellwidth + 1, m_cellheight + 1 });
				DrawBlockPriority(dc, x, y, block);
			}
			if (m_enableblocknumbers)
			{
				for (int t = 0; t < bw * bh; ++t)
				{
					const auto& tile = block.GetTile(t);
					auto label = Landstalker::StrPrintf("%03d%s%s%s", tile.GetIndex(),
						tile.Attributes().getAttribute(Landstalker::TileAttributes::Attribute::ATTR_HFLIP) ? "H" : "",
						tile.Attributes().getAttribute(Landstalker::TileAttributes::Attribute::ATTR_VFLIP) ? "V" : "",
						tile.Attributes().getAttribute(Landstalker::TileAttributes::Attribute::ATTR_PRIORITY) ? "P" : "");
					auto extent = dc.GetTextExtent(label);
					if ((extent.GetWidth() < m_tilewidth - 2) && (extent.GetHeight() < m_tileheight - 2))
					{
						dc.DrawText(label, {
							x * m_cellwidth + (t % bw) * m_tilewidth + 2,
							y * m_cellheight + (t / bw) * m_tileheight + 2
						});
					}
				}
			}
		}
	}
}

void BlocksetEditorCtrl::OnPaint(wxPaintEvent& /*evt*/)
{
	wxBufferedPaintDC dc(this);
	this->PrepareDC(dc);
	this->OnDraw(dc);
}

void BlocksetEditorCtrl::OnSize(wxSizeEvent& evt)
{
	this->GetClientSize(&m_ctrlwidth, &m_ctrlheight);
	if (UpdateRowCount())
	{
		ForceRedraw();
	}
	wxVScrolledWindow::HandleOnSize(evt);
	Refresh(false);
}

void BlocksetEditorCtrl::OnMouseMove(wxMouseEvent& evt)
{
	if (m_sel_drag != SelDrag::None)
	{
		const wxPoint slot = RawSlotFromPoint(evt.GetPosition());
		UpdateBoxDrag(slot.x, slot.y);
		evt.Skip();
		return;
	}
	if (m_shape_active)
	{
		const wxPoint slot = RawSlotFromPoint(evt.GetPosition());
		UpdateShapeDrag(slot.x, slot.y);
		evt.Skip();
		return;
	}
	if (m_stroke_active)
	{
		// Paint, then fall through so the hover tracking stays current.
		const wxPoint slot = RawSlotFromPoint(evt.GetPosition());
		StrokeTo(slot.x, slot.y);
	}
	if (m_enablehover)
	{
		auto block_idx = ConvertXYToBlockIdx(evt.GetPosition());
		auto tile_idx = ConvertXYToTileIdx(evt.GetPosition());
		switch (m_mode)
		{
		case Mode::BLOCK_SELECT:
			if (m_hoveredblock != block_idx)
			{
				const int old = m_hoveredblock;
				m_hoveredblock = block_idx;
				FireBlockEvent(EVT_BLOCK_HOVER, "");
				RefreshStatusbar();
				RefreshBlock(old);
				RefreshBlock(m_hoveredblock);
			}
			break;
		case Mode::TILE_SELECT:
		case Mode::PENCIL:
		case Mode::LINE:
		case Mode::RECTANGLE_OUTLINE:
		case Mode::RECTANGLE_FILLED:
		case Mode::CIRCLE_OUTLINE:
		case Mode::CIRCLE_FILLED:
		case Mode::FILL:
			if (m_hoveredblock != block_idx || m_hoveredtile != tile_idx)
			{
				const int old = m_hoveredblock;
				m_hoveredblock = block_idx;
				m_hoveredtile = tile_idx;
				FireBlockEvent(EVT_BLOCK_HOVER, "");
				RefreshStatusbar();
				RefreshBlock(old);
				RefreshBlock(m_hoveredblock);
			}
			break;
		default:
			break;
		}
	}
	evt.Skip();
}

void BlocksetEditorCtrl::OnMouseLeave(wxMouseEvent& evt)
{
	if ((m_sel_drag != SelDrag::None) || m_shape_active || m_stroke_active)
	{
		// The mouse is captured; the drag continues outside the window.
		evt.Skip();
		return;
	}
	if (m_enablehover)
	{
		if (m_hoveredblock != -1)
		{
			const int old = m_hoveredblock;
			m_hoveredblock = -1;
			m_hoveredtile = -1;
			FireBlockEvent(EVT_BLOCK_HOVER, "");
			RefreshStatusbar();
			RefreshBlock(old);
		}
	}
	evt.Skip();
}

void BlocksetEditorCtrl::OnMouseDown(wxMouseEvent& evt)
{
	// Clicking the canvas takes the keyboard, so Esc and the Ctrl shortcuts work without
	// first having to tab into the window.
	SetFocus();
	if (m_mode == Mode::BOX_SELECT)
	{
		if (evt.LeftDown())
		{
			const wxPoint slot = RawSlotFromPoint(evt.GetPosition());
			BeginBoxAction(slot.x, slot.y);
		}
		evt.Skip();
		return;
	}
	if (IsShapeMode(m_mode) || (m_mode == Mode::FILL))
	{
		const wxPoint slot = RawSlotFromPoint(evt.GetPosition());
		if (evt.LeftDown() || evt.LeftDClick())
		{
			if (m_mode == Mode::FILL)
			{
				FloodFillAt(slot.x, slot.y);
			}
			else
			{
				BeginShapeDrag(slot.x, slot.y);
			}
		}
		else if (evt.RightDown() && !m_shape_active)
		{
			// Same as the pencil: right-click picks up the tile under the cursor.
			const auto tile = GetSlotTile(slot.x, slot.y);
			if (tile.GetTileValue() != SEL_INVALID_TILE)
			{
				m_drawtile = tile;
				FireEvent(EVT_TILE_SELECT, std::to_string(m_drawtile.GetIndex()));
			}
		}
		evt.Skip();
		return;
	}
	int block_idx = ConvertXYToBlockIdx(evt.GetPosition());
	int tile_idx = ConvertXYToTileIdx(evt.GetPosition());
	const int old_hover = m_hoveredblock;
	const int old_selection = m_selectedblock;
	bool refresh = false;
	switch (m_mode)
	{
	case Mode::BLOCK_SELECT:
		if (m_enablehover && m_hoveredblock != block_idx)
		{
			m_hoveredblock = block_idx;
			FireBlockEvent(EVT_BLOCK_HOVER, "");
			refresh = true;
		}
		if (m_enableselection && m_selectedblock != block_idx)
		{
			m_selectedblock = block_idx;
			FireBlockEvent(EVT_BLOCK_SELECT, "");
			refresh = true;
		}
		break;
	case Mode::TILE_SELECT:
		if (m_enablehover && (m_hoveredblock != block_idx || m_hoveredtile != tile_idx))
		{
			m_hoveredblock = block_idx;
			m_hoveredtile = tile_idx;
			FireBlockEvent(EVT_BLOCK_HOVER, "");
			refresh = true;
		}
		if (m_enableselection && (m_selectedblock != block_idx || m_selectedtile != tile_idx))
		{
			m_selectedblock = block_idx;
			m_selectedtile = tile_idx;
			FireBlockEvent(EVT_BLOCK_SELECT, "");
			refresh = true;
		}
		break;
	case Mode::PENCIL:
		if (evt.LeftDown() || evt.LeftDClick())
		{
			const wxPoint slot = RawSlotFromPoint(evt.GetPosition());
			BeginStroke(slot.x, slot.y);
		}
		else if (evt.RightDown() && !m_stroke_active)
		{
			m_drawtile = GetHoveredTile();
			FireEvent(EVT_TILE_SELECT, std::to_string(m_drawtile.GetIndex()));
		}
		break;
	default:
		break;
	}
	if (refresh)
	{
		RefreshStatusbar();
		RefreshBlock(old_hover);
		RefreshBlock(old_selection);
		RefreshBlock(block_idx);
	}
	evt.Skip();
}

void BlocksetEditorCtrl::OnMouseUp(wxMouseEvent& evt)
{
	if (evt.LeftUp() && (m_sel_drag != SelDrag::None))
	{
		if (evt.RightIsDown())
		{
			// Releasing the left button with the right still held cancels the operation.
			CancelBoxDrag();
		}
		else
		{
			FinishBoxDrag();
		}
	}
	else if (evt.LeftUp() && m_shape_active)
	{
		if (evt.RightIsDown())
		{
			CancelShapeDrag();
		}
		else
		{
			CommitShapeDrag();
		}
	}
	else if (evt.LeftUp() && m_stroke_active)
	{
		if (evt.RightIsDown())
		{
			CancelStroke();
		}
		else
		{
			EndStroke();
		}
	}
	evt.Skip();
}

void BlocksetEditorCtrl::OnCaptureLost(wxMouseCaptureLostEvent& /*evt*/)
{
	// Capture already gone - drop the box-drag state, clear any shape preview off the
	// screen, and commit an in-progress stroke so its painted slots stay undoable.
	m_sel_drag = SelDrag::None;
	m_sel_snapshot.reset();
	m_sel_op_changed = false;
	if (m_shape_active)
	{
		CancelShapeDrag();
	}
	if (m_stroke_active)
	{
		EndStroke();
	}
}

void BlocksetEditorCtrl::FireUpdateStatusEvent(const std::string& caption, int pane)
{
	wxCommandEvent evt(EVT_STATUSBAR_UPDATE);
	evt.SetString(caption);
	evt.SetInt(pane);
	evt.SetClientData(m_frame);
	wxPostEvent(m_frame->GetParent(), evt);
}

void BlocksetEditorCtrl::FireEvent(const wxEventType& e, const std::string& data)
{
	wxCommandEvent evt(e);
	evt.SetString(data);
	evt.SetClientData(GetParent());
	wxPostEvent(m_frame, evt);
}

void BlocksetEditorCtrl::FireTilesetEvent(const wxEventType& e, const std::string& data)
{
	wxCommandEvent evt(e);
	evt.SetInt(m_selectedtile);
	evt.SetExtraLong(m_selectedblock);
	evt.SetString(data);
	evt.SetClientData(GetParent());
	wxPostEvent(m_frame, evt);
}

void BlocksetEditorCtrl::FireBlockEvent(const wxEventType& e, const std::string& data)
{
	wxCommandEvent evt(e);
	evt.SetInt(m_selectedblock);
	evt.SetExtraLong(m_selectedtile);
	evt.SetString(data);
	evt.SetClientData(GetParent());
	wxPostEvent(m_frame, evt);
}
