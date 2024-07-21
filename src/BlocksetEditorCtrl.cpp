#include "BlocksetEditorCtrl.h"

#include <wx/wx.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/dcbuffer.h>
#include <EditorFrame.h>

wxBEGIN_EVENT_TABLE(BlocksetEditorCtrl, wxVScrolledWindow)
EVT_PAINT(BlocksetEditorCtrl::OnPaint)
EVT_SIZE(BlocksetEditorCtrl::OnSize)
EVT_LEFT_DOWN(BlocksetEditorCtrl::OnMouseDown)
EVT_RIGHT_DOWN(BlocksetEditorCtrl::OnMouseDown)
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
	  m_blocks(std::make_shared<Blockset>()),
	  m_mode(Mode::BLOCK_SELECT),
	  m_columns(0),
	  m_rows(0),
	  m_pixelsize(4),
   	  m_selectable(false),
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
	  m_redraw_all(true),
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

void BlocksetEditorCtrl::SetGameData(std::shared_ptr<GameData> gd)
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
	m_drawtile = Tile();
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
	m_drawtile = Tile();
	m_blocks = m_gd->GetRoomData()->GetCombinedBlocksetForRoom(roomnum);
	auto tse = m_gd->GetRoomData()->GetTilesetForRoom(roomnum);
	m_tileset = tse->GetData();
	m_pal_name = m_gd->GetRoomData()->GetPaletteForRoom(roomnum)->GetName();
	m_pal = m_gd->GetRoomData()->GetPaletteForRoom(roomnum)->GetData();
	m_enablealpha = false;
	m_enabletileborders = false;
	m_selectable = true;
	m_pixelsize = 2;
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
		m_redraw_list.insert(index);
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
		m_redraw_list.insert(index);
		Refresh(false);
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
	return MapBlock::GetBlockWidth();
}

int BlocksetEditorCtrl::GetBlockHeight() const
{
	return MapBlock::GetBlockHeight();
}

std::shared_ptr<Tileset> BlocksetEditorCtrl::GetTileset()
{
	return m_tileset;
}

std::shared_ptr<Palette> BlocksetEditorCtrl::GetPalette()
{
	return m_pal;
}

std::shared_ptr<std::vector<MapBlock>> BlocksetEditorCtrl::GetBlocks()
{
	return m_blocks;
}

void BlocksetEditorCtrl::SetMode(const Mode& mode)
{
	if (m_mode != mode)
	{
		m_mode = mode;
		m_redraw_list.insert(m_selectedblock);
		m_selectedtile = IsBlockSelectionValid() ? 0 : -1;
		m_selectedblock = IsBlockSelectionValid() ? m_selectedblock : -1;
		Refresh();
	}
}

BlocksetEditorCtrl::Mode BlocksetEditorCtrl::GetMode() const
{
	return m_mode;
}

void BlocksetEditorCtrl::SetDrawTile(const Tile& tile)
{
	m_drawtile = tile;
	RefreshStatusbar();
}

Tile BlocksetEditorCtrl::GetDrawTile()
{
	return m_drawtile;
}

void BlocksetEditorCtrl::ToggleHFlip(int block_idx, int tile_idx)
{
	if (IsTileIndexValid(tile_idx) && IsBlockIndexValid(block_idx))
	{
		auto tile = GetTile(block_idx, tile_idx);
		tile.Attributes().toggleAttribute(TileAttributes::Attribute::ATTR_HFLIP);
		SetTile(block_idx, tile_idx, tile);
	}
}

void BlocksetEditorCtrl::ToggleVFlip(int block_idx, int tile_idx)
{
	if (IsTileIndexValid(tile_idx) && IsBlockIndexValid(block_idx))
	{
		auto tile = GetTile(block_idx, tile_idx);
		tile.Attributes().toggleAttribute(TileAttributes::Attribute::ATTR_VFLIP);
		SetTile(block_idx, tile_idx, tile);
	}
}

void BlocksetEditorCtrl::TogglePriority(int block_idx, int tile_idx)
{
	if (IsTileIndexValid(tile_idx) && IsBlockIndexValid(block_idx))
	{
		auto tile = GetTile(block_idx, tile_idx);
		tile.Attributes().toggleAttribute(TileAttributes::Attribute::ATTR_PRIORITY);
		SetTile(block_idx, tile_idx, tile);
	}
}

