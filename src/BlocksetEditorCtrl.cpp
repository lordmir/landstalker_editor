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
EVT_LEFT_DCLICK(BlocksetEditorCtrl::OnDoubleClick)
EVT_MOTION(BlocksetEditorCtrl::OnMouseMove)
EVT_LEAVE_WINDOW(BlocksetEditorCtrl::OnMouseLeave)
wxEND_EVENT_TABLE()

wxDEFINE_EVENT(EVT_BLOCK_SELECT, wxCommandEvent);
wxDEFINE_EVENT(EVT_BLOCK_HOVER, wxCommandEvent);
wxDEFINE_EVENT(EVT_BLOCK_EDIT_REQUEST, wxCommandEvent);
wxDEFINE_EVENT(EVT_BLOCK_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_BLOCK_TILE_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_BLOCK_ACTIVATE, wxCommandEvent);

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
	  m_enableblocknumbers(true),
	  m_enabletilenumbers(false),
	  m_enableborders(true),
	  m_enabletileborders(true),
	  m_enableselection(true),
	  m_enablehover(true),
	  m_enablealpha(false),
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

void BlocksetEditorCtrl::SetColour(int c)
{
}

bool BlocksetEditorCtrl::Save(const wxString& filename)
{
	return false;
}

bool BlocksetEditorCtrl::Open(const wxString& filename)
{
	return false;
}

