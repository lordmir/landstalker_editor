#include "SpriteFrameEditorCtrl.h"

#include <wx/wx.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/dcbuffer.h>

#include <fstream>
#include "LZ77.h"
#include "SpriteFrameEditorCtrl.h"
#include "SubspriteControlFrame.h"

wxBEGIN_EVENT_TABLE(SpriteFrameEditorCtrl, wxHVScrolledWindow)
EVT_PAINT(SpriteFrameEditorCtrl::OnPaint)
EVT_SIZE(SpriteFrameEditorCtrl::OnSize)
EVT_LEFT_DOWN(SpriteFrameEditorCtrl::OnMouseDown)
EVT_LEFT_DCLICK(SpriteFrameEditorCtrl::OnDoubleClick)
EVT_MOTION(SpriteFrameEditorCtrl::OnMouseMove)
EVT_LEAVE_WINDOW(SpriteFrameEditorCtrl::OnMouseLeave)
EVT_SET_FOCUS(SpriteFrameEditorCtrl::OnTilesetFocus)
wxEND_EVENT_TABLE()

wxDEFINE_EVENT(EVT_SPRITE_FRAME_SELECT, wxCommandEvent);
wxDEFINE_EVENT(EVT_SPRITE_FRAME_HOVER, wxCommandEvent);
wxDEFINE_EVENT(EVT_SPRITE_FRAME_EDIT_REQUEST, wxCommandEvent);
wxDEFINE_EVENT(EVT_SPRITE_FRAME_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_SPRITE_FRAME_TILE_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_SPRITE_FRAME_ACTIVATE, wxCommandEvent);

static const uint8_t UNKNOWN_TILE[32] = {
	0x11, 0xFF, 0xFF, 0x11, 0x1F, 0xF1, 0x1F, 0xF1, 0x11, 0x11, 0x1F, 0xF1, 0x11, 0x11, 0xFF, 0x11,
	0x11, 0x1F, 0xF1, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1F, 0xF1, 0x11, 0x11, 0x11, 0x11, 0x11 };

SpriteFrameEditorCtrl::SpriteFrameEditorCtrl(wxWindow* parent)
	: wxHVScrolledWindow(parent, wxID_ANY),
	m_pixelsize(8),
	m_selectable(false),
	m_selectedtile(-1),
	m_hoveredtile(-1),
	m_tilebase(0),
	m_columns(MAX_WIDTH),
	m_rows(MAX_HEIGHT),
	m_cellwidth(8),
	m_cellheight(8),
	m_enabletilenumbers(false),
	m_enableborders(true),
	m_enableselection(true),
	m_enablehover(true),
	m_enablealpha(true),
	m_enablesubsprites(true),
	m_enablehitbox(true),
	m_pal(nullptr),
	m_sprite(nullptr),
	m_gd(nullptr),
	m_ctrlwidth(1),
	m_ctrlheight(1),
	m_redraw_all(true),
	m_pendingswap(-1)
{
	m_tiles = std::make_shared<Tileset>();
	m_tiles->InsertTilesBefore(0, MAX_SIZE);

	SetRowColumnCount(m_rows + 1, m_columns + 1);
	SetBackgroundStyle(wxBG_STYLE_PAINT);

	InitialiseBrushesAndPens();
}

SpriteFrameEditorCtrl::~SpriteFrameEditorCtrl()
{
}

bool SpriteFrameEditorCtrl::Save(wxString filename, bool compressed)
{
	return m_sprite->Save(filename.ToStdString(), compressed);
}

bool SpriteFrameEditorCtrl::Open(wxString filename, int sprite_id)
{
	m_sprite_id = sprite_id;
	bool result = m_sprite->Open(filename.ToStdString());
	if (result)
	{
		UpdateTileBuffer();
		ForceRedraw();
	}
	return result;
}

bool SpriteFrameEditorCtrl::Open(std::vector<uint8_t>& pixels, int sprite_id)
{
	m_sprite_id = sprite_id;
	m_sprite->SetBits(pixels);
	UpdateTileBuffer();
	ForceRedraw();
	return true;
}

bool SpriteFrameEditorCtrl::Open(std::shared_ptr<SpriteFrame> frame, std::shared_ptr<Palette> pal, int sprite_id)
{
	m_sprite = frame;
	m_pal = pal;
	m_sprite_id = sprite_id;
	UpdateTileBuffer();
	ForceRedraw();
	return true;
}

void SpriteFrameEditorCtrl::RedrawTiles(int index)
{
	int subsprite = m_selected_subsprite;
	if (subsprite > 0 && subsprite <= static_cast<int>(m_sprite->GetSubSpriteCount()))
	{
		if (subsprite != m_selected_subsprite)
		{
			index = -1;
		}
	}
	else
	{
		m_selected_subsprite = -1;
		if (subsprite != -1)
		{
			index = -1;
		}
	}
	if ((index < 0) || (index >= static_cast<int>(m_sprite->GetTileCount())))
	{
		UpdateAllSpriteTiles();
		ForceRedraw();
	}
	else
	{
		m_redraw_list.insert(index);
		UpdateSpriteTile(index);
		Refresh(false);
	}
}

void SpriteFrameEditorCtrl::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
}

void SpriteFrameEditorCtrl::ClearGameData()
{
	m_gd = nullptr;
}

wxCoord SpriteFrameEditorCtrl::OnGetRowHeight(size_t /*row*/) const
{
	if (m_sprite)
	{
		return wxCoord(m_pixelsize * m_sprite->GetTileHeight());
	}
	else
	{
		return 0;
	}
}