void BlocksetEditorCtrl::ToggleSelectedHFlip()
{
	if (IsTileSelectionValid())
	{
		auto tile = GetSelectedTile();
		tile.Attributes().toggleAttribute(TileAttributes::Attribute::ATTR_HFLIP);
		SetSelectedTile(tile);
	}
}

void BlocksetEditorCtrl::ToggleSelectedVFlip()
{
	if (IsTileSelectionValid())
	{
		auto tile = GetSelectedTile();
		tile.Attributes().toggleAttribute(TileAttributes::Attribute::ATTR_VFLIP);
		SetSelectedTile(tile);
	}
}

void BlocksetEditorCtrl::ToggleSelectedPriority()
{
	if (IsTileSelectionValid())
	{
		auto tile = GetSelectedTile();
		tile.Attributes().toggleAttribute(TileAttributes::Attribute::ATTR_PRIORITY);
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
			tile.Attributes().setAttribute(TileAttributes::Attribute::ATTR_HFLIP);
		}
		else
		{
			tile.Attributes().clearAttribute(TileAttributes::Attribute::ATTR_HFLIP);
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
			tile.Attributes().setAttribute(TileAttributes::Attribute::ATTR_VFLIP);
		}
		else
		{
			tile.Attributes().clearAttribute(TileAttributes::Attribute::ATTR_VFLIP);
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
			tile.Attributes().setAttribute(TileAttributes::Attribute::ATTR_PRIORITY);
		}
		else
		{
			tile.Attributes().clearAttribute(TileAttributes::Attribute::ATTR_PRIORITY);
		}
		SetSelectedTile(tile);
	}
}

bool BlocksetEditorCtrl::GetSelectedHFlip() const
{
	if (IsTileSelectionValid())
	{
		return GetSelectedTile().Attributes().getAttribute(TileAttributes::Attribute::ATTR_HFLIP);
	}
	return false;
}

bool BlocksetEditorCtrl::GetSelectedVFlip() const
{
	if (IsTileSelectionValid())
	{
		return GetSelectedTile().Attributes().getAttribute(TileAttributes::Attribute::ATTR_VFLIP);
	}
	return false;
}

bool BlocksetEditorCtrl::GetSelectedPriority() const
{
	if (IsTileSelectionValid())
	{
		return GetSelectedTile().Attributes().getAttribute(TileAttributes::Attribute::ATTR_PRIORITY);
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
		m_blocks->insert(m_blocks->cbegin() + row, MapBlock());
		for (int i = row; i < static_cast<int>(m_blocks->size()); ++i)
		{
			m_redraw_list.insert(i);
			Refresh(true);
		}
		return true;
	}
	return false;
}