bool BlocksetEditorCtrl::Open(const std::vector<uint8_t>& data)
{
	return false;
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

bool BlocksetEditorCtrl::New()
{
	return false;
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
	return 0;
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
	m_mode = mode;
}

BlocksetEditorCtrl::Mode BlocksetEditorCtrl::GetMode() const
{
	return m_mode;
}

void BlocksetEditorCtrl::SetDrawTile(const Tile& tile)
{
	m_drawtile = tile;
}

Tile BlocksetEditorCtrl::GetDrawTile()
{
	return m_drawtile;
}

bool BlocksetEditorCtrl::GetTileNumbersEnabled() const
{
	return m_enabletilenumbers;
}

void BlocksetEditorCtrl::SetTileNumbersEnabled(bool enabled)
{
	m_enabletilenumbers = enabled;
}

bool BlocksetEditorCtrl::GetSelectionEnabled() const
{
	return m_enableselection;
}

void BlocksetEditorCtrl::SetSelectionEnabled(bool enabled)
{
	m_enableselection = enabled;
}

bool BlocksetEditorCtrl::GetHoverEnabled() const
{
	return m_enablehover;
}

void BlocksetEditorCtrl::SetHoverEnabled(bool enabled)
{
	m_enablehover = enabled;
}

bool BlocksetEditorCtrl::GetAlphaEnabled() const
{
	return m_enablealpha;
}

void BlocksetEditorCtrl::SetAlphaEnabled(bool enabled)
{
	m_enablealpha = enabled;
}

bool BlocksetEditorCtrl::GetBordersEnabled() const
{
	return m_enableborders;
}

void BlocksetEditorCtrl::SetBordersEnabled(bool enabled)
{
	m_enableborders = enabled;
}

bool BlocksetEditorCtrl::InsertBlock(int row)
{
	return false;
}

bool BlocksetEditorCtrl::DeleteBlock(int row)
{
	return false;
}

bool BlocksetEditorCtrl::IsBlockSelectionValid() const
{
	return m_selectedblock >= 0 && m_selectedblock < static_cast<int>(m_blocks->size());
}

bool BlocksetEditorCtrl::IsBlockHoverValid() const
{
	return m_hoveredblock >= 0 && m_hoveredblock < static_cast<int>(m_blocks->size());
}

bool BlocksetEditorCtrl::IsTileSelectionValid() const
{
	return m_selectedtile >= 0 && m_selectedtile < static_cast<int>(m_blocks->size()) * 4;
}

bool BlocksetEditorCtrl::IsTileHoverValid() const
{
	return m_hoveredblock >= 0 && m_hoveredblock < static_cast<int>(m_blocks->size()) * 4;
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
	if (block >= 0 && block < static_cast<int>(m_blocks->size()))
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
	return Tile();
}

void BlocksetEditorCtrl::SetSelectedBlock(const MapBlock& block)
{
}

void BlocksetEditorCtrl::SetSelectedTile(const Tile& tile)
{
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

void BlocksetEditorCtrl::SetHoveredBlock(const MapBlock& tile)
{
}

Tile BlocksetEditorCtrl::GetHoveredTile() const
{
	return Tile();
}

void BlocksetEditorCtrl::SetHoveredTile(const Tile& tile)
{
}

MapBlock BlocksetEditorCtrl::GetBlockAtPosition(const Position& block) const
{
	int idx = ToBlockIndex(block);
	if (idx >= 0 && idx < static_cast<int>(m_blocks->size()))
	{
		return m_blocks->at(idx);
	}
	return MapBlock();
}

Tile BlocksetEditorCtrl::GetTileAtPosition(const Position& block, const Position& tile) const
{
	return Tile();
}

void BlocksetEditorCtrl::SetBlockAtPosition(const Position& block, const MapBlock& new_block)
{
	int idx = ToBlockIndex(block);
	if (idx >= 0 && idx < static_cast<int>(m_blocks->size()))
	{
		m_blocks->at(idx) = new_block;
	}
}

void BlocksetEditorCtrl::SetTileAtPosition(const Position& block, const Position& tile, const Tile& new_tile)
{
}

bool BlocksetEditorCtrl::IsPositionValid(const Position& tp) const
{
	int idx = ToBlockIndex(tp);
	return (idx >= 0 && idx < static_cast<int>(m_blocks->size()));
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
	case Mode::TILE_SELECT:
		if (IsBlockHoverValid())
		{
			FireUpdateStatusEvent(StrPrintf("Hovered: %04d:%01d", GetBlockHover(), GetTileHover()), 0);
		}
		else
		{
			FireUpdateStatusEvent("", 0);
		}
		if (IsBlockSelectionValid())
		{
			FireUpdateStatusEvent(StrPrintf("Selected: %04d:%01d", GetBlockSelection(), GetTileSelection()), 1);
		}
		else
		{
			FireUpdateStatusEvent("", 1);
		}
	}
}

wxCoord BlocksetEditorCtrl::OnGetRowHeight(size_t row) const
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
		if (m_enabletileborders)
		{
			dc.SetPen(*m_tile_border_pen);
			dc.SetBrush(*wxTRANSPARENT_BRUSH);
			dc.DrawRectangle({ xx, yy, tile_width + 1, tile_height + 1 });
		}
		if (m_enabletilenumbers)
		{
			auto label = (wxString::Format("%03d", tile.GetIndex()));
			auto extent = dc.GetTextExtent(label);
			if ((extent.GetWidth() < tile_width - 2) && (extent.GetHeight() < tile_height - 2))
			{
				dc.DrawText(label, { x * tile_width + 2, y * tile_height + 2 });
			}
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

bool BlocksetEditorCtrl::DrawBlock(wxDC& dc, int x, int y, int idx, const MapBlock& block)
{
	bool retval = true;
	for (std::size_t i = 0; i < MapBlock::GetBlockSize(); ++i)
	{
		const auto pos = ToTilePosition(i);
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
		bool pri = false;
		for (int i = 0; i < static_cast<int>(MapBlock::GetBlockSize()); ++i)
		{
			pri = pri || m_blocks->at(x + y * m_columns).GetTile(i).Attributes().getAttribute(TileAttributes::Attribute::ATTR_PRIORITY);
		}
		if (pri)
		{
			dc.SetPen(*m_priority_pen);
			dc.DrawRectangle({ x * m_cellwidth + 1, y * m_cellheight + 1, m_cellwidth - 2, m_cellheight - 2 });
		}
	}
	if (m_enableblocknumbers)
	{
		auto label = (wxString::Format("%03X", idx));
		auto extent = dc.GetTextExtent(label);
		if ((extent.GetWidth() < GetBlockWidth() - 2) && (extent.GetHeight() < GetBlockHeight() - 2))
		{
			dc.DrawText(label, { x * GetBlockWidth() + 2, y * GetBlockHeight() + 2});
		}
	}
	return retval;
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

BlocksetEditorCtrl::Position BlocksetEditorCtrl::ConvertXYToBlockPos(const wxPoint& point) const
{
	return Position();
}

int BlocksetEditorCtrl::ConvertXYToTileIdx(const wxPoint& point) const
{
	if (m_tileset == nullptr)
	{
		return -1;
	}
	int s = GetVisibleRowsBegin();
	int xx = point.x % (m_pixelsize * m_tileset->GetTileWidth() * MapBlock::GetBlockWidth());
	int yy = s + point.y % (m_pixelsize * m_tileset->GetTileHeight() * MapBlock::GetBlockHeight());
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

BlocksetEditorCtrl::Position BlocksetEditorCtrl::ConvertXYToTilePos(const wxPoint& point) const
{
	return Position();
}

void BlocksetEditorCtrl::SelectBlock(const BlocksetEditorCtrl::Position& tp)
{
	
}

void BlocksetEditorCtrl::SelectTile(const BlocksetEditorCtrl::Position& tp)
{
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
			if (!DrawBlock(m_memdc, pos.x, pos.y, i, m_blocks->at(i)))
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
				if (DrawBlock(m_memdc, pos.x, pos.y, *it, m_blocks->at(*it)))
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
	}
	if (refresh)
	{
		RefreshStatusbar();
		Refresh();
	}
	evt.Skip();
}

void BlocksetEditorCtrl::OnDoubleClick(wxMouseEvent& evt)
{
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