wxCoord SpriteFrameEditorCtrl::OnGetColumnWidth(size_t /*column*/) const
{
	if (m_sprite)
	{
		return wxCoord(m_pixelsize * m_sprite->GetTileWidth());
	}
	else
	{
		return 0;
	}
}

void SpriteFrameEditorCtrl::ClearSelections()
{
}

void SpriteFrameEditorCtrl::MoveSelectionUp()
{
}

void SpriteFrameEditorCtrl::MoveSelectionDown()
{
}

void SpriteFrameEditorCtrl::MoveSelectionLeft()
{
}

void SpriteFrameEditorCtrl::MoveSelectionRight()
{
}

void SpriteFrameEditorCtrl::MoveSubSpriteUp()
{
}

void SpriteFrameEditorCtrl::MoveSubSpriteDown()
{
}

void SpriteFrameEditorCtrl::MoveSubSpriteLeft()
{
}

void SpriteFrameEditorCtrl::MoveSubSpriteRight()
{
}

void SpriteFrameEditorCtrl::ExpandSubSpriteWidth()
{
}

void SpriteFrameEditorCtrl::ContractSubSpriteWidth()
{
}

void SpriteFrameEditorCtrl::ExpandSubSpriteHeight()
{
}

void SpriteFrameEditorCtrl::ContractSubSpriteHeight()
{
}

bool SpriteFrameEditorCtrl::CheckSubSpriteCollisionUp()
{
	return false;
}

bool SpriteFrameEditorCtrl::CheckSubSpriteCollisionDown()
{
	return false;
}

bool SpriteFrameEditorCtrl::CheckSubSpriteCollisionLeft()
{
	return false;
}

bool SpriteFrameEditorCtrl::CheckSubSpriteCollisionRight()
{
	return false;
}

void SpriteFrameEditorCtrl::InsertSubSprite()
{
}

void SpriteFrameEditorCtrl::DeleteSubSprite()
{
}

void SpriteFrameEditorCtrl::IncreaseSubSpritePriority()
{
}

void SpriteFrameEditorCtrl::DecreaseSubSpritePriority()
{
}

void SpriteFrameEditorCtrl::ClearCell()
{
}

void SpriteFrameEditorCtrl::CutCell()
{
}

void SpriteFrameEditorCtrl::CopyCell()
{
}

void SpriteFrameEditorCtrl::PasteCell()
{
}

void SpriteFrameEditorCtrl::SwapCell()
{
}

void SpriteFrameEditorCtrl::OnDraw(wxDC& dc)
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
	if (!m_gd)
	{
		m_memdc.SelectObject(wxNullBitmap);
		return;
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
	if (m_sprite != nullptr)
	{
		if (m_redraw_all == true)
		{
			m_redraw_list.clear();
			for (int y = 0; y < MAX_HEIGHT; ++y)
			{
				for (int x = 0; x < MAX_WIDTH; ++x)
				{
					if (!DrawTileAtPosition(m_memdc, x + y * MAX_WIDTH))
					{
						m_redraw_list.insert(x + y * MAX_WIDTH);
					}
				}
			}
			m_redraw_all = false;
		}
		else
		{
			auto it = m_redraw_list.begin();
			while (it != m_redraw_list.end())
			{
				if ((*it >= 0) && (*it < static_cast<int>(m_tiles->GetTileCount())))
				{
					if (DrawTileAtPosition(m_memdc, *it))
					{
						m_redraw_list.erase(it++);
					}
					else
					{
						++it;
					}
				}
				else
				{
					m_redraw_list.erase(it++);
				}
			}
		}
		DrawSelectionBorders(m_memdc);

		m_memdc.SetBrush(*wxTRANSPARENT_BRUSH);
		if (m_enablesubsprites)
		{
			for (int i = 0; i < static_cast<int>(m_sprite->GetSubSpriteCount()); ++i)
			{
				const auto& s = m_sprite->GetSubSprite(i);
				m_memdc.SetPen(wxPen(i + 1 == m_hovered_subsprite ? wxColor(255, 128, 128) : *wxRED, i + 1 == m_selected_subsprite ? 3 : 1));
				m_memdc.DrawRectangle(SpriteToScreenXY({ s.x, s.y }), { static_cast<int>(s.w * m_sprite->GetTileWidth() * m_pixelsize), static_cast<int>(s.h * m_sprite->GetTileHeight() * m_pixelsize) });
			}
		}
		if (m_enableborders)
		{
			m_memdc.SetPen(wxPen(*wxGREEN, 2));
			m_memdc.DrawLine(SpriteToScreenXY({ -10, 0 }), SpriteToScreenXY({ 10, 0 }));
			m_memdc.DrawLine(SpriteToScreenXY({ 0, -10 }), SpriteToScreenXY({ 0, 10 }));
		}
		if (m_enablehitbox)
		{
			auto hitbox = m_gd->GetSpriteData()->GetSpriteHitbox(m_sprite_id);
			m_memdc.SetPen(wxPen(*wxYELLOW, 1));
			wxPoint hitbox_fg_points[] = {
				SpriteToScreenXY({ hitbox.first * 2, 0}),
				SpriteToScreenXY({ 0, hitbox.first}),
				SpriteToScreenXY({ -hitbox.first * 2, 0}),
				SpriteToScreenXY({ -hitbox.first * 2, -hitbox.second}),
				SpriteToScreenXY({ 0, hitbox.first - hitbox.second}),
				SpriteToScreenXY({ 0, hitbox.first}),
				SpriteToScreenXY({ 0, hitbox.first - hitbox.second}),
				SpriteToScreenXY({ hitbox.first * 2, 0 - hitbox.second}),
				SpriteToScreenXY({ hitbox.first * 2, 0}),
				SpriteToScreenXY({ hitbox.first * 2, 0 - hitbox.second}),
				SpriteToScreenXY({ 0, -hitbox.first - hitbox.second}),
				SpriteToScreenXY({ -hitbox.first * 2, -hitbox.second}),
				SpriteToScreenXY({ -hitbox.first * 2, 0}),
				SpriteToScreenXY({ 0, hitbox.first}),
				SpriteToScreenXY({ hitbox.first * 2, 0}),
			};
			m_memdc.DrawPolygon(sizeof(hitbox_fg_points) / sizeof(hitbox_fg_points[0]), &hitbox_fg_points[0]);
		}
	}

	PaintBitmap(dc);
	m_memdc.SelectObject(wxNullBitmap);
}