bool BlocksetEditorCtrl::DeleteBlock(int row)
{
	if (m_blocks && row >= 0 && row < static_cast<int>(m_blocks->size()))
	{
		m_blocks->erase(m_blocks->cbegin() + row);
		for (int i = row; i < static_cast<int>(m_blocks->size()); ++i)
		{
			m_redraw_list.insert(i);
			Refresh(true);
		}
		return true;
	}
	return false;
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
		if (m_selectedblock != -1)
		{
			m_redraw_list.insert(m_selectedblock);
		}
		m_selectedblock = b;
		if (m_selectedblock != -1)
		{
			m_redraw_list.insert(m_selectedblock);
			std::size_t row = m_selectedblock / m_columns;
			if (row >= (GetVisibleRowsEnd() - 1) || row < GetVisibleRowsBegin())
			{
				ScrollToRow(row);
			}
		}
		FireBlockEvent(EVT_BLOCK_SELECT, "");
		RefreshStatusbar();
		Refresh();
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
		if (m_hoveredblock != -1)
		{
			m_redraw_list.insert(m_hoveredblock);
		}
		m_hoveredblock = b;
		m_hoveredtile = t;
		if (m_hoveredblock != -1)
		{
			m_redraw_list.insert(m_hoveredblock);
			std::size_t row = m_hoveredblock / m_columns;
			if (row >= (GetVisibleRowsEnd() - 1) || row < GetVisibleRowsBegin())
			{
				ScrollToRow(row);
			}
		}
		RefreshStatusbar();
		Refresh();
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
		if (m_selectedblock != -1)
		{
			m_redraw_list.insert(m_selectedblock);
		}
		m_selectedblock = b;
		m_selectedtile = t;
		if (m_selectedblock != -1)
		{
			m_redraw_list.insert(m_selectedblock);
			std::size_t row = m_selectedblock / m_columns;
			if (row >= (GetVisibleRowsEnd() - 1) || row < GetVisibleRowsBegin())
			{
				ScrollToRow(row);
			}
		}
		FireEvent(EVT_BLOCK_SELECT, "");
		RefreshStatusbar();
		Refresh();
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

MapBlock BlocksetEditorCtrl::GetSelectedBlock() const
{
	if (IsBlockSelectionValid())
	{
		return m_blocks->at(m_selectedblock);
	}
	else
	{
		return MapBlock();
	}
}

Tile BlocksetEditorCtrl::GetSelectedTile() const
{
	if (m_blocks && IsTileSelectionValid())
	{
		return GetTile(m_selectedblock, m_selectedtile);
	}
	return Tile();
}

void BlocksetEditorCtrl::SetSelectedBlock(const MapBlock& block)
{
	if (IsBlockSelectionValid())
	{
		SetBlock(m_selectedblock, block);
	}
}

void BlocksetEditorCtrl::SetSelectedTile(const Tile& tile)
{
	if (IsTileSelectionValid() && tile != GetSelectedTile())
	{
		SetTile(m_selectedblock, m_selectedtile, tile);
	}
}

MapBlock BlocksetEditorCtrl::GetHoveredBlock() const
{
	if (IsBlockHoverValid())
	{
		return m_blocks->at(m_hoveredblock);
	}
	else
	{
		return MapBlock();
	}
}

Tile BlocksetEditorCtrl::GetHoveredTile() const
{
	if (IsTileHoverValid())
	{
		return GetTile(m_hoveredblock, m_hoveredtile);
	}
	else
	{
		return Tile();
	}
}

void BlocksetEditorCtrl::SetHoveredTile(const Tile& tile)
{
	if (IsTileHoverValid() && tile != GetHoveredTile())
	{
		SetTile(m_hoveredblock, m_hoveredtile, tile);
	}
}

MapBlock BlocksetEditorCtrl::GetBlock(int index) const
{
	if (m_blocks && IsBlockIndexValid(index))
	{
		return m_blocks->at(index);
	}
	return MapBlock();
}

Tile BlocksetEditorCtrl::GetTile(int block_idx, int tile_idx) const
{
	if (m_blocks && IsBlockIndexValid(block_idx) && IsTileIndexValid(tile_idx))
	{
		return m_blocks->at(block_idx).GetTile(tile_idx);
	}
	return Tile();
}

void BlocksetEditorCtrl::SetBlock(int block, const MapBlock& new_block)
{
	if (m_blocks && IsBlockIndexValid(block))
	{
		m_blocks->at(block) = new_block;
		m_redraw_list.insert(block);
		Refresh();
	}
}

void BlocksetEditorCtrl::SetTile(int block_idx, int tile_idx, const Tile& new_tile)
{
	if (m_blocks && IsBlockIndexValid(block_idx) && IsTileIndexValid(tile_idx))
	{
		m_blocks->at(block_idx).SetTile(tile_idx, new_tile);
		m_redraw_list.insert(block_idx);
		Refresh();
	}
}

bool BlocksetEditorCtrl::IsBlockIndexValid(int block_index) const
{
	return (m_blocks && (block_index >= 0) && (block_index < static_cast<int>(m_blocks->size())));
}

bool BlocksetEditorCtrl::IsTileIndexValid(int tile_index) const
{
	return (tile_index >= 0 && tile_index < static_cast<int>(MapBlock::GetBlockSize()));
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
			FireUpdateStatusEvent(StrPrintf("Hovered: %04d", GetBlockHover()), 0);
		}
		else
		{
			FireUpdateStatusEvent("", 0);
		}
		if (IsBlockSelectionValid())
		{
			FireUpdateStatusEvent(StrPrintf("Selected: %04d", GetBlockSelection()), 1);
		}
		else
		{
			FireUpdateStatusEvent("", 1);
		}
		break;
	case Mode::TILE_SELECT:
	case Mode::PENCIL:
		if (IsBlockHoverValid())
		{
			const auto& tile = GetHoveredTile();
			FireUpdateStatusEvent(StrPrintf("Hovered: %04d:%01d (%03d%s%s%s)", GetBlockHover(), GetTileHover(),
				tile.GetIndex(), tile.Attributes().getAttribute(TileAttributes::Attribute::ATTR_HFLIP) ? "H" : "",
				tile.Attributes().getAttribute(TileAttributes::Attribute::ATTR_VFLIP) ? "V" : "",
				tile.Attributes().getAttribute(TileAttributes::Attribute::ATTR_PRIORITY) ? "P" : ""), 0);
		}
		else
		{
			FireUpdateStatusEvent("", 0);
		}
		if (m_mode == Mode::TILE_SELECT && IsTileSelectionValid())
		{
			const auto& tile = GetSelectedTile();
			FireUpdateStatusEvent(StrPrintf("Selected: %04d:%01d (%03d%s%s%s)", GetBlockSelection(), GetTileSelection(),
				tile.GetIndex(), tile.Attributes().getAttribute(TileAttributes::Attribute::ATTR_HFLIP) ? "H" : "",
				tile.Attributes().getAttribute(TileAttributes::Attribute::ATTR_VFLIP) ? "V" : "",
				tile.Attributes().getAttribute(TileAttributes::Attribute::ATTR_PRIORITY) ? "P" : ""), 1);
		}
		else if (m_mode == Mode::PENCIL && m_drawtile >= 0)
		{
			FireUpdateStatusEvent(StrPrintf("Selected Tile: %03d", m_drawtile.GetIndex()), 1);
		}
		else
		{
			FireUpdateStatusEvent("", 1);
		}
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

bool BlocksetEditorCtrl::DrawTile(wxDC& dc, int x, int y, const Tile& tile)
{
	bool retval = false;
	int s = GetVisibleRowsBegin();
	int e = GetVisibleRowsEnd();

	const int tile_width = m_pixelsize * m_tileset->GetTileWidth();
	const int tile_height = m_pixelsize * m_tileset->GetTileHeight();
	const int xx = x * tile_width;
	const int yy = y * tile_height;
	const int ycell = y / MapBlock::GetBlockHeight();

	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	if ((ycell >= s) && (ycell < e))
	{
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.SetBrush(m_enablealpha ? *m_alpha_brush : *wxBLACK_BRUSH);
		dc.DrawRectangle({ xx, yy, tile_width, tile_height });
		DrawTilePixels(dc, xx, yy, tile);
		if (m_enableborders)
		{
			dc.SetPen(*m_tile_border_pen);
			dc.SetBrush(*wxTRANSPARENT_BRUSH);
			dc.DrawRectangle({ xx, yy, tile_width + 1, tile_height + 1 });
		}
		retval = true;
	}
	return retval;
}

void BlocksetEditorCtrl::DrawTilePixels(wxDC& dc, int x, int y, const Tile& tile)
{
	wxPen pen = dc.GetPen();
	wxBrush brush = dc.GetBrush();

	pen.SetStyle(wxPENSTYLE_TRANSPARENT);
	brush.SetStyle(wxBRUSHSTYLE_SOLID);
	dc.SetPen(pen);

	auto tile_pixels = m_tileset->GetTileBGRA(tile, GetSelectedPalette());

	for (std::size_t i = 0; i < tile_pixels.size(); ++i)
	{
		int xx = x + (i % m_tileset->GetTileWidth()) * m_pixelsize;
		int yy = y + (i / m_tileset->GetTileWidth()) * m_pixelsize;
		brush.SetColour(wxColour(tile_pixels[i]));
		dc.SetBrush(brush);
		// Has alpha
		if ((tile_pixels[i] & 0xFF000000) > 0)
		{
			dc.DrawRectangle({ xx, yy, m_pixelsize, m_pixelsize });
		}
	}
}

bool BlocksetEditorCtrl::DrawBlock(wxDC& dc, int x, int y, const MapBlock& block)
{
	bool retval = true;
	for (int i = 0; i < static_cast<int>(MapBlock::GetBlockSize()); ++i)
	{
		Position pos = {i % static_cast<int>(MapBlock::GetBlockWidth()), i / static_cast<int>(MapBlock::GetBlockWidth())};
		const int xx = x * MapBlock::GetBlockWidth() + pos.x;
		const int yy = y * MapBlock::GetBlockHeight() + pos.y;
		if (!DrawTile(dc, xx, yy, block.GetTile(pos.x, pos.y)))
		{
			retval = false;
		}
	}
	if (m_enableborders)
	{
		dc.SetPen(*m_border_pen);
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.DrawRectangle({ x * m_cellwidth, y * m_cellheight, m_cellwidth + 1, m_cellheight + 1 });
		DrawBlockPriority(dc, x, y, block);
	}
	if (m_enableblocknumbers)
	{
		for (int i = 0; i < static_cast<int>(MapBlock::GetBlockSize()); ++i)
		{
			const auto& tile = block.GetTile(i);
			auto label = StrPrintf("%03d%s%s%s", tile.GetIndex(),
				tile.Attributes().getAttribute(TileAttributes::Attribute::ATTR_HFLIP) ? "H" : "",
				tile.Attributes().getAttribute(TileAttributes::Attribute::ATTR_VFLIP) ? "V" : "",
				tile.Attributes().getAttribute(TileAttributes::Attribute::ATTR_PRIORITY) ? "P" : "");
			auto extent = dc.GetTextExtent(label);
			if ((extent.GetWidth() < m_tilewidth - 2) && (extent.GetHeight() <m_tileheight - 2))
			{
				dc.DrawText(label, {
					(x * GetBlockWidth() + (i % static_cast<int>(MapBlock::GetBlockWidth()))) * m_tilewidth + 2,
					(y * GetBlockHeight() + (i / static_cast<int>(MapBlock::GetBlockHeight()))) * m_tileheight + 2
				});
			}
		}
	}
	return retval;
}

bool BlocksetEditorCtrl::DrawBlockPriority(wxDC& dc, int x, int y, const MapBlock& block)
{
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.SetPen(*m_priority_pen);

	int pri_tile_count = 0;
	std::array<bool, MapBlock::GetBlockSize()> tile_priorities = {false, false, false, false};
	for (int i = 0; i < static_cast<int>(block.GetBlockSize()); ++i)
	{
		if (block.GetTile(i).Attributes().getAttribute(TileAttributes::Attribute::ATTR_PRIORITY))
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
					x * m_cellwidth + 1 + ((i % static_cast<int>(MapBlock::GetBlockWidth())) * m_tilewidth),
					y * m_cellheight + 1 + ((i / static_cast<int>(MapBlock::GetBlockWidth())) * m_tileheight),
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
	int hbx = (m_hoveredblock % m_columns) * m_cellwidth;
	int hby = (m_hoveredblock / m_columns) * m_cellheight;
	int sbx = (m_selectedblock % m_columns) * m_cellwidth;
	int sby = (m_selectedblock / m_columns) * m_cellheight;
	int htx = (m_hoveredtile % MapBlock::GetBlockWidth()) * m_tilewidth;
	int hty = (m_hoveredtile / MapBlock::GetBlockWidth()) * m_tileheight;
	int stx = (m_selectedtile % MapBlock::GetBlockWidth()) * m_tilewidth;
	int sty = (m_selectedtile / MapBlock::GetBlockWidth()) * m_tileheight;
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
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		if (m_hoveredblock != -1 && m_hoveredtile != -1)
		{
			dc.SetPen(*wxWHITE_PEN);
			dc.DrawRectangle({ hbx + htx, hby + hty, m_tilewidth, m_tileheight });
		}
	}
}

void BlocksetEditorCtrl::PaintBitmap(wxDC& dc)
{
	dc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE)));
	dc.Clear();
	int sy = GetVisibleRowsBegin() * m_cellheight;

	int vX, vY, vW, vH;                 // Dimensions of client area in pixels
	wxRegionIterator upd(GetUpdateRegion()); // get the update rect list
	while (upd)
	{
		vX = upd.GetX();
		vY = upd.GetY();
		vW = upd.GetW();
		vH = upd.GetH();
		// Alternatively we can do this:
		// wxRect rect(upd.GetRect());
		// Repaint this rectangle
		dc.Blit(vX, vY + sy, vW, vH, &m_memdc, vX, vY + sy);
		upd++;
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
	m_redraw_all = true;
	wxVarVScrollHelper::RefreshAll();
	Refresh();
}

