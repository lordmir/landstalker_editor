#include "BlocksetEditorCtrl.h"

#include <wx/wx.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/dcbuffer.h>

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

BlocksetEditorCtrl::BlocksetEditorCtrl(wxWindow* parent)
	: wxVScrolledWindow(parent, wxID_ANY),
	m_mode(Mode::BLOCK_SELECT),
	m_columns(0),
	m_rows(0),
	m_cellwidth(0),
	m_cellheight(0),
	m_ctrlwidth(0),
	m_ctrlheight(0),
	m_pixelsize(4),
	m_selectable(false),
	m_selectedblock(-1),
	m_hoveredblock(-1),
	m_selectedtile(-1),
	m_hoveredtile(-1),
	block_width(2),
	block_height(2),
	m_enableblocknumbers(true),
	m_enabletilenumbers(false),
	m_enableborders(true),
	m_enableselection(true),
	m_enablehover(true),
	m_enablealpha(false),
	m_redraw_all(true),
	m_drawtile(0)
{
	SetRowCount(m_rows);
	SetBackgroundStyle(wxBG_STYLE_PAINT);

	InitialiseBrushesAndPens();
}

BlocksetEditorCtrl::~BlocksetEditorCtrl()
{
	delete m_border_pen;
	delete m_tile_border_pen;
	delete m_selected_border_pen;
	delete m_highlighted_border_pen;
	delete m_highlighted_brush;
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

bool BlocksetEditorCtrl::New()
{
	return false;
}

void BlocksetEditorCtrl::RedrawTiles(int index)
{
	if ((index < 0) || (index >= m_tileset->GetTileCount()))
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
	if ((index < 0) || (index >= m_blocks->size()))
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
	return std::shared_ptr<Tileset>();
}

std::shared_ptr<Palette> BlocksetEditorCtrl::GetPalette()
{
	return std::shared_ptr<Palette>();
}

std::shared_ptr<std::vector<MapBlock>> BlocksetEditorCtrl::GetBlocks()
{
	return std::shared_ptr<std::vector<MapBlock>>();
}

void BlocksetEditorCtrl::SetMode(const Mode& mode)
{
}

BlocksetEditorCtrl::Mode BlocksetEditorCtrl::GetMode() const
{
	return Mode();
}

void BlocksetEditorCtrl::SetDrawTile(const Tile& tile)
{
}

Tile BlocksetEditorCtrl::GetDrawTile()
{
	return Tile();
}

bool BlocksetEditorCtrl::GetTileNumbersEnabled() const
{
	return false;
}

void BlocksetEditorCtrl::SetTileNumbersEnabled(bool enabled)
{
}

bool BlocksetEditorCtrl::GetSelectionEnabled() const
{
	return false;
}

void BlocksetEditorCtrl::SetSelectionEnabled(bool enabled)
{
}

bool BlocksetEditorCtrl::GetHoverEnabled() const
{
	return false;
}

void BlocksetEditorCtrl::SetHoverEnabled(bool enabled)
{
}

bool BlocksetEditorCtrl::GetAlphaEnabled() const
{
	return false;
}

void BlocksetEditorCtrl::SetAlphaEnabled(bool enabled)
{
}

bool BlocksetEditorCtrl::GetBordersEnabled() const
{
	return false;
}

void BlocksetEditorCtrl::SetBordersEnabled(bool enabled)
{
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
	return false;
}

bool BlocksetEditorCtrl::IsBlockHoverValid() const
{
	return false;
}

bool BlocksetEditorCtrl::IsTileSelectionValid() const
{
	return false;
}

bool BlocksetEditorCtrl::IsTileHoverValid() const
{
	return false;
}

BlocksetEditorCtrl::Position BlocksetEditorCtrl::GetBlockSelection() const
{
	return Position();
}

BlocksetEditorCtrl::Position BlocksetEditorCtrl::GetBlockHover() const
{
	return Position();
}

BlocksetEditorCtrl::Position BlocksetEditorCtrl::GetTileSelection() const
{
	return Position();
}

BlocksetEditorCtrl::Position BlocksetEditorCtrl::GetTileHover() const
{
	return Position();
}

MapBlock BlocksetEditorCtrl::GetSelectedBlock() const
{
	return MapBlock();
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
	return MapBlock();
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
	return MapBlock();
}

Tile BlocksetEditorCtrl::GetTileAtPosition(const Position& block, const Position& tile) const
{
	return Tile();
}

void BlocksetEditorCtrl::SetBlockAtPosition(const Position& block, const MapBlock& new_block)
{
}

void BlocksetEditorCtrl::SetTileAtPosition(const Position& block, const Position& tile, const Tile& new_tile)
{
}

bool BlocksetEditorCtrl::IsPositionValid(const Position& tp) const
{
	return false;
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
	m_cellwidth = m_pixelsize * m_tileset->GetTileWidth() * GetBlockWidth();
	m_cellheight = m_pixelsize * m_tileset->GetTileHeight() * GetBlockHeight();
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
	const int xcell = x / MapBlock::GetBlockWidth();
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

	for (int i = 0; i < tile_pixels.size(); ++i)
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
	for (int i = 0; i < MapBlock::GetBlockSize(); ++i)
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
	}
	return retval;
}