void SpriteFrameEditorCtrl::OnPaint(wxPaintEvent& /*evt*/)
{
	wxBufferedPaintDC dc(this);
	this->PrepareDC(dc);
	this->OnDraw(dc);
}

void SpriteFrameEditorCtrl::OnSize(wxSizeEvent& evt)
{
	this->GetClientSize(&m_ctrlwidth, &m_ctrlheight);
	if (UpdateRowCount())
	{
		ForceRedraw();
	}
	wxVarHScrollHelper::HandleOnSize(evt);
	wxVarVScrollHelper::HandleOnSize(evt);
	ScrollToRow(std::max<int>(0U, (m_rows - GetVisibleRowsEnd() + GetVisibleRowsBegin()) / 2 - 4));
	ScrollToColumn(1 + std::max<int>(0U, (m_columns - GetVisibleColumnsEnd() + GetVisibleColumnsBegin()) / 2 - 2));
	Refresh(false);
}

void SpriteFrameEditorCtrl::OnMouseDown(wxMouseEvent& evt)
{
	if (!m_enableselection) return;
	int sel = ConvertXYToTile(evt.GetPosition());
	int selected_subsprite = GetSubspriteAt(sel);
	if (evt.GetModifiers() == 0)
	{
		if (IsTileInSprite(sel))
		{
			SelectTile(sel);
		}
	}
	else if (evt.GetModifiers() & wxMOD_CONTROL)
	{
		if (selected_subsprite != m_selected_subsprite)
		{
			FireEvent(EVT_SUBSPRITE_SELECT, selected_subsprite);
		}
	}
	if (selected_subsprite > 0 && (evt.GetModifiers() & wxMOD_CONTROL))
	{
		SetMouseCursor(wxStockCursor::wxCURSOR_HAND);
	}
	else
	{
		SetMouseCursor(wxStockCursor::wxCURSOR_ARROW);
	}
	evt.Skip();
}

void SpriteFrameEditorCtrl::OnDoubleClick(wxMouseEvent& evt)
{
	int sel = ConvertXYToTile(evt.GetPosition());
	if (sel != -1)
	{
		EditTile(sel);
	}
	evt.Skip();
}

void SpriteFrameEditorCtrl::OnMouseMove(wxMouseEvent& evt)
{
	if (!m_enablehover) return;
	if (m_sprite == nullptr) return;
	int sel = ConvertXYToTile(evt.GetPosition());
	int hovered_tile = (evt.GetModifiers() & wxMOD_CONTROL) ? -1 : sel;
	if ((m_hoveredtile != -1) && (hovered_tile != m_hoveredtile))
	{
		m_redraw_list.insert(m_hoveredtile);
	}
	if (hovered_tile != m_hoveredtile)
	{
		m_hoveredtile = hovered_tile;
		FireEvent(EVT_SPRITE_FRAME_HOVER, std::to_string(m_hoveredtile));
		Refresh();
	}
	int hovered_subsprite = GetSubspriteAt(sel);
	if (hovered_subsprite != m_hovered_subsprite)
	{
		m_hovered_subsprite = hovered_subsprite;
		RedrawTiles();
	}
	if (hovered_subsprite > 0 && (evt.GetModifiers() & wxMOD_CONTROL) > 0)
	{
		SetMouseCursor(wxStockCursor::wxCURSOR_HAND);
	}
	else
	{
		SetMouseCursor(wxStockCursor::wxCURSOR_ARROW);
	}
	evt.Skip();
}

void SpriteFrameEditorCtrl::OnMouseLeave(wxMouseEvent& evt)
{
	SetMouseCursor(wxStockCursor::wxCURSOR_ARROW);
	if (!m_enablehover) return;
	if (m_hoveredtile != -1)
	{
		m_redraw_list.insert(m_hoveredtile);
		m_hoveredtile = -1;
		FireEvent(EVT_SPRITE_FRAME_HOVER, std::to_string(m_hoveredtile));
		Refresh();
	}
	if (m_hovered_subsprite != -1)
	{
		m_hovered_subsprite = -1;
		RedrawTiles();
	}
	evt.Skip();
}

void SpriteFrameEditorCtrl::OnTilesetFocus(wxFocusEvent& evt)
{
	FireEvent(EVT_SPRITE_FRAME_ACTIVATE, "");
	evt.Skip();
}