Palette& BlocksetEditorCtrl::GetSelectedPalette()
{
	return *m_pal;
}

BlocksetEditorCtrl::Position BlocksetEditorCtrl::ToBlockPosition(int index) const
{
	if (index < 0) return { -1,-1 };
	const int x = index % m_columns;
	const int y = index / m_columns;
	return Position{ x,y };
}

int BlocksetEditorCtrl::ToBlockIndex(const Position& tp) const
{
	if ((tp.x < 0 || tp.x >= m_columns) || (tp.y < 0 || tp.y >= m_rows))
	{
		return -1;
	}
	std::size_t idx = tp.x + tp.y * m_columns;
	if (idx >= m_blocks->size())
	{
		return -1;
	}
	return idx;
}

BlocksetEditorCtrl::Position BlocksetEditorCtrl::ToTilePosition(int index)
{
	if (index < 0) return { -1,-1 };
	auto tp = ToBlockPosition(index / MapBlock::GetBlockSize());
	tp.x *= MapBlock::GetBlockWidth();
	tp.y *= MapBlock::GetBlockHeight();
	int surplus = index % MapBlock::GetBlockSize();
	tp.x += surplus % MapBlock::GetBlockWidth();
	tp.y += surplus / MapBlock::GetBlockHeight();
	return tp;
}