void BlocksetEditorCtrl::DrawSelectionBorders(wxDC& dc)
{
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
	m_alpha_brush = new wxBrush();
	wxBitmap* stipple = new wxBitmap(6, 6);
	wxMemoryDC* imagememDC = new wxMemoryDC();
	imagememDC->SelectObject(*stipple);
	imagememDC->SetBackground(*wxGREY_BRUSH);
	imagememDC->Clear();
	imagememDC->SetBrush(*wxLIGHT_GREY_BRUSH);
	imagememDC->SetPen(*wxTRANSPARENT_PEN);
	imagememDC->DrawRectangle(0, 0, 3, 3);
	imagememDC->DrawRectangle(3, 3, 5, 5);
	imagememDC->SelectObject(wxNullBitmap);
	m_alpha_brush->SetStyle(wxBRUSHSTYLE_STIPPLE_MASK);
	m_alpha_brush->SetStipple(*stipple);
	delete stipple;
	delete imagememDC;
	m_border_pen = new wxPen(*wxMEDIUM_GREY_PEN);
	m_tile_border_pen = new wxPen(wxColour(65, 65, 65));
	m_selected_border_pen = new wxPen(*wxRED_PEN);
	m_highlighted_border_pen = new wxPen(*wxBLUE_PEN);
	m_highlighted_brush = new wxBrush(*wxTRANSPARENT_BRUSH);
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
	int idx = tp.x + tp.y * m_columns;
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
	bidx += (tp.x % MapBlock::GetBlockWidth());
	bidx += (tp.y % MapBlock::GetBlockHeight()) * MapBlock::GetBlockWidth();
	return bidx;
}

int BlocksetEditorCtrl::ConvertXYToBlockIdx(const wxPoint& point) const
{
	return 0;
}

BlocksetEditorCtrl::Position BlocksetEditorCtrl::ConvertXYToBlockPos(const wxPoint& point) const
{
	return Position();
}

int BlocksetEditorCtrl::ConvertXYToTileIdx(const wxPoint& point) const
{
	return 0;
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

	if (m_pixelsize > 3)
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
		for (int i = 0; i < m_blocks->size(); ++i)
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
			if ((*it >= 0) && (*it < m_blocks->size()))
			{
				auto pos = ToBlockPosition(*it);
				if (DrawBlock(m_memdc, pos.x, pos.y, m_blocks->at(*it)))
				{
					m_redraw_list.erase(it++);
				}
				else
				{
					++it;
				}
			}
		}
	}

	DrawSelectionBorders(m_memdc);
	PaintBitmap(dc);
	m_memdc.SelectObject(wxNullBitmap);
}

void BlocksetEditorCtrl::OnPaint(wxPaintEvent& evt)
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
}

void BlocksetEditorCtrl::OnMouseLeave(wxMouseEvent& evt)
{
}

void BlocksetEditorCtrl::OnMouseDown(wxMouseEvent& evt)
{
}

void BlocksetEditorCtrl::OnDoubleClick(wxMouseEvent& evt)
{
}

void BlocksetEditorCtrl::FireEvent(const wxEventType& e, const std::string& data)
{
}

void BlocksetEditorCtrl::FireTilesetEvent(const wxEventType& e, const std::string& data)
{
}