void SpriteFrameEditorCtrl::HandleKeyDown(int key, int modifiers)
{
	switch (key)
	{
	case WXK_ESCAPE:
		ClearSelections();
		break;
	case WXK_UP:
	case 'w':
	case 'W':
		if (modifiers == 0)
		{
			MoveSelectionUp();
		}
		else if (modifiers == wxMOD_CONTROL)
		{
			MoveSubSpriteUp();
		}
		else if (modifiers == wxMOD_SHIFT)
		{
			ContractSubSpriteHeight();
		}
		break;
	case WXK_DOWN:
	case 's':
	case 'S':
		if (modifiers == 0)
		{
			MoveSelectionDown();
		}
		else if (modifiers == wxMOD_CONTROL)
		{
			MoveSubSpriteDown();
		}
		else if (modifiers == wxMOD_SHIFT)
		{
			ExpandSubSpriteHeight();
		}
		break;
	case WXK_LEFT:
	case 'a':
	case 'A':
		if (modifiers == 0)
		{
			MoveSelectionLeft();
		}
		else if (modifiers == wxMOD_CONTROL)
		{
			MoveSubSpriteLeft();
		}
		else if (modifiers == wxMOD_SHIFT)
		{
			ContractSubSpriteWidth();
		}
		break;
	case WXK_RIGHT:
	case 'd':
	case 'D':
		if (modifiers == 0)
		{
			MoveSelectionRight();
		}
		else if (modifiers == wxMOD_CONTROL)
		{
			MoveSubSpriteRight();
		}
		else if (modifiers == wxMOD_SHIFT)
		{
			ExpandSubSpriteWidth();
		}
		break;
	case WXK_TAB:
		if (modifiers == 0)
		{
			SelectNextSubSprite();
		}
		else if (modifiers == wxMOD_SHIFT)
		{
			SelectPrevSubSprite();
		}
		break;
	case WXK_DELETE:
		if (modifiers == 0)
		{
			ClearCell();
		}
		else if (modifiers == wxMOD_SHIFT)
		{
			DeleteSubSprite();
		}
		break;
	case WXK_INSERT:
		InsertSubSprite();
		break;
	case '[':
	case '{':
		IncreaseSubSpritePriority();
		break;
	case ']':
	case '}':
		DecreaseSubSpritePriority();
		break;
	case 'x':
	case 'X':
		if (modifiers == wxMOD_CONTROL)
		{
			CutCell();
		}
		break;
	case 'c':
	case 'C':
		if (modifiers == wxMOD_CONTROL)
		{
			CopyCell();
		}
		break;
	case 'v':
	case 'V':
		if (modifiers == wxMOD_CONTROL)
		{
			PasteCell();
		}
		break;
	case 'p':
	case 'P':
		if (modifiers == wxMOD_CONTROL)
		{
			SwapCell();
		}
		break;
	default:
		break;
	}
}

int SpriteFrameEditorCtrl::ConvertXYToTile(const wxPoint& point)
{
	wxPoint c = point;
	c.x += GetVisibleColumnsBegin() * m_tiles->GetTileWidth() * m_pixelsize;
	c.y += GetVisibleRowsBegin() * m_tiles->GetTileHeight() * m_pixelsize;
	auto p = ScreenToSpriteXY(c);
	p.x += ORIGIN_X * m_tiles->GetTileWidth();
	p.y += ORIGIN_Y * m_tiles->GetTileHeight();
	p.x /= static_cast<int>(m_tiles->GetTileWidth());
	p.y /= static_cast<int>(m_tiles->GetTileHeight());
	if(p.x >= 0 && p.x < MAX_WIDTH && p.y >= 0 && p.y < MAX_HEIGHT)
	{
		return p.y * MAX_WIDTH + p.x;
	}
	return -1;
}

wxPoint SpriteFrameEditorCtrl::ConvertTileToXY(int tile) const
{
	if (tile >= 0 && tile < MAX_SIZE)
	{

		return { tile % MAX_WIDTH, tile / MAX_WIDTH };
	}
	return { -1, -1 };
}

wxPoint SpriteFrameEditorCtrl::SpriteToScreenXY(wxPoint sprite)
{
	return wxPoint((sprite.x + 0x80) * m_pixelsize, (sprite.y + 0x80) * m_pixelsize);
}

wxPoint SpriteFrameEditorCtrl::ScreenToSpriteXY(wxPoint screen)
{
	return wxPoint(screen.x / m_pixelsize - 0x80, screen.y / m_pixelsize - 0x80 );
}

bool SpriteFrameEditorCtrl::IsTileInSprite(int tile) const
{
	int x = tile % MAX_WIDTH - ORIGIN_X;
	int y = tile / MAX_WIDTH - ORIGIN_Y;
	for (const auto& s : m_sprite->GetSubSprites())
	{
		int sxb = s.x / static_cast<int>(m_tiles->GetTileWidth());
		int syb = s.y / static_cast<int>(m_tiles->GetTileHeight());
		int sxe = sxb + static_cast<int>(s.w);
		int sye = syb + static_cast<int>(s.h);
		if (x >= sxb && x < sxe && y >= syb && y < sye)
		{
			return true;
		}
	}
	return false;
}

int SpriteFrameEditorCtrl::GetSpriteTileNum(int tile)
{
	int tx = tile % MAX_WIDTH - ORIGIN_X;
	int ty = tile / MAX_WIDTH - ORIGIN_Y;
	int tc = 0;
	for (const auto& s : m_sprite->GetSubSprites())
	{
		int sxb = s.x / static_cast<int>(m_tiles->GetTileWidth());
		int syb = s.y / static_cast<int>(m_tiles->GetTileHeight());
		int sxe = sxb + static_cast<int>(s.w);
		int sye = syb + static_cast<int>(s.h);
		for (int x = sxb; x < sxe; ++x)
		{
			for (int y = syb; y < sye; ++y)
			{
				if (tx == x && ty == y)
				{
					return tc;
				}
				++tc;
			}
		}
	}
	return -1;
}