int BlocksetEditorCtrl::ToTileIndex(const Position& tp)
{
	auto bpos = Position{ tp.x / static_cast<int>(MapBlock::GetBlockWidth()),
						 tp.y / static_cast<int>(MapBlock::GetBlockHeight()) };
	auto bidx = ToBlockIndex(bpos);
	if (bidx < 0)
	{
		return -1;
	}
	auto tidx = bidx * MapBlock::GetBlockSize();
	tidx += (tp.x % MapBlock::GetBlockWidth());
	tidx += (tp.y % MapBlock::GetBlockHeight()) * MapBlock::GetBlockWidth();
	return tidx;
}

int BlocksetEditorCtrl::ConvertXYToBlockIdx(const wxPoint& point) const
{
	if (m_tileset == nullptr)
	{
		return -1;
	}
	int s = GetVisibleRowsBegin();
	int x = point.x / (m_pixelsize * m_tileset->GetTileWidth() * MapBlock::GetBlockWidth());
	int y = s + point.y / (m_pixelsize * m_tileset->GetTileHeight() * MapBlock::GetBlockHeight());
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
	int xx = point.x % (m_pixelsize * m_tileset->GetTileWidth() * MapBlock::GetBlockWidth());
	int yy = point.y % (m_pixelsize * m_tileset->GetTileHeight() * MapBlock::GetBlockHeight());
	int x = xx / (m_pixelsize * m_tileset->GetTileWidth());
	int y = yy / (m_pixelsize * m_tileset->GetTileHeight());
	int sel = x + y * MapBlock::GetBlockWidth();
	if ((sel >= static_cast<int>(MapBlock::GetBlockSize()) || (x < 0) || (y < 0) ||
		(x >= static_cast<int>(MapBlock::GetBlockWidth()))) || (y >= static_cast<int>(MapBlock::GetBlockHeight())))
	{
		sel = -1;
	}
	return sel;
}

void BlocksetEditorCtrl::OnDraw(wxDC& dc)
{
	if (m_redraw_all == true)
	{
		m_bmp.Create(m_cellwidth * m_columns + 1, m_cellheight * m_rows + 1);
	}
	m_memdc.SelectObject(m_bmp);
	if (m_redraw_all == true)
	{
		m_memdc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE)));
		m_memdc.Clear();
	}
	m_memdc.SetTextForeground(wxColour(255, 255, 255));
	m_memdc.SetTextBackground(wxColour(150, 150, 150));
	m_memdc.SetBackgroundMode(wxSOLID);

	if (m_pixelsize > 1)
	{
		m_border_pen->SetStyle(wxPENSTYLE_SOLID);
	}
	else
	{
		m_border_pen->SetStyle(wxPENSTYLE_TRANSPARENT);
	}

	m_memdc.SetBrush(*wxTRANSPARENT_BRUSH);
	if (m_redraw_all == true)
	{
		m_redraw_list.clear();
		for (std::size_t i = 0; i < m_blocks->size(); ++i)
		{
			const auto pos = ToBlockPosition(i);
			if (!DrawBlock(m_memdc, pos.x, pos.y, m_blocks->at(i)))
			{
				m_redraw_list.insert(i);
			}
		}
		m_redraw_all = false;
	}
	else
	{
		auto it = m_redraw_list.begin();
		while (it != m_redraw_list.end())
		{
			if ((*it >= 0) && (*it < static_cast<int>(m_blocks->size())))
			{
				auto pos = ToBlockPosition(*it);
				if (DrawBlock(m_memdc, pos.x, pos.y, m_blocks->at(*it)))
				{
					it = m_redraw_list.erase(it);
				}
				else
				{
					++it;
				}
			}
			else
			{
				it = m_redraw_list.erase(it);
			}
		}
	}

	DrawSelectionBorders(m_memdc);
	PaintBitmap(dc);
	m_memdc.SelectObject(wxNullBitmap);
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
	if (m_enablehover)
	{
		auto block_idx = ConvertXYToBlockIdx(evt.GetPosition());
		auto tile_idx = ConvertXYToTileIdx(evt.GetPosition());
		switch (m_mode)
		{
		case Mode::BLOCK_SELECT:
			if (m_hoveredblock != block_idx)
			{
				if (m_hoveredblock != -1)
				{
					m_redraw_list.insert(m_hoveredblock);
				}
				m_hoveredblock = block_idx;
				FireBlockEvent(EVT_BLOCK_HOVER, "");
				RefreshStatusbar();
				Refresh();
			}
			break;
		case Mode::TILE_SELECT:
		case Mode::PENCIL:
			if (m_hoveredblock != block_idx || m_hoveredtile != tile_idx)
			{
				if (m_hoveredblock != -1)
				{
					m_redraw_list.insert(m_hoveredblock);
				}
				m_hoveredblock = block_idx;
				m_hoveredtile = tile_idx;
				FireBlockEvent(EVT_BLOCK_HOVER, "");
				RefreshStatusbar();
				Refresh();
			}
			break;
		}
	}
	evt.Skip();
}