void SpriteFrameEditorCtrl::UpdateTileBuffer()
{
	int cur_tile = 0;
	m_tiles->Reset();
	for (const auto& s : m_sprite->GetSubSprites())
	{
		int sxb = s.x / static_cast<int>(m_tiles->GetTileWidth()) + ORIGIN_X;
		int syb = s.y / static_cast<int>(m_tiles->GetTileHeight()) + ORIGIN_Y;
		int sxe = sxb + static_cast<int>(s.w);
		int sye = syb + static_cast<int>(s.h);
		for (int x = sxb; x < sxe; ++x)
		{
			for (int y = syb; y < sye; ++y)
			{
				m_tiles->SetTile(y * MAX_WIDTH + x, m_sprite->GetTileset()->GetTile(cur_tile++));
			}
		}
	}
}

void SpriteFrameEditorCtrl::UpdateSpriteTile(int tile)
{
	int cur_tile = 0;
	for (const auto& s : m_sprite->GetSubSprites())
	{
		int sxb = s.x / static_cast<int>(m_tiles->GetTileWidth()) + ORIGIN_X;
		int syb = s.y / static_cast<int>(m_tiles->GetTileHeight()) + ORIGIN_Y;
		int sxe = sxb + static_cast<int>(s.w);
		int sye = syb + static_cast<int>(s.h);
		for (int x = sxb; x < sxe; ++x)
		{
			for (int y = syb; y < sye; ++y)
			{
				int tpos = y * MAX_WIDTH + x;
				if (tile == tpos)
				{
					m_sprite->GetTileset()->SetTile(cur_tile, m_tiles->GetTile(tpos));
					return;
				}
				++cur_tile;
			}
		}
	}
}

void SpriteFrameEditorCtrl::UpdateAllSpriteTiles()
{
	int cur_tile = 0;
	for (const auto& s : m_sprite->GetSubSprites())
	{
		int sxb = s.x / static_cast<int>(m_tiles->GetTileWidth()) + ORIGIN_X;
		int syb = s.y / static_cast<int>(m_tiles->GetTileHeight()) + ORIGIN_Y;
		int sxe = sxb + static_cast<int>(s.w);
		int sye = syb + static_cast<int>(s.h);
		for (int x = sxb; x < sxe; ++x)
		{
			for (int y = syb; y < sye; ++y)
			{
				int tpos = y * MAX_WIDTH + x;
				m_sprite->GetTileset()->SetTile(cur_tile++, m_tiles->GetTile(tpos));
			}
		}
	}
}

bool SpriteFrameEditorCtrl::UpdateRowCount()
{
	if (m_sprite == nullptr)
	{
		return false;
	}
	m_cellwidth = m_pixelsize * m_sprite->GetTileWidth();
	m_cellheight = m_pixelsize * m_sprite->GetTileHeight();
	return false;
}

void SpriteFrameEditorCtrl::DrawTile(wxDC& dc, int x, int y, int tile)
{
	wxPen pen = dc.GetPen();
	wxBrush brush = dc.GetBrush();

	pen.SetStyle(wxPENSTYLE_TRANSPARENT);
	brush.SetStyle(wxBRUSHSTYLE_SOLID);
	dc.SetPen(pen);

	auto tile_bytes = m_tiles->GetTile(tile);
	const auto& pal = GetSelectedPalette();
	std::vector<uint32_t> tile_pixels;
	for (const auto& b : tile_bytes)
	{
		tile_pixels.push_back(pal.getBGRA(b));
	}

	for (int i = 0; i < static_cast<int>(tile_pixels.size()); ++i)
	{
		int xx = x + (i % m_tiles->GetTileWidth()) * m_pixelsize;
		int yy = y + (i / m_tiles->GetTileWidth()) * m_pixelsize;
		if (IsTileInSprite(tile))
		{
			brush.SetColour(wxColour(tile_pixels[i]));
		}
		else
		{
			brush.SetColour(wxColour(tile_pixels[i]).ChangeLightness(50));
		}
		dc.SetBrush(brush);
		// Has alpha
		if ((tile_pixels[i] & 0xFF000000) > 0)
		{
			dc.DrawRectangle({ xx, yy, m_pixelsize, m_pixelsize });
		}
	}

}