void BlocksetEditorCtrl::OnMouseLeave(wxMouseEvent& evt)
{
	if (m_enablehover)
	{
		if (m_hoveredblock != -1)
		{
			m_redraw_list.insert(m_hoveredblock);
			m_hoveredblock = -1;
			m_hoveredtile = -1;
			FireBlockEvent(EVT_BLOCK_HOVER, "");
			RefreshStatusbar();
			Refresh();
		}
	}
	evt.Skip();
}

void BlocksetEditorCtrl::OnMouseDown(wxMouseEvent& evt)
{
	int block_idx = ConvertXYToBlockIdx(evt.GetPosition());
	int tile_idx = ConvertXYToTileIdx(evt.GetPosition());
	bool refresh = false;
	switch (m_mode)
	{
	case Mode::BLOCK_SELECT:
		if (m_enablehover && m_hoveredblock != block_idx)
		{
			if (m_hoveredblock != -1)
			{
				m_redraw_list.insert(m_hoveredblock);
			}
			m_hoveredblock = block_idx;
			FireBlockEvent(EVT_BLOCK_HOVER, "");
			refresh = true;
		}
		if (m_enableselection && m_selectedblock != block_idx)
		{
			if (m_selectedblock != -1)
			{
				m_redraw_list.insert(m_selectedblock);
			}
			m_selectedblock = block_idx;
			FireBlockEvent(EVT_BLOCK_SELECT, "");
			refresh = true;
		}
		break;
	case Mode::TILE_SELECT:
		if (m_enablehover && (m_hoveredblock != block_idx || m_hoveredtile != tile_idx))
		{
			if (m_hoveredblock != -1)
			{
				m_redraw_list.insert(m_hoveredblock);
			}
			m_hoveredblock = block_idx;
			m_hoveredtile = tile_idx;
			FireBlockEvent(EVT_BLOCK_HOVER, "");
			refresh = true;
		}
		if (m_enableselection && (m_selectedblock != block_idx || m_selectedtile != tile_idx))
		{
			if (m_selectedblock != -1)
			{
				m_redraw_list.insert(m_selectedblock);
			}
			m_selectedblock = block_idx;
			m_selectedtile = tile_idx;
			FireBlockEvent(EVT_BLOCK_SELECT, "");
			refresh = true;
		}
		break;
	case Mode::PENCIL:
		if (evt.LeftDown())
		{
			SetHoveredTile(m_drawtile);
		}
		else if (evt.RightDown())
		{
			m_drawtile = GetHoveredTile();
			FireEvent(EVT_TILE_SELECT, std::to_string(m_drawtile.GetIndex()));
		}
		break;
	}
	if (refresh)
	{
		RefreshStatusbar();
		Refresh();
	}
	evt.Skip();
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