bool SpriteFrameEditorCtrl::DrawTileAtPosition(wxDC& dc, int pos)
{
	bool retval = false;
	int sx = GetVisibleColumnsBegin();
	int ex = GetVisibleColumnsEnd();
	int sy = GetVisibleRowsBegin();
	int ey = GetVisibleRowsEnd();

	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	//auto p = m_sprite->GetTilePosition(pos);
	//auto ss = SpriteToScreenXY({ p.first, p.second });
	auto x = pos % MAX_WIDTH;
	auto y = pos / MAX_WIDTH;
	if ((y >= sy) && (y <= ey) && (x >= sx) && (x <= ex))
	{
		dc.SetPen(*wxTRANSPARENT_PEN);
		if (m_enablealpha)
		{
			dc.SetBrush(IsTileInSprite(pos) ? *m_alpha_brush : *m_dark_alpha_brush);
		}
		else
		{
			dc.SetBrush(IsTileInSprite(pos) ? *wxBLACK_BRUSH : wxBrush(wxColor(64, 64, 64)));
		}
		dc.DrawRectangle({ x * m_cellwidth, y * m_cellheight, m_cellwidth, m_cellheight });
		DrawTile(dc, x * m_cellwidth, y * m_cellheight, pos);
		if (m_enableborders)
		{
			dc.SetPen(*m_border_pen);
			dc.SetBrush(*wxTRANSPARENT_BRUSH);
			dc.DrawRectangle({ x * m_cellwidth, y * m_cellheight, m_cellwidth + 1, m_cellheight + 1 });
		}
		if (m_enabletilenumbers)
		{
			auto label = (wxString::Format("%03d", pos));
			auto extent = dc.GetTextExtent(label);
			if ((extent.GetWidth() < m_cellwidth - 2) && (extent.GetHeight() < m_cellheight - 2))
			{
				dc.DrawText(label, { x * m_cellwidth + 2, y * m_cellheight + 2 });
			}
		}
		retval = true;
	}
	return retval;
}

void SpriteFrameEditorCtrl::DrawSelectionBorders(wxDC& dc)
{
	if (m_hoveredtile != -1)
	{
		auto p = ConvertTileToXY(m_hoveredtile);
		p.x *= m_tiles->GetTileWidth() * m_pixelsize;
		p.y *= m_tiles->GetTileHeight() * m_pixelsize;
		dc.SetBrush(*m_highlighted_brush);
		dc.SetPen(IsTileInSprite(m_hoveredtile) ? *m_highlighted_border_pen : *wxGREY_PEN);
		if (m_hoveredtile == m_selectedtile)
		{
			dc.DrawRectangle({ p.x + 1, p.y + 1, m_cellwidth - 2, m_cellheight - 2 });
		}
		else
		{
			dc.DrawRectangle({ p.x, p.y, m_cellwidth, m_cellheight });
		}
	}
	if (IsSelectionValid())
	{
		auto p = ConvertTileToXY(m_selectedtile);
		p.x *= m_tiles->GetTileWidth() * m_pixelsize;
		p.y *= m_tiles->GetTileHeight() * m_pixelsize;
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.SetPen(*m_selected_border_pen);
		dc.DrawRectangle({ p.x, p.y, m_cellwidth, m_cellheight });
	}
}

void SpriteFrameEditorCtrl::PaintBitmap(wxDC& dc)
{
	dc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE)));
	dc.Clear();
	int sx = GetVisibleColumnsBegin() * m_sprite->GetTileWidth() * m_pixelsize;
	int sy = GetVisibleRowsBegin() * m_sprite->GetTileHeight() * m_pixelsize;

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
		dc.Blit(vX + sx, vY + sy, vW, vH, &m_memdc, vX + sx, vY + sy);
		upd++;
	}
}

void SpriteFrameEditorCtrl::InitialiseBrushesAndPens()
{
	m_alpha_brush = std::make_unique<wxBrush>();
	m_dark_alpha_brush = std::make_unique<wxBrush>();
	m_stipple = std::make_unique<wxBitmap>(6, 6);
	m_dark_stipple = std::make_unique<wxBitmap>(6, 6);
	std::unique_ptr<wxMemoryDC> imagememDC(new wxMemoryDC());
	imagememDC->SelectObject(*m_stipple);
	imagememDC->SetBackground(*wxGREY_BRUSH);
	imagememDC->Clear();
	imagememDC->SetBrush(*wxLIGHT_GREY_BRUSH);
	imagememDC->SetPen(*wxTRANSPARENT_PEN);
	imagememDC->DrawRectangle(0, 0, 3, 3);
	imagememDC->DrawRectangle(3, 3, 5, 5);
	imagememDC->SelectObject(*m_dark_stipple);
	imagememDC->SetBackground(wxColor(32, 32, 32));
	imagememDC->Clear();
	imagememDC->SetBrush(wxColor(64, 64, 64));
	imagememDC->SetPen(*wxTRANSPARENT_PEN);
	imagememDC->DrawRectangle(0, 0, 3, 3);
	imagememDC->DrawRectangle(3, 3, 5, 5);
	imagememDC->SelectObject(wxNullBitmap);
	m_alpha_brush->SetStyle(wxBRUSHSTYLE_STIPPLE_MASK);
	m_alpha_brush->SetStipple(*m_stipple);
	m_dark_alpha_brush->SetStyle(wxBRUSHSTYLE_STIPPLE_MASK);
	m_dark_alpha_brush->SetStipple(*m_dark_stipple);
	m_border_pen = std::make_unique<wxPen>(*wxMEDIUM_GREY_PEN);
	m_selected_border_pen = std::make_unique<wxPen>(wxColour(255,0,255));
	m_highlighted_border_pen = std::make_unique<wxPen>(*wxBLUE_PEN);
	m_highlighted_brush = std::make_unique<wxBrush>(*wxTRANSPARENT_BRUSH);
}

void SpriteFrameEditorCtrl::ForceRedraw()
{
	m_redraw_all = true;
	wxVarVScrollHelper::RefreshAll();
	Refresh();
}

const Palette& SpriteFrameEditorCtrl::GetSelectedPalette()
{
	return *m_pal;
}

void SpriteFrameEditorCtrl::SetMouseCursor(wxStockCursor cursor)
{
	if (cursor != m_cursor)
	{
		SetCursor(cursor);
		m_cursor = cursor;
	}
}

void SpriteFrameEditorCtrl::SetPixelSize(int n)
{
	m_pixelsize = n;
	UpdateRowCount();
	ForceRedraw();
}

int SpriteFrameEditorCtrl::GetPixelSize() const
{
	return m_pixelsize;
}

int SpriteFrameEditorCtrl::GetTilemapSize() const
{
	return m_sprite->GetTileCount();
}

bool SpriteFrameEditorCtrl::GetCompressed() const
{
	return m_sprite->GetCompressed();
}

void SpriteFrameEditorCtrl::SetActivePalette(const std::shared_ptr<Palette> pal)
{
	m_pal = pal;
	ForceRedraw();
}

void SpriteFrameEditorCtrl::SelectSubSprite(int sel)
{
	if (sel > 0 && sel <= static_cast<int>(m_sprite->GetSubSpriteCount()))
	{
		m_selected_subsprite = sel;
	}
	else
	{
		m_selected_subsprite = -1;
	}
	m_redraw_all = true;
	Refresh();
}

int SpriteFrameEditorCtrl::GetSelectedSubSprite() const
{
	return m_selected_subsprite;
}

int SpriteFrameEditorCtrl::GetHoveredSubSprite() const
{
	return m_hovered_subsprite;
}

void SpriteFrameEditorCtrl::ClearSubSpriteSelection()
{
	m_selected_subsprite = -1;
	m_redraw_all = true;
	Refresh();
}

std::shared_ptr<Tileset> SpriteFrameEditorCtrl::GetTileset()
{
	return m_tiles;
}

bool SpriteFrameEditorCtrl::GetTileNumbersEnabled() const
{
	return m_enabletilenumbers;
}

void SpriteFrameEditorCtrl::SetTileNumbersEnabled(bool enabled)
{
	if (enabled != m_enabletilenumbers)
	{
		m_enabletilenumbers = enabled;
		ForceRedraw();
	}
}

bool SpriteFrameEditorCtrl::GetSelectionEnabled() const
{
	return m_enableselection;
}

void SpriteFrameEditorCtrl::SetSelectionEnabled(bool enabled)
{
	if (m_enableselection != enabled)
	{
		m_redraw_list.insert(m_selectedtile);
		if (enabled == false)
		{
			m_selectedtile = -1;
		}
		m_enableselection = enabled;
		Refresh(false);
	}
}

bool SpriteFrameEditorCtrl::GetHoverEnabled() const
{
	return m_enablehover;
}

void SpriteFrameEditorCtrl::SetHoverEnabled(bool enabled)
{
	if (m_enablehover != enabled)
	{
		m_redraw_list.insert(m_selectedtile);
		if (enabled == false)
		{
			m_hoveredtile = -1;
		}
		m_enablehover = enabled;
		Refresh(false);
	}
}

bool SpriteFrameEditorCtrl::GetAlphaEnabled() const
{
	return m_enablealpha;
}

void SpriteFrameEditorCtrl::SetAlphaEnabled(bool enabled)
{
	if (m_enablealpha != enabled)
	{
		m_enablealpha = enabled;
		ForceRedraw();
	}
}

bool SpriteFrameEditorCtrl::GetBordersEnabled() const
{
	return m_enableborders;
}

void SpriteFrameEditorCtrl::SetBordersEnabled(bool enabled)
{
	if (m_enableborders != enabled)
	{
		m_enableborders = enabled;
		ForceRedraw();
	}
}

std::pair<int, int> SpriteFrameEditorCtrl::GetTilePosition(int tile) const
{
	if (tile != -1)
	{
		return { tile % MAX_WIDTH - ORIGIN_X, tile / MAX_WIDTH - ORIGIN_Y };
	}
	else
	{
		return { -1, -1 };
	}
}

bool SpriteFrameEditorCtrl::IsSelectionValid() const
{
	return ((m_selectedtile != -1) && (m_selectedtile < static_cast<int>(m_tiles->GetTileCount()))) && IsTileInSprite(m_selectedtile);
}

Tile SpriteFrameEditorCtrl::GetSelectedTile() const
{
	return Tile(m_selectedtile);
}

std::pair<int, int> SpriteFrameEditorCtrl::GetSelectedTilePosition() const
{
	return GetTilePosition(m_selectedtile);
}

bool SpriteFrameEditorCtrl::IsHoverValid() const
{
	return m_hoveredtile != -1 && (m_hoveredtile < static_cast<int>(m_tiles->GetTileCount())) && IsTileInSprite(m_hoveredtile);
}

Tile SpriteFrameEditorCtrl::GetHoveredTile() const
{
	return Tile(m_hoveredtile);
}

std::pair<int, int> SpriteFrameEditorCtrl::GetHoveredTilePosition() const
{
	return GetTilePosition(m_hoveredtile);
}

int SpriteFrameEditorCtrl::GetFirstTile() const
{
	if (!m_sprite || m_sprite->GetSubSpriteCount() < 1)
	{
		return 0;
	}
	else
	{
		const auto& s = m_sprite->GetSubSprite(0);
		int x = s.x / static_cast<int>(m_tiles->GetTileWidth()) + ORIGIN_X;
		int y = s.y / static_cast<int>(m_tiles->GetTileHeight()) + ORIGIN_Y;
		return x + y * MAX_WIDTH;
	}
}

int SpriteFrameEditorCtrl::GetSubspriteAt(int tile) const
{
	for (int i = 0; i < static_cast<int>(m_sprite->GetSubSpriteCount()); ++i)
	{
		const auto& s = m_sprite->GetSubSprite(i);
		int sxb = s.x / static_cast<int>(m_tiles->GetTileWidth()) + ORIGIN_X;
		int syb = s.y / static_cast<int>(m_tiles->GetTileHeight()) + ORIGIN_Y;
		int sxe = sxb + static_cast<int>(s.w);
		int sye = syb + static_cast<int>(s.h);
		for (int x = sxb; x < sxe; ++x)
		{
			for (int y = syb; y < sye; ++y)
			{
				int tpos = y * MAX_WIDTH + x;
				if (tile == tpos)
				{
					return i + 1;
				}
			}
		}
	}
	return -1;
}

void SpriteFrameEditorCtrl::SelectTile(int tile)
{
	if ((m_selectedtile != -1) && (tile != m_selectedtile))
	{
		if (m_pendingswap)
		{
			m_redraw_list.insert(tile);
			m_sprite->GetTileset()->SwapTile(m_pendingswap, tile);
			m_pendingswap = -1;
		}
		m_redraw_list.insert(m_selectedtile);
	}
	if (tile != m_selectedtile)
	{
		FireEvent(EVT_SPRITE_FRAME_SELECT, std::to_string(tile));
		m_selectedtile = tile;
		Refresh();
	}
}

void SpriteFrameEditorCtrl::InsertTileBefore(const Tile& tile)
{
	if (tile.GetIndex() <= m_sprite->GetTileCount())
	{
		m_sprite->GetTileset()->InsertTilesBefore(tile.GetIndex());
		for (int i = tile.GetIndex(); i < static_cast<int>(m_sprite->GetTileCount()); ++i)
		{
			m_redraw_list.insert(i);
		}
		SelectTile(m_selectedtile + 1);
		UpdateRowCount();
		Refresh();
		FireEvent(EVT_SPRITE_FRAME_CHANGE, std::to_string(tile.GetTileValue()));
	}
}

void SpriteFrameEditorCtrl::InsertTileAfter(const Tile& tile)
{
	if (tile.GetIndex() <= m_sprite->GetTileCount())
	{
		m_sprite->GetTileset()->InsertTilesBefore(tile.GetIndex() + 1);
		for (int i = tile.GetIndex(); i < static_cast<int>(m_sprite->GetTileCount()); ++i)
		{
			m_redraw_list.insert(i);
		}
		UpdateRowCount();
		Refresh();
		FireEvent(EVT_SPRITE_FRAME_CHANGE, std::to_string(tile.GetTileValue()));
	}
}

void SpriteFrameEditorCtrl::InsertTileAtEnd()
{
	m_sprite->GetTileset()->InsertTilesBefore(m_sprite->GetTileCount());
	m_redraw_list.insert(m_sprite->GetTileCount() - 1);
	UpdateRowCount();
	Refresh();
	FireEvent(EVT_SPRITE_FRAME_CHANGE, "");
}

void SpriteFrameEditorCtrl::DeleteTileAt(const Tile& tile)
{
	if (tile.GetIndex() <= m_sprite->GetTileCount())
	{
		m_sprite->GetTileset()->DeleteTile(tile.GetIndex());
		if (!IsSelectionValid())
		{
			SelectTile(m_sprite->GetTileCount() - 1);
		}
		ForceRedraw();
		FireEvent(EVT_SPRITE_FRAME_CHANGE, std::to_string(tile.GetTileValue()));
	}
}

void SpriteFrameEditorCtrl::CutTile(const Tile& tile)
{
	if (tile.GetIndex() <= m_sprite->GetTileCount())
	{
		m_clipboard = m_sprite->GetTile(tile);
		DeleteTileAt(tile);
	}
}

void SpriteFrameEditorCtrl::CopyTile(const Tile& tile) const
{
	if (tile.GetIndex() <= m_sprite->GetTileCount())
	{
		m_clipboard = m_sprite->GetTile(tile);
	}
}

void SpriteFrameEditorCtrl::PasteTile(const Tile& tile)
{
	if ((tile.GetIndex() <= m_sprite->GetTileCount()) && !IsClipboardEmpty())
	{
		m_sprite->GetTilePixels(tile.GetTileValue()) = m_clipboard;
		m_redraw_list.insert(tile.GetTileValue());
		Refresh();
		FireEvent(EVT_SPRITE_FRAME_CHANGE, std::to_string(tile.GetTileValue()));
	}
}

void SpriteFrameEditorCtrl::SwapTile(const Tile& tile)
{
	if (tile.GetIndex() <= m_sprite->GetTileCount())
	{
		m_pendingswap = tile.GetIndex();
	}
}

void SpriteFrameEditorCtrl::EditTile(const Tile& tile)
{
	if (tile.GetIndex() < m_sprite->GetTileCount())
	{
		FireEvent(EVT_SPRITE_FRAME_EDIT_REQUEST, std::to_string(tile.GetIndex()));
	}
}

bool SpriteFrameEditorCtrl::IsClipboardEmpty() const
{
	return m_clipboard.empty();
}


void SpriteFrameEditorCtrl::FireEvent(const wxEventType& e, const std::string& data)
{
	wxCommandEvent evt(e);
	evt.SetString(data);
	evt.SetClientData(&m_sprite);
	wxPostEvent(this->GetParent(), evt);
}

void SpriteFrameEditorCtrl::FireEvent(const wxEventType& e, int data)
{
	wxCommandEvent evt(e);
	evt.SetInt(data);
	evt.SetExtraLong(data);
	evt.SetClientData(&m_sprite);
	wxPostEvent(this->GetParent(), evt);
}
