#include <sprites/SpriteFrameEditorCtrl.h>

#include <algorithm>
#include <fstream>
#include <wx/wx.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/dcbuffer.h>
#include <wx/rawbmp.h>

#include <landstalker/misc/LZ77.h>
#include <sprites/SubspriteControlFrame.h>

wxBEGIN_EVENT_TABLE(SpriteFrameEditorCtrl, wxHVScrolledWindow)
EVT_PAINT(SpriteFrameEditorCtrl::OnPaint)
EVT_SIZE(SpriteFrameEditorCtrl::OnSize)
EVT_LEFT_DOWN(SpriteFrameEditorCtrl::OnMouseDown)
EVT_LEFT_UP(SpriteFrameEditorCtrl::OnMouseUp)
EVT_RIGHT_DOWN(SpriteFrameEditorCtrl::OnRightDown)
EVT_RIGHT_UP(SpriteFrameEditorCtrl::OnMouseUp)
EVT_LEFT_DCLICK(SpriteFrameEditorCtrl::OnDoubleClick)
EVT_MOTION(SpriteFrameEditorCtrl::OnMouseMove)
EVT_LEAVE_WINDOW(SpriteFrameEditorCtrl::OnMouseLeave)
EVT_ENTER_WINDOW(SpriteFrameEditorCtrl::OnMouseEnter)
EVT_MOUSE_CAPTURE_LOST(SpriteFrameEditorCtrl::OnCaptureLost)
EVT_SET_FOCUS(SpriteFrameEditorCtrl::OnTilesetFocus)
wxEND_EVENT_TABLE()

wxDEFINE_EVENT(EVT_SPRITE_FRAME_SELECT, wxCommandEvent);
wxDEFINE_EVENT(EVT_SPRITE_FRAME_HOVER, wxCommandEvent);
wxDEFINE_EVENT(EVT_SPRITE_FRAME_EDIT_REQUEST, wxCommandEvent);
wxDEFINE_EVENT(EVT_SPRITE_FRAME_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_SPRITE_FRAME_TILE_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_SPRITE_FRAME_ACTIVATE, wxCommandEvent);
wxDEFINE_EVENT(EVT_SPRITE_FRAME_COLOUR_PICK, wxCommandEvent);

static const uint8_t UNKNOWN_TILE[32] = {
	0x11, 0xFF, 0xFF, 0x11, 0x1F, 0xF1, 0x1F, 0xF1, 0x11, 0x11, 0x1F, 0xF1, 0x11, 0x11, 0xFF, 0x11,
	0x11, 0x1F, 0xF1, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1F, 0xF1, 0x11, 0x11, 0x11, 0x11, 0x11 };

SpriteFrameEditorCtrl::SpriteFrameEditorCtrl(wxWindow* parent)
	: wxHVScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxWANTS_CHARS),
	m_pixelsize(8),
	m_selectedtile(-1),
	m_hoveredtile(-1),
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
	m_pendingswap(-1)
{
	m_tiles = std::make_shared<Landstalker::Tileset>();
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
		ClearHistory();
		UpdateTileBuffer();
		ForceRedraw();
	}
	return result;
}

bool SpriteFrameEditorCtrl::Open(std::vector<uint8_t>& pixels, int sprite_id)
{
	m_sprite_id = sprite_id;
	m_sprite->SetBits(pixels);
	ClearHistory();
	UpdateTileBuffer();
	ForceRedraw();
	return true;
}

bool SpriteFrameEditorCtrl::Open(std::shared_ptr<Landstalker::SpriteFrame> frame, std::shared_ptr<Landstalker::Palette> pal, int sprite_id)
{
	m_sprite = frame;
	m_pal = pal;
	m_sprite_id = sprite_id;
	ClearHistory();
	UpdateTileBuffer();
	ForceRedraw();
	return true;
}

void SpriteFrameEditorCtrl::RedrawTiles(int index)
{
	// A stale subsprite selection (e.g. after deletions) invalidates the layout, so clear
	// it and fall back to a full redraw.
	if ((m_selected_subsprite != -1) &&
	    ((m_selected_subsprite <= 0) || (m_selected_subsprite > static_cast<int>(m_sprite->GetSubSpriteCount()))))
	{
		m_selected_subsprite = -1;
		index = -1;
	}
	if ((index < 0) || (index >= static_cast<int>(m_sprite->GetTileCount())))
	{
		UpdateAllSpriteTiles();
		ForceRedraw();
	}
	else
	{
		// The tile's pixels changed: queue a per-tile patch of the cached bitmap. A full
		// re-render here ran once per painted pixel and made drawing crawl.
		m_redraw_list.insert(index);
		UpdateSpriteTile(index);
		RefreshTileRect(index);
	}
}

void SpriteFrameEditorCtrl::UpdateSubSprites()
{
	UpdateAllSpriteTiles();
	if (m_selected_subsprite > static_cast<int>(m_sprite->GetSubSpriteCount()))
	{
		SelectSubSprite(-1);
	}
	RedrawTiles();
}

void SpriteFrameEditorCtrl::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
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
	SelectSubSprite(-1);
	FireEvent(EVT_SUBSPRITE_SELECT, -1);
}

void SpriteFrameEditorCtrl::MoveSelectionUp()
{
	if (m_selectedtile == -1)
	{
		return;
	}
	int x = m_selectedtile % MAX_WIDTH;
	int y = m_selectedtile / MAX_WIDTH;
	do
	{
		--y;
	} while (y >= 0 && !IsTileInSprite(x + y * MAX_WIDTH));
	if (y >= 0)
	{
		SelectTile(x + y * MAX_WIDTH);
	}
}

void SpriteFrameEditorCtrl::MoveSelectionDown()
{
	if (m_selectedtile == -1)
	{
		return;
	}
	int x = m_selectedtile % MAX_WIDTH;
	int y = m_selectedtile / MAX_WIDTH;
	do
	{
		++y;
	} while (y <= MAX_HEIGHT && !IsTileInSprite(x + y * MAX_WIDTH));
	if (y < MAX_HEIGHT)
	{
		SelectTile(x + y * MAX_WIDTH);
	}
}

void SpriteFrameEditorCtrl::MoveSelectionLeft()
{
	if (m_selectedtile == -1)
	{
		return;
	}
	int x = m_selectedtile % MAX_WIDTH;
	int y = m_selectedtile / MAX_WIDTH;
	do
	{
		--x;
	} while (x >= 0 && !IsTileInSprite(x + y * MAX_WIDTH));
	if(x >= 0)
	{
		SelectTile(x + y * MAX_WIDTH);
	}
}

void SpriteFrameEditorCtrl::MoveSelectionRight()
{
	if (m_selectedtile == -1)
	{
		return;
	}
	int x = m_selectedtile % MAX_WIDTH;
	int y = m_selectedtile / MAX_WIDTH;
	do
	{
		++x;
	} while (x < MAX_WIDTH && !IsTileInSprite(x + y * MAX_WIDTH));
	if (x < MAX_WIDTH)
	{
		SelectTile(x + y * MAX_WIDTH);
	}
}

void SpriteFrameEditorCtrl::MoveSubSpriteUp()
{
	if (GetSelectedSubSprite() <= 0)
	{
		return;
	}
	auto& orig_s = m_sprite->GetSubSprite(GetSelectedSubSprite() - 1);
	Landstalker::SpriteFrame::SubSprite new_s = orig_s;
	new_s.y -= static_cast<int>(m_tiles->GetTileHeight());

	if (new_s.y >= -ORIGIN_Y * static_cast<int>(m_tiles->GetTileHeight()))
	{
		if (!CheckSubSpriteCollision(new_s, GetSelectedSubSprite() - 1))
		{
			PushUndo();
			orig_s = new_s;
			FireEvent(EVT_SUBSPRITE_UPDATE);
		}
	}
}

void SpriteFrameEditorCtrl::MoveSubSpriteDown()
{
	if (GetSelectedSubSprite() <= 0)
	{
		return;
	}
	auto& orig_s = m_sprite->GetSubSprite(GetSelectedSubSprite() - 1);
	Landstalker::SpriteFrame::SubSprite new_s = orig_s;
	new_s.y += static_cast<int>(m_tiles->GetTileHeight());

	if (new_s.y < ORIGIN_Y * static_cast<int>(m_tiles->GetTileHeight()))
	{
		if (!CheckSubSpriteCollision(new_s, GetSelectedSubSprite() - 1))
		{
			PushUndo();
			orig_s = new_s;
			FireEvent(EVT_SUBSPRITE_UPDATE);
		}
	}
}

void SpriteFrameEditorCtrl::MoveSubSpriteLeft()
{
	if (GetSelectedSubSprite() <= 0)
	{
		return;
	}
	auto& orig_s = m_sprite->GetSubSprite(GetSelectedSubSprite() - 1);
	Landstalker::SpriteFrame::SubSprite new_s = orig_s;
	new_s.x -= static_cast<int>(m_tiles->GetTileWidth());

	if (new_s.x >= -ORIGIN_X * static_cast<int>(m_tiles->GetTileWidth()))
	{
		if (!CheckSubSpriteCollision(new_s, GetSelectedSubSprite() - 1))
		{
			PushUndo();
			orig_s = new_s;
			FireEvent(EVT_SUBSPRITE_UPDATE);
		}
	}
}

void SpriteFrameEditorCtrl::MoveSubSpriteRight()
{
	if (GetSelectedSubSprite() <= 0)
	{
		return;
	}
	auto& orig_s = m_sprite->GetSubSprite(GetSelectedSubSprite() - 1);
	Landstalker::SpriteFrame::SubSprite new_s = orig_s;
	new_s.x += static_cast<int>(m_tiles->GetTileWidth());

	if (new_s.x < ORIGIN_X * static_cast<int>(m_tiles->GetTileWidth()))
	{
		if (!CheckSubSpriteCollision(new_s, GetSelectedSubSprite() - 1))
		{
			PushUndo();
			orig_s = new_s;
			FireEvent(EVT_SUBSPRITE_UPDATE);
		}
	}
}

void SpriteFrameEditorCtrl::ExpandSubSpriteWidth()
{
	if (GetSelectedSubSprite() <= 0)
	{
		return;
	}
	auto& orig_s = m_sprite->GetSubSprite(GetSelectedSubSprite() - 1);
	if (orig_s.w > 3)
	{
		return;
	}
	Landstalker::SpriteFrame::SubSprite new_s = orig_s;
	++new_s.w;
	if (!CheckSubSpriteCollision(new_s, GetSelectedSubSprite() - 1))
	{
		PushUndo();
		orig_s = new_s;
		FireEvent(EVT_SUBSPRITE_UPDATE);
	}
}

void SpriteFrameEditorCtrl::ContractSubSpriteWidth()
{
	if (GetSelectedSubSprite() <= 0)
	{
		return;
	}
	auto& orig_s = m_sprite->GetSubSprite(GetSelectedSubSprite() - 1);
	if (orig_s.w < 2)
	{
		return;
	}
	Landstalker::SpriteFrame::SubSprite new_s = orig_s;
	--new_s.w;
	if (!CheckSubSpriteCollision(new_s, GetSelectedSubSprite() - 1))
	{
		PushUndo();
		orig_s = new_s;
		FireEvent(EVT_SUBSPRITE_UPDATE);
	}
}

void SpriteFrameEditorCtrl::ExpandSubSpriteHeight()
{
	if (GetSelectedSubSprite() <= 0)
	{
		return;
	}
	auto& orig_s = m_sprite->GetSubSprite(GetSelectedSubSprite() - 1);
	if (orig_s.h > 3)
	{
		return;
	}
	Landstalker::SpriteFrame::SubSprite new_s = orig_s;
	++new_s.h;
	if (!CheckSubSpriteCollision(new_s, GetSelectedSubSprite() - 1))
	{
		PushUndo();
		orig_s = new_s;
		FireEvent(EVT_SUBSPRITE_UPDATE);
	}
}

void SpriteFrameEditorCtrl::ContractSubSpriteHeight()
{
	if (GetSelectedSubSprite() <= 0)
	{
		return;
	}
	auto& orig_s = m_sprite->GetSubSprite(GetSelectedSubSprite() - 1);
	if (orig_s.h < 2)
	{
		return;
	}
	Landstalker::SpriteFrame::SubSprite new_s = orig_s;
	--new_s.h;
	if (!CheckSubSpriteCollision(new_s, GetSelectedSubSprite() - 1))
	{
		PushUndo();
		orig_s = new_s;
		FireEvent(EVT_SUBSPRITE_UPDATE);
	}
}

bool SpriteFrameEditorCtrl::CheckSubSpriteCollision(const Landstalker::SpriteFrame::SubSprite& s, int index)
{
	for (int i = 0; i < static_cast<int>(m_sprite->GetSubSpriteCount()); ++i)
	{
		if (index == i)
		{
			continue;
		}
		if (m_sprite->GetSubSprite(i).Collides(s))
		{
			return true;
		}
	}
	return false;
}

void SpriteFrameEditorCtrl::InsertSubSprite()
{
	FireEvent(EVT_SUBSPRITE_ADD, m_selected_subsprite);
}

void SpriteFrameEditorCtrl::DeleteSubSprite()
{
	FireEvent(EVT_SUBSPRITE_DELETE, m_selected_subsprite);
}

void SpriteFrameEditorCtrl::IncreaseSubSpritePriority()
{
	FireEvent(EVT_SUBSPRITE_MOVE_UP, m_selected_subsprite);
}

void SpriteFrameEditorCtrl::DecreaseSubSpritePriority()
{
	FireEvent(EVT_SUBSPRITE_MOVE_DOWN, m_selected_subsprite);
}

void SpriteFrameEditorCtrl::SelectNextSubSprite()
{
	if (m_selected_subsprite < static_cast<int>(m_sprite->GetSubSpriteCount()) && m_selected_subsprite > 0)
	{
		FireEvent(EVT_SUBSPRITE_SELECT, m_selected_subsprite + 1);
	}
	else
	{
		FireEvent(EVT_SUBSPRITE_SELECT, 1);
	}
}

void SpriteFrameEditorCtrl::SelectPrevSubSprite()
{
	if (m_selected_subsprite > 1)
	{
		FireEvent(EVT_SUBSPRITE_SELECT, m_selected_subsprite - 1);
	}
	else
	{
		FireEvent(EVT_SUBSPRITE_SELECT, m_sprite->GetSubSpriteCount());
	}
}

void SpriteFrameEditorCtrl::ClearCell()
{
	const auto blank = Landstalker::ByteVector(m_tiles->GetTileWidth() * m_tiles->GetTileHeight());
	if (m_tiles->GetTile(GetSelectedTile()) == blank)
	{
		return;
	}
	PushUndo();
	m_tiles->SetTile(GetSelectedTile(), blank);
	FireEvent(EVT_SPRITE_FRAME_CHANGE, std::to_string(GetSelectedTile().GetIndex()));
	RedrawTiles(GetSelectedTile().GetIndex());
}

void SpriteFrameEditorCtrl::CutCell()
{
	CopyCell();
	ClearCell();
}

void SpriteFrameEditorCtrl::CopyCell()
{
	m_clipboard = m_tiles->GetTile(GetSelectedTile());
}

void SpriteFrameEditorCtrl::PasteCell()
{
	if (m_clipboard.size() == m_tiles->GetTileWidth() * m_tiles->GetTileHeight())
	{
		if (m_tiles->GetTile(GetSelectedTile()) == m_clipboard)
		{
			return;
		}
		PushUndo();
		m_tiles->SetTile(GetSelectedTile(), m_clipboard);
		FireEvent(EVT_SPRITE_FRAME_CHANGE, std::to_string(GetSelectedTile().GetIndex()));
		RedrawTiles(GetSelectedTile().GetIndex());
	}
}

void SpriteFrameEditorCtrl::SwapCell()
{
	if (m_pendingswap == -1)
	{
		m_pendingswap = m_selectedtile;
	}
	else
	{
		if (m_pendingswap != m_selectedtile)
		{
			PushUndo();
			m_swapbuffer = m_tiles->GetTilePixels(m_pendingswap);
			m_tiles->SetTile(m_pendingswap, m_tiles->GetTilePixels(m_selectedtile));
			m_tiles->SetTile(m_selectedtile, m_swapbuffer);
			FireEvent(EVT_SPRITE_FRAME_CHANGE, std::to_string(m_pendingswap));
			FireEvent(EVT_SPRITE_FRAME_CHANGE, std::to_string(GetSelectedTile().GetIndex()));
		}
		m_pendingswap = -1;
	}
}

bool SpriteFrameEditorCtrl::CanUndo() const
{
	return !m_undo_stack.empty();
}

bool SpriteFrameEditorCtrl::CanRedo() const
{
	return !m_redo_stack.empty();
}

void SpriteFrameEditorCtrl::Undo()
{
	if (!m_sprite || !m_tiles || m_undo_stack.empty())
	{
		return;
	}
	m_redo_stack.push_back(MakeUndoState());
	auto state = std::move(m_undo_stack.back());
	m_undo_stack.pop_back();
	RestoreUndoState(state);
}

void SpriteFrameEditorCtrl::Redo()
{
	if (!m_sprite || !m_tiles || m_redo_stack.empty())
	{
		return;
	}
	m_undo_stack.push_back(MakeUndoState());
	auto state = std::move(m_redo_stack.back());
	m_redo_stack.pop_back();
	RestoreUndoState(state);
}

void SpriteFrameEditorCtrl::PushUndo()
{
	if (!m_sprite || !m_tiles)
	{
		return;
	}
	PushUndoState(MakeUndoState());
}

void SpriteFrameEditorCtrl::PushUndoState(UndoState&& state)
{
	// A new edit invalidates anything that was undone.
	m_redo_stack.clear();
	m_undo_stack.push_back(std::move(state));
	while (m_undo_stack.size() > 100)
	{
		m_undo_stack.pop_front();
	}
}

void SpriteFrameEditorCtrl::ClearHistory()
{
	m_undo_stack.clear();
	m_redo_stack.clear();
	// Called when a frame is (re)opened: any selection or float belongs to the old frame.
	m_sel_rect = wxRect();
	m_sel_drag = SelDrag::None;
	m_sel_floating = false;
	m_sel_from_paste = false;
	m_sel_snapshot_valid = false;
	m_sel_op_changed = false;
	m_float_bmp.reset();
	m_float_data.clear();
	// The pixel clipboard survives on purpose, so content can be pasted across frames.
}

SpriteFrameEditorCtrl::UndoState SpriteFrameEditorCtrl::MakeUndoState() const
{
	return { m_tiles->GetBits(false), m_sprite->GetSubSprites() };
}

void SpriteFrameEditorCtrl::RestoreUndoState(const UndoState& state)
{
	// Any selection or pending float refers to canvas content that is about to change.
	m_sel_rect = wxRect();
	m_sel_floating = false;
	m_sel_from_paste = false;
	m_float_bmp.reset();
	m_float_data.clear();
	m_pending_sync.clear();
	m_tiles->SetBits(state.tiles, false);
	m_sprite->SetSubSprites(state.subsprites);
	// The sprite's own tileset is always derived from the canvas and the subsprite layout.
	UpdateAllSpriteTiles();
	// A restored layout can have fewer subsprites, invalidating the selection.
	if (m_selected_subsprite > static_cast<int>(m_sprite->GetSubSpriteCount()))
	{
		m_selected_subsprite = -1;
	}
	m_pendingswap = -1;
	ForceRedraw();
}

void SpriteFrameEditorCtrl::SetMode(Mode mode)
{
	if (m_mode == mode)
	{
		return;
	}
	EndSubSpriteDrag();
	CancelShape();
	EndStroke();
	if (m_sel_drag != SelDrag::None)
	{
		CancelSelectionDrag();
	}
	// Switching tool/mode confirms a pending paste and drops the selection.
	ClearPixelSelection(true);
	m_drawing = false;
	m_hoveredpixel = { -1, -1 };
	m_mode = mode;
	SetMouseCursor(wxStockCursor::wxCURSOR_ARROW);
	Refresh();
}

SpriteFrameEditorCtrl::Mode SpriteFrameEditorCtrl::GetMode() const
{
	return m_mode;
}

void SpriteFrameEditorCtrl::SetDrawTool(Tool tool)
{
	if (m_tool != tool)
	{
		CancelShape();
		if (m_sel_drag != SelDrag::None)
		{
			CancelSelectionDrag();
		}
		// Switching tool confirms a pending paste and drops the selection.
		ClearPixelSelection(true);
		m_tool = tool;
	}
}

SpriteFrameEditorCtrl::Tool SpriteFrameEditorCtrl::GetDrawTool() const
{
	return m_tool;
}

void SpriteFrameEditorCtrl::SetPrimaryColour(uint8_t colour)
{
	if (colour < 16)
	{
		m_primary_colour = colour;
	}
}

uint8_t SpriteFrameEditorCtrl::GetPrimaryColour() const
{
	return m_primary_colour;
}

void SpriteFrameEditorCtrl::SetSecondaryColour(uint8_t colour)
{
	if (colour < 16)
	{
		m_secondary_colour = colour;
	}
}

uint8_t SpriteFrameEditorCtrl::GetSecondaryColour() const
{
	return m_secondary_colour;
}

bool SpriteFrameEditorCtrl::IsPixelHoverValid() const
{
	return (m_hoveredpixel.x >= 0) && (m_hoveredpixel.y >= 0);
}

wxPoint SpriteFrameEditorCtrl::GetHoveredPixel() const
{
	return m_hoveredpixel;
}

int SpriteFrameEditorCtrl::GetColourAtPixel(const wxPoint& pixel) const
{
	if ((m_tiles == nullptr) || (pixel.x < 0) || (pixel.y < 0))
	{
		return -1;
	}
	const int tw = static_cast<int>(m_tiles->GetTileWidth());
	const int th = static_cast<int>(m_tiles->GetTileHeight());
	const int tile = (pixel.x / tw) + (pixel.y / th) * MAX_WIDTH;
	if ((tile >= static_cast<int>(m_tiles->GetTileCount())) || !IsTileInSprite(tile))
	{
		return -1;
	}
	const auto& pixels = m_tiles->GetTilePixels(tile);
	const std::size_t idx = (pixel.x % tw) + (pixel.y % th) * tw;
	return (idx < pixels.size()) ? pixels[idx] : -1;
}

wxPoint SpriteFrameEditorCtrl::MouseToLogical(const wxPoint& point) const
{
	return { point.x + static_cast<int>(GetVisibleColumnsBegin()) * m_cellwidth,
	         point.y + static_cast<int>(GetVisibleRowsBegin()) * m_cellheight };
}

std::vector<SpriteFrameEditorCtrl::SubSpriteHandle> SpriteFrameEditorCtrl::GetSubSpriteHandles()
{
	std::vector<SubSpriteHandle> handles;
	if ((m_mode != Mode::SUBSPRITE) || (m_sprite == nullptr) || (m_selected_subsprite <= 0) ||
	    (m_selected_subsprite > static_cast<int>(m_sprite->GetSubSpriteCount())))
	{
		return handles;
	}
	const auto& s = m_sprite->GetSubSprite(m_selected_subsprite - 1);
	const int tw = static_cast<int>(m_tiles->GetTileWidth());
	const int th = static_cast<int>(m_tiles->GetTileHeight());
	const wxPoint o = SpriteToScreenXY({ s.x, s.y });
	const int w = static_cast<int>(s.w) * tw * m_pixelsize;
	const int h = static_cast<int>(s.h) * th * m_pixelsize;
	handles = {
		{ o.x,         o.y,         EDGE_LEFT | EDGE_TOP },
		{ o.x + w / 2, o.y,         EDGE_TOP },
		{ o.x + w,     o.y,         EDGE_RIGHT | EDGE_TOP },
		{ o.x,         o.y + h / 2, EDGE_LEFT },
		{ o.x + w,     o.y + h / 2, EDGE_RIGHT },
		{ o.x,         o.y + h,     EDGE_LEFT | EDGE_BOTTOM },
		{ o.x + w / 2, o.y + h,     EDGE_BOTTOM },
		{ o.x + w,     o.y + h,     EDGE_RIGHT | EDGE_BOTTOM },
	};
	return handles;
}

bool SpriteFrameEditorCtrl::HitTestSubSpriteHandles(const wxPoint& logical, int& edges)
{
	// Generous hit area so the handles stay usable at low zoom, where a 1x1 subsprite
	// is only 8 pixels across.
	const int hit = std::max(4, m_pixelsize / 2 + 2);
	for (const auto& h : GetSubSpriteHandles())
	{
		if ((std::abs(logical.x - h.x) <= hit) && (std::abs(logical.y - h.y) <= hit))
		{
			edges = h.edges;
			return true;
		}
	}
	return false;
}

void SpriteFrameEditorCtrl::UpdateSubSpriteCursor(const wxPoint& logical)
{
	int edges = 0;
	if (HitTestSubSpriteHandles(logical, edges))
	{
		const bool l = (edges & EDGE_LEFT) != 0;
		const bool r = (edges & EDGE_RIGHT) != 0;
		const bool t = (edges & EDGE_TOP) != 0;
		const bool b = (edges & EDGE_BOTTOM) != 0;
		if ((l && t) || (r && b))
		{
			SetMouseCursor(wxStockCursor::wxCURSOR_SIZENWSE);
		}
		else if ((r && t) || (l && b))
		{
			SetMouseCursor(wxStockCursor::wxCURSOR_SIZENESW);
		}
		else if (l || r)
		{
			SetMouseCursor(wxStockCursor::wxCURSOR_SIZEWE);
		}
		else
		{
			SetMouseCursor(wxStockCursor::wxCURSOR_SIZENS);
		}
	}
	else if (m_hovered_subsprite > 0)
	{
		SetMouseCursor(wxStockCursor::wxCURSOR_SIZING);
	}
	else
	{
		SetMouseCursor(wxStockCursor::wxCURSOR_ARROW);
	}
}

void SpriteFrameEditorCtrl::DoSubSpriteDrag(const wxPoint& logical)
{
	if ((m_drag_subsprite <= 0) || (m_drag_subsprite > static_cast<int>(m_sprite->GetSubSpriteCount())))
	{
		return;
	}
	const int tw = static_cast<int>(m_tiles->GetTileWidth());
	const int th = static_cast<int>(m_tiles->GetTileHeight());
	auto& s = m_sprite->GetSubSprite(m_drag_subsprite - 1);
	// Snapped to the tile grid: the canvas<->sprite tile mapping assumes tile alignment.
	const int tx = std::clamp(logical.x / m_cellwidth - m_drag_offset.x, 0, MAX_WIDTH - static_cast<int>(s.w));
	const int ty = std::clamp(logical.y / m_cellheight - m_drag_offset.y, 0, MAX_HEIGHT - static_cast<int>(s.h));
	// Walk one tile at a time towards the target, so a drag into another subsprite clamps
	// against its edge (and slides along it) instead of ignoring the move altogether.
	int cx = s.x / tw + ORIGIN_X;
	int cy = s.y / th + ORIGIN_Y;
	Landstalker::SpriteFrame::SubSprite cand = s;
	bool progress = true;
	while (progress && ((cx != tx) || (cy != ty)))
	{
		progress = false;
		if (cx != tx)
		{
			const int step = (cx < tx) ? 1 : -1;
			cand.x = (cx + step - ORIGIN_X) * tw;
			cand.y = (cy - ORIGIN_Y) * th;
			if (!CheckSubSpriteCollision(cand, m_drag_subsprite - 1))
			{
				cx += step;
				progress = true;
			}
		}
		if (cy != ty)
		{
			const int step = (cy < ty) ? 1 : -1;
			cand.x = (cx - ORIGIN_X) * tw;
			cand.y = (cy + step - ORIGIN_Y) * th;
			if (!CheckSubSpriteCollision(cand, m_drag_subsprite - 1))
			{
				cy += step;
				progress = true;
			}
		}
	}
	Landstalker::SpriteFrame::SubSprite new_s = s;
	new_s.x = (cx - ORIGIN_X) * tw;
	new_s.y = (cy - ORIGIN_Y) * th;
	if (new_s == s)
	{
		return;
	}
	if (!m_drag_undo_pushed)
	{
		PushUndo();
		m_drag_undo_pushed = true;
	}
	s = new_s;
	FireEvent(EVT_SUBSPRITE_UPDATE);
}

void SpriteFrameEditorCtrl::DoSubSpriteResize(const wxPoint& logical)
{
	if ((m_drag_subsprite <= 0) || (m_drag_subsprite > static_cast<int>(m_sprite->GetSubSpriteCount())))
	{
		return;
	}
	const int tw = static_cast<int>(m_tiles->GetTileWidth());
	const int th = static_cast<int>(m_tiles->GetTileHeight());
	auto& s = m_sprite->GetSubSprite(m_drag_subsprite - 1);
	const int left = s.x / tw + ORIGIN_X;
	const int top = s.y / th + ORIGIN_Y;
	const int right = left + static_cast<int>(s.w);
	const int bottom = top + static_cast<int>(s.h);
	const int mx = logical.x / m_cellwidth;
	const int my = logical.y / m_cellheight;
	Landstalker::SpriteFrame::SubSprite new_s = s;
	// The dragged edge follows the mouse tile; the opposite edge stays put. Width and
	// height are hardware-limited to 1-4 tiles. Growth steps one tile at a time so an
	// edge dragged into a neighbouring subsprite clamps against it; shrinking can never
	// collide, so it jumps straight to the target.
	const auto fits = [&](const Landstalker::SpriteFrame::SubSprite& cand)
	{
		return !CheckSubSpriteCollision(cand, m_drag_subsprite - 1);
	};
	if (m_drag_edges & EDGE_RIGHT)
	{
		const int target = std::clamp(mx + 1, left + 1, std::min(left + 4, MAX_WIDTH));
		int r = std::min(right, target);
		while (r < target)
		{
			auto cand = new_s;
			cand.w = static_cast<std::size_t>(r + 1 - left);
			if (!fits(cand)) break;
			++r;
		}
		new_s.w = static_cast<std::size_t>(r - left);
	}
	if (m_drag_edges & EDGE_LEFT)
	{
		const int target = std::clamp(mx, std::max(right - 4, 0), right - 1);
		int l = std::max(left, target);
		while (l > target)
		{
			auto cand = new_s;
			cand.x = (l - 1 - ORIGIN_X) * tw;
			cand.w = static_cast<std::size_t>(right - (l - 1));
			if (!fits(cand)) break;
			--l;
		}
		new_s.x = (l - ORIGIN_X) * tw;
		new_s.w = static_cast<std::size_t>(right - l);
	}
	if (m_drag_edges & EDGE_BOTTOM)
	{
		const int target = std::clamp(my + 1, top + 1, std::min(top + 4, MAX_HEIGHT));
		int b = std::min(bottom, target);
		while (b < target)
		{
			auto cand = new_s;
			cand.h = static_cast<std::size_t>(b + 1 - top);
			if (!fits(cand)) break;
			++b;
		}
		new_s.h = static_cast<std::size_t>(b - top);
	}
	if (m_drag_edges & EDGE_TOP)
	{
		const int target = std::clamp(my, std::max(bottom - 4, 0), bottom - 1);
		int t = std::max(top, target);
		while (t > target)
		{
			auto cand = new_s;
			cand.y = (t - 1 - ORIGIN_Y) * th;
			cand.h = static_cast<std::size_t>(bottom - (t - 1));
			if (!fits(cand)) break;
			--t;
		}
		new_s.y = (t - ORIGIN_Y) * th;
		new_s.h = static_cast<std::size_t>(bottom - t);
	}
	if (new_s == s)
	{
		return;
	}
	if (CheckSubSpriteCollision(new_s, m_drag_subsprite - 1))
	{
		return;
	}
	if (!m_drag_undo_pushed)
	{
		PushUndo();
		m_drag_undo_pushed = true;
	}
	s = new_s;
	FireEvent(EVT_SUBSPRITE_UPDATE);
}

void SpriteFrameEditorCtrl::DrawSubSpriteHandles(wxDC& dc)
{
	const auto handles = GetSubSpriteHandles();
	if (handles.empty())
	{
		return;
	}
	const int side = std::clamp(m_pixelsize, 6, 10);
	dc.SetPen(*wxBLACK_PEN);
	dc.SetBrush(*wxWHITE_BRUSH);
	for (const auto& h : handles)
	{
		dc.DrawRectangle(h.x - side / 2, h.y - side / 2, side, side);
	}
}

void SpriteFrameEditorCtrl::FlushSpriteTileSync()
{
	for (const int tile : m_pending_sync)
	{
		UpdateSpriteTile(tile);
	}
	m_pending_sync.clear();
}

void SpriteFrameEditorCtrl::StartDrawAction(const wxPoint& logical)
{
	if ((m_tiles == nullptr) || (m_pixelsize <= 0))
	{
		return;
	}
	const int gx = logical.x / m_pixelsize;
	const int gy = logical.y / m_pixelsize;
	switch (m_tool)
	{
	case Tool::Pencil:
		MouseDrawMove(logical);
		break;
	case Tool::Fill:
		FloodFillAt(gx, gy, m_secondary_active ? m_secondary_colour : m_primary_colour);
		break;
	case Tool::Picker:
		PickColourAt(gx, gy, m_secondary_active);
		break;
	case Tool::PixelSelect:
		if (!m_secondary_active)
		{
			BeginSelectionAction(gx, gy);
		}
		break;
	default:
		// Shape tools: anchor here, preview while dragging, commit on release.
		m_shape_active = true;
		m_shape_secondary = m_secondary_active;
		m_shape_start = wxPoint(gx, gy);
		m_shape_end = m_shape_start;
		RefreshRect(GlobalPixelBoxToClient(m_shape_start, m_shape_end));
		break;
	}
}

void SpriteFrameEditorCtrl::MouseDrawMove(const wxPoint& logical)
{
	if ((m_tiles == nullptr) || (m_pixelsize <= 0))
	{
		return;
	}
	const int tw = static_cast<int>(m_tiles->GetTileWidth());
	const int th = static_cast<int>(m_tiles->GetTileHeight());
	const int gx = logical.x / m_pixelsize;
	const int gy = logical.y / m_pixelsize;

	// Hovered tile and pixel; the pixel cursor only shows over drawable (in-sprite) tiles.
	int tile = -1;
	wxPoint pixel(-1, -1);
	if ((gx >= 0) && (gy >= 0) && (gx < MAX_WIDTH * tw) && (gy < MAX_HEIGHT * th))
	{
		tile = (gx / tw) + (gy / th) * MAX_WIDTH;
		if (IsTileInSprite(tile))
		{
			pixel = wxPoint(gx, gy);
		}
	}
	const int old_tile = m_hoveredtile;
	if (tile != old_tile)
	{
		m_hoveredtile = tile;
		FireEvent(EVT_SPRITE_FRAME_HOVER, std::to_string(m_hoveredtile));
		RefreshTileRect(old_tile);
		RefreshTileRect(tile);
	}
	if (pixel != m_hoveredpixel)
	{
		if (IsPixelHoverValid())
		{
			RefreshGlobalPixel(m_hoveredpixel);
		}
		m_hoveredpixel = pixel;
		if (pixel.x >= 0)
		{
			RefreshGlobalPixel(pixel);
		}
		if (tile == old_tile)
		{
			// The status bar shows the pixel coordinates; the tile branch above already
			// fired for cross-tile moves.
			FireEvent(EVT_SPRITE_FRAME_HOVER, std::to_string(m_hoveredtile));
		}
	}
	if (m_drawing && (m_tool == Tool::Picker))
	{
		// Dragging with the picker keeps sampling, like holding an eyedropper.
		PickColourAt(gx, gy, m_secondary_active);
	}
	if ((m_tool == Tool::PixelSelect) && (m_sel_drag != SelDrag::None))
	{
		UpdateSelectionDrag(gx, gy);
	}
	if (m_drawing && m_shape_active)
	{
		// Shape drag in progress: track the end point and repaint old and new extents.
		if (m_shape_end != wxPoint(gx, gy))
		{
			const wxPoint old_end = m_shape_end;
			m_shape_end = wxPoint(gx, gy);
			RefreshRect(GlobalPixelBoxToClient(m_shape_start, old_end));
			RefreshRect(GlobalPixelBoxToClient(m_shape_start, m_shape_end));
		}
	}
	if (m_drawing && (m_tool == Tool::Pencil))
	{
		if (!m_stroke_dirty)
		{
			// Provisional snapshot: pushed to the undo stack only once this stroke
			// actually changes a pixel.
			m_stroke_snapshot = MakeUndoState();
		}
		const uint8_t colour = m_secondary_active ? m_secondary_colour : m_primary_colour;
		bool changed = false;
		wxRect damage;
		if (m_last_drawn.x < 0)
		{
			changed = PaintGlobalPixel(gx, gy, colour, damage);
		}
		else
		{
			// Joined to the previous sample so fast strokes don't leave gaps.
			PlotShapeLine(m_last_drawn, wxPoint(gx, gy), [&](int x, int y)
				{
					changed |= PaintGlobalPixel(x, y, colour, damage);
				});
		}
		m_last_drawn = wxPoint(gx, gy);
		if (changed)
		{
			if (!m_stroke_dirty)
			{
				PushUndoState(std::move(m_stroke_snapshot));
			}
			m_stroke_dirty = true;
			// Once per motion sample, not per pixel: keeps the sprite tileset fresh for
			// the animation preview even mid-stroke.
			FlushSpriteTileSync();
			damage.Inflate(1, 1);
			RefreshRect(damage);
			// No Update() here: leaving the repaint pending lets Windows batch several
			// mouse samples into one paint; the Bresenham join keeps the line unbroken.
		}
	}
}

bool SpriteFrameEditorCtrl::PaintGlobalPixel(int gx, int gy, uint8_t colour, wxRect& damage)
{
	const int tw = static_cast<int>(m_tiles->GetTileWidth());
	const int th = static_cast<int>(m_tiles->GetTileHeight());
	if ((gx < 0) || (gy < 0) || (gx >= MAX_WIDTH * tw) || (gy >= MAX_HEIGHT * th))
	{
		return false;
	}
	const int tile = (gx / tw) + (gy / th) * MAX_WIDTH;
	if ((tile >= static_cast<int>(m_tiles->GetTileCount())) || !IsTileInSprite(tile))
	{
		return false;
	}
	auto& pixels = m_tiles->GetTilePixels(tile);
	const std::size_t idx = (gx % tw) + (gy % th) * tw;
	if ((idx >= pixels.size()) || (pixels[idx] == colour))
	{
		return false;
	}
	pixels[idx] = colour;
	// The canvas is the master copy; the sprite's own tileset is brought back in step once
	// per operation via FlushSpriteTileSync - a whole-tile copy per pixel is wasteful.
	m_pending_sync.insert(tile);
	m_redraw_list.insert(tile);
	damage.Union(wxRect(gx * m_pixelsize - GetVisibleColumnsBegin() * m_cellwidth,
	                    gy * m_pixelsize - GetVisibleRowsBegin() * m_cellheight,
	                    m_pixelsize + 1, m_pixelsize + 1));
	return true;
}

wxRect SpriteFrameEditorCtrl::GlobalPixelBoxToClient(const wxPoint& a, const wxPoint& b) const
{
	const int sx = static_cast<int>(GetVisibleColumnsBegin()) * m_cellwidth;
	const int sy = static_cast<int>(GetVisibleRowsBegin()) * m_cellheight;
	const int x0 = std::min(a.x, b.x);
	const int x1 = std::max(a.x, b.x);
	const int y0 = std::min(a.y, b.y);
	const int y1 = std::max(a.y, b.y);
	wxRect rect(x0 * m_pixelsize - sx, y0 * m_pixelsize - sy,
	            (x1 - x0 + 1) * m_pixelsize + 1, (y1 - y0 + 1) * m_pixelsize + 1);
	rect.Inflate(2, 2);
	return rect;
}

void SpriteFrameEditorCtrl::RefreshGlobalPixel(const wxPoint& pixel)
{
	RefreshRect(GlobalPixelBoxToClient(pixel, pixel));
}

std::vector<wxPoint> SpriteFrameEditorCtrl::MakeShapePoints(Tool tool, const wxPoint& a, const wxPoint& b) const
{
	// Geometry shared with the tileset editor - see ImageBufferWx.
	switch (tool)
	{
	case Tool::Line:             return MakeShapeToolPoints(ShapeTool::Line, a, b);
	case Tool::RectangleOutline: return MakeShapeToolPoints(ShapeTool::RectangleOutline, a, b);
	case Tool::RectangleFilled:  return MakeShapeToolPoints(ShapeTool::RectangleFilled, a, b);
	case Tool::CircleOutline:    return MakeShapeToolPoints(ShapeTool::CircleOutline, a, b);
	case Tool::CircleFilled:     return MakeShapeToolPoints(ShapeTool::CircleFilled, a, b);
	default:                     return {};
	}
}

void SpriteFrameEditorCtrl::CommitShape()
{
	if (!m_shape_active)
	{
		return;
	}
	m_shape_active = false;
	// Erase the preview regardless of whether the commit changes anything.
	RefreshRect(GlobalPixelBoxToClient(m_shape_start, m_shape_end));
	if ((m_tiles == nullptr) || (m_sprite == nullptr))
	{
		return;
	}
	auto snapshot = MakeUndoState();
	const uint8_t colour = m_shape_secondary ? m_secondary_colour : m_primary_colour;
	bool changed = false;
	wxRect damage;
	for (const auto& p : MakeShapePoints(m_tool, m_shape_start, m_shape_end))
	{
		changed |= PaintGlobalPixel(p.x, p.y, colour, damage);
	}
	if (changed)
	{
		FlushSpriteTileSync();
		PushUndoState(std::move(snapshot));
		FireEvent(EVT_SPRITE_FRAME_CHANGE, std::to_string(m_selectedtile));
		damage.Inflate(1, 1);
		RefreshRect(damage);
	}
}

void SpriteFrameEditorCtrl::CancelShape()
{
	if (m_shape_active)
	{
		m_shape_active = false;
		RefreshRect(GlobalPixelBoxToClient(m_shape_start, m_shape_end));
	}
}

void SpriteFrameEditorCtrl::FloodFillAt(int gx, int gy, uint8_t colour)
{
	// Fills the contiguous same-colour region across the whole sprite, bounded by the
	// subsprite areas: unlike the tileset editor's per-tile fill, a sprite's tiles form
	// one continuous piece of artwork.
	if ((m_tiles == nullptr) || (m_sprite == nullptr))
	{
		return;
	}
	const int tw = static_cast<int>(m_tiles->GetTileWidth());
	const int th = static_cast<int>(m_tiles->GetTileHeight());
	const int target = GetColourAtPixel({ gx, gy });
	if ((target < 0) || (target == colour))
	{
		return;
	}
	auto snapshot = MakeUndoState();
	bool changed = false;
	wxRect damage;
	std::vector<wxPoint> stack{ wxPoint(gx, gy) };
	while (!stack.empty())
	{
		const wxPoint p = stack.back();
		stack.pop_back();
		if ((p.x < 0) || (p.y < 0) || (p.x >= MAX_WIDTH * tw) || (p.y >= MAX_HEIGHT * th))
		{
			continue;
		}
		const int tile = (p.x / tw) + (p.y / th) * MAX_WIDTH;
		if ((tile >= static_cast<int>(m_tiles->GetTileCount())) || !IsTileInSprite(tile))
		{
			continue;
		}
		auto& pixels = m_tiles->GetTilePixels(tile);
		uint8_t& value = pixels[(p.x % tw) + (p.y % th) * tw];
		if (value != static_cast<uint8_t>(target))
		{
			continue;
		}
		value = colour;
		m_pending_sync.insert(tile);
		m_redraw_list.insert(tile);
		damage.Union(wxRect(p.x * m_pixelsize - GetVisibleColumnsBegin() * m_cellwidth,
		                    p.y * m_pixelsize - GetVisibleRowsBegin() * m_cellheight,
		                    m_pixelsize + 1, m_pixelsize + 1));
		changed = true;
		stack.emplace_back(p.x + 1, p.y);
		stack.emplace_back(p.x - 1, p.y);
		stack.emplace_back(p.x, p.y + 1);
		stack.emplace_back(p.x, p.y - 1);
	}
	if (changed)
	{
		FlushSpriteTileSync();
		PushUndoState(std::move(snapshot));
		damage.Inflate(1, 1);
		RefreshRect(damage);
		FireEvent(EVT_SPRITE_FRAME_CHANGE, std::to_string(m_selectedtile));
	}
}

wxColour SpriteFrameEditorCtrl::GetPaletteColour(int index) const
{
	if ((m_pal == nullptr) || (index < 0))
	{
		return *wxBLACK;
	}
	return wxColour(m_pal->getBGRA(index) & 0xFFFFFF);
}

void SpriteFrameEditorCtrl::DrawPixelCursor(wxDC& dc)
{
	if ((m_mode != Mode::DRAW) || !IsPixelHoverValid() || m_shape_active ||
	    (m_tool == Tool::PixelSelect))
	{
		return;
	}
	wxPen cursor(GetPaletteColour(m_secondary_active ? m_secondary_colour : m_primary_colour));
	cursor.SetWidth(std::min((m_pixelsize + 1) / 2, 3));
	dc.SetPen(cursor);
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.DrawRectangle(m_hoveredpixel.x * m_pixelsize, m_hoveredpixel.y * m_pixelsize,
	                 m_pixelsize + 1, m_pixelsize + 1);
}

void SpriteFrameEditorCtrl::DrawShapePreview(wxDC& dc)
{
	if (!m_shape_active)
	{
		return;
	}
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(wxBrush(GetPaletteColour(m_shape_secondary ? m_secondary_colour : m_primary_colour)));
	for (const auto& p : MakeShapePoints(m_tool, m_shape_start, m_shape_end))
	{
		dc.DrawRectangle(p.x * m_pixelsize, p.y * m_pixelsize, m_pixelsize, m_pixelsize);
	}
}

void SpriteFrameEditorCtrl::PickColourAt(int gx, int gy, bool secondary)
{
	const int c = GetColourAtPixel({ gx, gy });
	if (c < 0)
	{
		return;
	}
	if (secondary)
	{
		if (m_secondary_colour == c)
		{
			return;
		}
		SetSecondaryColour(static_cast<uint8_t>(c));
	}
	else
	{
		if (m_primary_colour == c)
		{
			return;
		}
		SetPrimaryColour(static_cast<uint8_t>(c));
	}
	FireEvent(EVT_SPRITE_FRAME_COLOUR_PICK, (secondary ? 0x100 : 0) | c);
}

void SpriteFrameEditorCtrl::CycleColour(int delta, bool secondary)
{
	uint8_t& colour = secondary ? m_secondary_colour : m_primary_colour;
	colour = static_cast<uint8_t>((colour + delta + 16) & 0x0F);
	// Same notification as the eyedropper, so the palette pane and status bar follow.
	FireEvent(EVT_SPRITE_FRAME_COLOUR_PICK, (secondary ? 0x100 : 0) | colour);
	if (IsPixelHoverValid())
	{
		// The pen cursor outline is drawn in the active colour.
		RefreshGlobalPixel(m_hoveredpixel);
	}
}

void SpriteFrameEditorCtrl::CancelStroke()
{
	if (!m_stroke_dirty)
	{
		return;
	}
	m_stroke_dirty = false;
	m_pending_sync.clear();
	if (!m_undo_stack.empty())
	{
		// The stroke pushed its pre-state when its first pixel landed; pop that back
		// without disturbing the redo stack.
		auto state = std::move(m_undo_stack.back());
		m_undo_stack.pop_back();
		RestoreUndoState(state);
	}
}

bool SpriteFrameEditorCtrl::CancelActiveDrawOp()
{
	if (m_shape_active)
	{
		CancelShape();
		m_drawing = false;
		return true;
	}
	if (m_stroke_dirty)
	{
		CancelStroke();
		m_drawing = false;
		return true;
	}
	if (m_sel_drag != SelDrag::None)
	{
		CancelSelectionDrag();
		return true;
	}
	if (m_sel_floating)
	{
		// Esc cancels a pending paste outright rather than confirming it.
		DiscardFloating();
		const wxRect old = m_sel_rect;
		m_sel_rect = wxRect();
		RefreshSelectionRect(old);
		return true;
	}
	if (HasPixelSelection())
	{
		const wxRect old = m_sel_rect;
		m_sel_rect = wxRect();
		RefreshSelectionRect(old);
		return true;
	}
	return false;
}

bool SpriteFrameEditorCtrl::HasPixelSelection() const
{
	return (m_sel_rect.width > 0) && (m_sel_rect.height > 0);
}

void SpriteFrameEditorCtrl::BeginSelectionAction(int gx, int gy)
{
	const int tw = static_cast<int>(m_tiles->GetTileWidth());
	const int th = static_cast<int>(m_tiles->GetTileHeight());
	const int cw = MAX_WIDTH * tw;
	const int ch = MAX_HEIGHT * th;
	if (HasPixelSelection() && m_sel_rect.Contains(wxPoint(gx, gy)))
	{
		m_sel_anchor = wxPoint(gx - m_sel_rect.x, gy - m_sel_rect.y);
		m_sel_op_changed = false;
		if (m_sel_floating)
		{
			// Dragging a pending paste just moves the float; it stays unconfirmed.
			m_sel_drag = SelDrag::Move;
		}
		else
		{
			m_sel_snapshot = MakeUndoState();
			m_sel_snapshot_valid = true;
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
			LiftSelection(m_sel_drag == SelDrag::Move);
		}
		CaptureMouse();
	}
	else
	{
		// Clicking outside confirms a pending paste, clears the selection and starts a
		// fresh marquee from here.
		ClearPixelSelection(true);
		if ((gx >= 0) && (gy >= 0) && (gx < cw) && (gy < ch))
		{
			m_sel_drag = SelDrag::Marquee;
			m_sel_anchor = wxPoint(gx, gy);
			m_sel_rect = wxRect(gx, gy, 1, 1);
			RefreshSelectionRect(m_sel_rect);
			CaptureMouse();
		}
	}
}

void SpriteFrameEditorCtrl::UpdateSelectionDrag(int gx, int gy)
{
	const int tw = static_cast<int>(m_tiles->GetTileWidth());
	const int th = static_cast<int>(m_tiles->GetTileHeight());
	const int cw = MAX_WIDTH * tw;
	const int ch = MAX_HEIGHT * th;
	switch (m_sel_drag)
	{
	case SelDrag::Marquee:
	{
		const int px = std::clamp(gx, 0, cw - 1);
		const int py = std::clamp(gy, 0, ch - 1);
		const wxRect next(wxPoint(std::min(m_sel_anchor.x, px), std::min(m_sel_anchor.y, py)),
		                  wxSize(std::abs(px - m_sel_anchor.x) + 1, std::abs(py - m_sel_anchor.y) + 1));
		if (next != m_sel_rect)
		{
			RefreshSelectionRect(m_sel_rect);
			m_sel_rect = next;
			RefreshSelectionRect(m_sel_rect);
		}
		break;
	}
	case SelDrag::Move:
	case SelDrag::Duplicate:
	case SelDrag::Stamp:
	{
		wxPoint tl(gx - m_sel_anchor.x, gy - m_sel_anchor.y);
		tl.x = std::clamp(tl.x, 0, cw - m_sel_rect.width);
		tl.y = std::clamp(tl.y, 0, ch - m_sel_rect.height);
		if (tl != m_sel_rect.GetTopLeft())
		{
			RefreshSelectionRect(m_sel_rect);
			m_sel_rect.x = tl.x;
			m_sel_rect.y = tl.y;
			if (m_sel_drag == SelDrag::Stamp)
			{
				// Continuous duplication: every step leaves a copy on the canvas.
				m_sel_op_changed |= StampFloating();
			}
			RefreshSelectionRect(m_sel_rect);
		}
		break;
	}
	default:
		break;
	}
}

void SpriteFrameEditorCtrl::FinishSelectionDrag()
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
				m_sel_op_changed |= StampFloating();
			}
			m_sel_floating = false;
			m_float_bmp.reset();
			// A drag that ends where it started leaves the canvas untouched (erase and
			// re-stamp cancel out); comparing against the snapshot avoids a junk undo
			// entry for that case.
			bool push = false;
			if (m_sel_snapshot_valid && m_sel_op_changed)
			{
				const auto now = MakeUndoState();
				push = (now.tiles != m_sel_snapshot.tiles) ||
				       (now.subsprites != m_sel_snapshot.subsprites);
			}
			if (push)
			{
				PushUndoState(std::move(m_sel_snapshot));
				FireEvent(EVT_SPRITE_FRAME_CHANGE, std::to_string(m_selectedtile));
			}
			RefreshSelectionRect(m_sel_rect);
		}
	}
	m_sel_drag = SelDrag::None;
	m_sel_snapshot_valid = false;
	m_sel_op_changed = false;
}

void SpriteFrameEditorCtrl::CancelSelectionDrag()
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
		RefreshSelectionRect(old);
		break;
	}
	case SelDrag::Move:
	case SelDrag::Duplicate:
	case SelDrag::Stamp:
		if (m_sel_from_paste)
		{
			// Cancelling mid-drag drops the pending paste entirely.
			DiscardFloating();
			const wxRect old = m_sel_rect;
			m_sel_rect = wxRect();
			RefreshSelectionRect(old);
		}
		else if (m_sel_snapshot_valid)
		{
			// Puts the canvas back exactly as it was before the lift.
			m_sel_floating = false;
			m_float_bmp.reset();
			RestoreUndoState(m_sel_snapshot);
		}
		break;
	default:
		break;
	}
	m_sel_drag = SelDrag::None;
	m_sel_snapshot_valid = false;
	m_sel_op_changed = false;
}

void SpriteFrameEditorCtrl::ClearPixelSelection(bool confirm_floating)
{
	if (m_sel_floating)
	{
		if (confirm_floating)
		{
			ConfirmFloating();
		}
		else
		{
			DiscardFloating();
		}
	}
	if (HasPixelSelection())
	{
		const wxRect old = m_sel_rect;
		m_sel_rect = wxRect();
		RefreshSelectionRect(old);
	}
}

void SpriteFrameEditorCtrl::ConfirmFloating()
{
	if (!m_sel_floating)
	{
		return;
	}
	auto snapshot = MakeUndoState();
	const bool changed = StampFloating();
	m_sel_floating = false;
	m_sel_from_paste = false;
	m_float_bmp.reset();
	if (changed)
	{
		PushUndoState(std::move(snapshot));
		FireEvent(EVT_SPRITE_FRAME_CHANGE, std::to_string(m_selectedtile));
	}
	RefreshSelectionRect(m_sel_rect);
}

void SpriteFrameEditorCtrl::DiscardFloating()
{
	if (!m_sel_floating)
	{
		return;
	}
	m_sel_floating = false;
	m_sel_from_paste = false;
	m_float_bmp.reset();
	m_float_data.clear();
	RefreshSelectionRect(m_sel_rect);
}

void SpriteFrameEditorCtrl::LiftSelection(bool erase_source)
{
	m_float_data = ReadRect(m_sel_rect);
	if (erase_source)
	{
		bool changed = false;
		wxRect damage;
		for (int y = 0; y < m_sel_rect.height; ++y)
		{
			for (int x = 0; x < m_sel_rect.width; ++x)
			{
				changed |= PaintGlobalPixel(m_sel_rect.x + x, m_sel_rect.y + y,
				                            m_secondary_colour, damage);
			}
		}
		if (changed)
		{
			FlushSpriteTileSync();
			damage.Inflate(1, 1);
			RefreshRect(damage);
			m_sel_op_changed = true;
		}
	}
	m_sel_floating = true;
	m_sel_from_paste = false;
	RenderFloatBitmap();
	RefreshSelectionRect(m_sel_rect);
}

bool SpriteFrameEditorCtrl::StampFloating()
{
	if (m_float_data.size() !=
	    static_cast<std::size_t>(m_sel_rect.width) * static_cast<std::size_t>(m_sel_rect.height))
	{
		return false;
	}
	bool changed = false;
	wxRect damage;
	for (int y = 0; y < m_sel_rect.height; ++y)
	{
		for (int x = 0; x < m_sel_rect.width; ++x)
		{
			const uint8_t v = m_float_data[x + y * m_sel_rect.width];
			if (v == SEL_TRANSPARENT)
			{
				continue;
			}
			changed |= PaintGlobalPixel(m_sel_rect.x + x, m_sel_rect.y + y, v, damage);
		}
	}
	if (changed)
	{
		FlushSpriteTileSync();
		damage.Inflate(1, 1);
		RefreshRect(damage);
	}
	return changed;
}

void SpriteFrameEditorCtrl::RenderFloatBitmap()
{
	const auto& pal = GetSelectedPalette();
	wxImage img(m_sel_rect.width, m_sel_rect.height);
	img.SetAlpha();
	unsigned char* rgb = img.GetData();
	unsigned char* alpha = img.GetAlpha();
	for (std::size_t i = 0; i < m_float_data.size(); ++i)
	{
		const uint8_t v = m_float_data[i];
		if (v == SEL_TRANSPARENT)
		{
			alpha[i] = 0;
			continue;
		}
		const uint32_t c = pal.getBGRA(v);
		rgb[i * 3] = c & 0xFF;
		rgb[i * 3 + 1] = (c >> 8) & 0xFF;
		rgb[i * 3 + 2] = (c >> 16) & 0xFF;
		alpha[i] = c >> 24;
	}
	m_float_bmp = std::make_unique<wxBitmap>(img, 32);
}

void SpriteFrameEditorCtrl::FillSelection(uint8_t colour)
{
	if (!HasPixelSelection() || m_sel_floating)
	{
		return;
	}
	auto snapshot = MakeUndoState();
	bool changed = false;
	wxRect damage;
	for (int y = 0; y < m_sel_rect.height; ++y)
	{
		for (int x = 0; x < m_sel_rect.width; ++x)
		{
			changed |= PaintGlobalPixel(m_sel_rect.x + x, m_sel_rect.y + y, colour, damage);
		}
	}
	if (changed)
	{
		FlushSpriteTileSync();
		PushUndoState(std::move(snapshot));
		FireEvent(EVT_SPRITE_FRAME_CHANGE, std::to_string(m_selectedtile));
		damage.Inflate(1, 1);
		RefreshRect(damage);
	}
}

void SpriteFrameEditorCtrl::FlipSelection(bool horizontal)
{
	if (!HasPixelSelection())
	{
		return;
	}
	const int w = m_sel_rect.width;
	const int h = m_sel_rect.height;
	const auto flip = [&](const std::vector<uint8_t>& src)
	{
		std::vector<uint8_t> out(src.size());
		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				out[x + y * w] = horizontal ? src[(w - 1 - x) + y * w]
				                            : src[x + (h - 1 - y) * w];
			}
		}
		return out;
	};
	if (m_sel_floating)
	{
		m_float_data = flip(m_float_data);
		RenderFloatBitmap();
		RefreshSelectionRect(m_sel_rect);
		return;
	}
	auto snapshot = MakeUndoState();
	const auto flipped = flip(ReadRect(m_sel_rect));
	bool changed = false;
	wxRect damage;
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const uint8_t v = flipped[x + y * w];
			if (v == SEL_TRANSPARENT)
			{
				continue;
			}
			changed |= PaintGlobalPixel(m_sel_rect.x + x, m_sel_rect.y + y, v, damage);
		}
	}
	if (changed)
	{
		FlushSpriteTileSync();
		PushUndoState(std::move(snapshot));
		FireEvent(EVT_SPRITE_FRAME_CHANGE, std::to_string(m_selectedtile));
		damage.Inflate(1, 1);
		RefreshRect(damage);
	}
}

void SpriteFrameEditorCtrl::CopySelection()
{
	if (!HasPixelSelection())
	{
		return;
	}
	m_pixel_clipboard.rect = m_sel_rect;
	m_pixel_clipboard.data = m_sel_floating ? m_float_data : ReadRect(m_sel_rect);
}

void SpriteFrameEditorCtrl::CutSelection()
{
	if (!HasPixelSelection())
	{
		return;
	}
	CopySelection();
	if (m_sel_floating)
	{
		// Cutting a pending paste just removes the float; the canvas never had it.
		DiscardFloating();
	}
	else
	{
		FillSelection(m_secondary_colour);
	}
}

void SpriteFrameEditorCtrl::PastePixels()
{
	if (m_pixel_clipboard.data.empty())
	{
		return;
	}
	ClearPixelSelection(true);
	const int tw = static_cast<int>(m_tiles->GetTileWidth());
	const int th = static_cast<int>(m_tiles->GetTileHeight());
	wxRect r = m_pixel_clipboard.rect;
	r.x = std::clamp(r.x, 0, MAX_WIDTH * tw - r.width);
	r.y = std::clamp(r.y, 0, MAX_HEIGHT * th - r.height);
	m_sel_rect = r;
	m_float_data = m_pixel_clipboard.data;
	m_sel_floating = true;
	m_sel_from_paste = true;
	RenderFloatBitmap();
	RefreshSelectionRect(m_sel_rect);
}

void SpriteFrameEditorCtrl::SelectAllPixels()
{
	if ((m_sprite == nullptr) || (m_sprite->GetSubSpriteCount() == 0))
	{
		return;
	}
	ClearPixelSelection(true);
	const int tw = static_cast<int>(m_tiles->GetTileWidth());
	const int th = static_cast<int>(m_tiles->GetTileHeight());
	int minx = MAX_WIDTH;
	int miny = MAX_HEIGHT;
	int maxx = 0;
	int maxy = 0;
	for (const auto& s : m_sprite->GetSubSprites())
	{
		const int sxb = s.x / tw + ORIGIN_X;
		const int syb = s.y / th + ORIGIN_Y;
		minx = std::min(minx, sxb);
		miny = std::min(miny, syb);
		maxx = std::max(maxx, sxb + static_cast<int>(s.w));
		maxy = std::max(maxy, syb + static_cast<int>(s.h));
	}
	if ((maxx <= minx) || (maxy <= miny))
	{
		return;
	}
	m_sel_rect = wxRect(minx * tw, miny * th, (maxx - minx) * tw, (maxy - miny) * th);
	RefreshSelectionRect(m_sel_rect);
}

void SpriteFrameEditorCtrl::SelectHoveredCell()
{
	const int tile = (m_hoveredtile != -1) ? m_hoveredtile : m_selectedtile;
	if (tile < 0)
	{
		return;
	}
	ClearPixelSelection(true);
	const int tw = static_cast<int>(m_tiles->GetTileWidth());
	const int th = static_cast<int>(m_tiles->GetTileHeight());
	m_sel_rect = wxRect((tile % MAX_WIDTH) * tw, (tile / MAX_WIDTH) * th, tw, th);
	RefreshSelectionRect(m_sel_rect);
}

std::vector<uint8_t> SpriteFrameEditorCtrl::ReadRect(const wxRect& rect) const
{
	std::vector<uint8_t> out(static_cast<std::size_t>(rect.width) * static_cast<std::size_t>(rect.height),
	                         SEL_TRANSPARENT);
	for (int y = 0; y < rect.height; ++y)
	{
		for (int x = 0; x < rect.width; ++x)
		{
			const int c = GetColourAtPixel({ rect.x + x, rect.y + y });
			if (c >= 0)
			{
				out[x + y * rect.width] = static_cast<uint8_t>(c);
			}
		}
	}
	return out;
}

void SpriteFrameEditorCtrl::RefreshSelectionRect(const wxRect& rect)
{
	if ((rect.width <= 0) || (rect.height <= 0))
	{
		return;
	}
	RefreshRect(GlobalPixelBoxToClient(rect.GetTopLeft(), rect.GetBottomRight()));
}

void SpriteFrameEditorCtrl::DrawPixelSelection(wxDC& dc)
{
	if (!HasPixelSelection())
	{
		return;
	}
	if (m_sel_floating && (m_float_bmp != nullptr))
	{
		wxMemoryDC mem(*m_float_bmp);
		dc.StretchBlit(m_sel_rect.x * m_pixelsize, m_sel_rect.y * m_pixelsize,
		               m_sel_rect.width * m_pixelsize, m_sel_rect.height * m_pixelsize,
		               &mem, 0, 0, m_sel_rect.width, m_sel_rect.height, wxCOPY, true);
		mem.SelectObject(wxNullBitmap);
	}
	// White underlay + black dashes stays visible over any artwork.
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.SetPen(*wxWHITE_PEN);
	dc.DrawRectangle(m_sel_rect.x * m_pixelsize, m_sel_rect.y * m_pixelsize,
	                 m_sel_rect.width * m_pixelsize + 1, m_sel_rect.height * m_pixelsize + 1);
	dc.SetPen(wxPen(*wxBLACK, 1, wxPENSTYLE_SHORT_DASH));
	dc.DrawRectangle(m_sel_rect.x * m_pixelsize, m_sel_rect.y * m_pixelsize,
	                 m_sel_rect.width * m_pixelsize + 1, m_sel_rect.height * m_pixelsize + 1);
}

void SpriteFrameEditorCtrl::OnDraw(wxDC& dc)
{
	// Same pipeline as the tileset/blockset/map editors: tiles render at native resolution
	// into m_tiles_bmp when data changes (patched per tile for pixel edits), and each paint
	// is one scaled blit of the damaged cells plus overlays, clipped to the damaged area.
	dc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE)));
	if (!m_gd || (m_sprite == nullptr) || (m_tiles == nullptr))
	{
		dc.Clear();
		return;
	}
	if (m_tiles_bmp_dirty || (m_tiles_bmp == nullptr))
	{
		RenderTilesBitmap();
	}
	else if (!m_redraw_list.empty())
	{
		PatchTilesBitmap();
	}
	m_redraw_list.clear();
	if (m_tiles_bmp == nullptr)
	{
		dc.Clear();
		return;
	}

	const int tw = static_cast<int>(m_tiles->GetTileWidth());
	const int th = static_cast<int>(m_tiles->GetTileHeight());
	if ((m_cellwidth <= 0) || (m_cellheight <= 0))
	{
		dc.Clear();
		return;
	}

	wxRect damage = GetUpdateRegion().GetBox();
	damage.Offset(GetVisibleColumnsBegin() * m_cellwidth, GetVisibleRowsBegin() * m_cellheight);
	dc.SetClippingRegion(damage);
	dc.Clear();

	const int sx = std::max(static_cast<int>(GetVisibleColumnsBegin()), damage.GetLeft() / m_cellwidth);
	const int ex = std::min({ static_cast<int>(GetVisibleColumnsEnd()) + 1, MAX_WIDTH, damage.GetRight() / m_cellwidth + 1 });
	const int sy = std::max(static_cast<int>(GetVisibleRowsBegin()), damage.GetTop() / m_cellheight);
	const int ey = std::min({ static_cast<int>(GetVisibleRowsEnd()) + 1, MAX_HEIGHT, damage.GetBottom() / m_cellheight + 1 });
	if ((ex <= sx) || (ey <= sy))
	{
		return;
	}

	// Backdrop: dark outside the sprite, normal checkerboard inside the subsprite areas.
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(m_enablealpha ? *m_dark_alpha_brush : wxBrush(wxColor(64, 64, 64)));
	dc.DrawRectangle(sx * m_cellwidth, sy * m_cellheight, (ex - sx) * m_cellwidth, (ey - sy) * m_cellheight);
	dc.SetBrush(m_enablealpha ? *m_alpha_brush : *wxBLACK_BRUSH);
	for (int i = 0; i < static_cast<int>(m_sprite->GetSubSpriteCount()); ++i)
	{
		const auto& s = m_sprite->GetSubSprite(i);
		dc.DrawRectangle(SpriteToScreenXY({ s.x, s.y }),
			{ static_cast<int>(s.w * tw * m_pixelsize), static_cast<int>(s.h * th * m_pixelsize) });
	}

	// One scaled blit of the damaged cells out of the native-resolution bitmap.
	wxMemoryDC tiles(*m_tiles_bmp);
	dc.StretchBlit({ sx * m_cellwidth, sy * m_cellheight }, { (ex - sx) * m_cellwidth, (ey - sy) * m_cellheight },
		&tiles, { sx * tw, sy * th }, { (ex - sx) * tw, (ey - sy) * th },
		wxCOPY, true, { sx * tw, sy * th });
	tiles.SelectObject(wxNullBitmap);

	DrawOverlays(dc, sx, ex, sy, ey);
	DrawSelectionBorders(dc);

	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	// The subsprite editing tool needs visible targets, so its rectangles stay on even
	// when gridlines/borders are toggled off.
	if (m_enableborders || (m_mode == Mode::SUBSPRITE))
	{
		for (int i = 0; i < static_cast<int>(m_sprite->GetSubSpriteCount()); ++i)
		{
			const auto& s = m_sprite->GetSubSprite(i);
			dc.SetPen(wxPen(i + 1 == m_hovered_subsprite ? wxColor(255, 128, 128) : *wxRED, i + 1 == m_selected_subsprite ? 3 : 1));
			dc.DrawRectangle(SpriteToScreenXY({ s.x, s.y }), { static_cast<int>(s.w * tw * m_pixelsize), static_cast<int>(s.h * th * m_pixelsize) });
		}
		dc.SetPen(wxPen(*wxGREEN, 2));
		dc.DrawLine(SpriteToScreenXY({ -10, 0 }), SpriteToScreenXY({ 10, 0 }));
		dc.DrawLine(SpriteToScreenXY({ 0, -10 }), SpriteToScreenXY({ 0, 10 }));
	}
	DrawSubSpriteHandles(dc);
	DrawPixelCursor(dc);
	DrawShapePreview(dc);
	DrawPixelSelection(dc);
	if (m_enablehitbox)
	{
		auto hitbox = m_gd->GetSpriteData()->GetSpriteHitbox(m_sprite_id);
		dc.SetPen(wxPen(*wxYELLOW, 1));
		// The subsprite handles select a white brush; without resetting it the polygon
		// gets partially filled.
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		wxPoint hitbox_fg_points[] = {
			SpriteToScreenXY({ hitbox.base * 2, 0}),
			SpriteToScreenXY({ 0, hitbox.base}),
			SpriteToScreenXY({ -hitbox.base * 2, 0}),
			SpriteToScreenXY({ -hitbox.base * 2, -hitbox.height}),
			SpriteToScreenXY({ 0, hitbox.base - hitbox.height}),
			SpriteToScreenXY({ 0, hitbox.base}),
			SpriteToScreenXY({ 0, hitbox.base - hitbox.height}),
			SpriteToScreenXY({ hitbox.base * 2, 0 - hitbox.height}),
			SpriteToScreenXY({ hitbox.base * 2, 0}),
			SpriteToScreenXY({ hitbox.base * 2, 0 - hitbox.height}),
			SpriteToScreenXY({ 0, -hitbox.base - hitbox.height}),
			SpriteToScreenXY({ -hitbox.base * 2, -hitbox.height}),
			SpriteToScreenXY({ -hitbox.base * 2, 0}),
			SpriteToScreenXY({ 0, hitbox.base}),
			SpriteToScreenXY({ hitbox.base * 2, 0}),
		};
		dc.DrawPolygon(sizeof(hitbox_fg_points) / sizeof(hitbox_fg_points[0]), &hitbox_fg_points[0]);
	}
}

void SpriteFrameEditorCtrl::DrawOverlays(wxDC& dc, int sx, int ex, int sy, int ey)
{
	dc.SetTextForeground(wxColour(255, 255, 255));
	dc.SetTextBackground(wxColour(150, 150, 150));
	dc.SetBackgroundMode(wxSOLID);

	if (m_pixelsize > 3)
	{
		m_border_pen->SetStyle(wxPENSTYLE_SOLID);
	}
	else
	{
		m_border_pen->SetStyle(wxPENSTYLE_TRANSPARENT);
	}

	for (int y = sy; y < ey; ++y)
	{
		for (int x = sx; x < ex; ++x)
		{
			if (m_enableborders)
			{
				dc.SetPen(*m_border_pen);
				dc.SetBrush(*wxTRANSPARENT_BRUSH);
				dc.DrawRectangle({ x * m_cellwidth, y * m_cellheight, m_cellwidth + 1, m_cellheight + 1 });
			}
			if (m_enabletilenumbers)
			{
				const int pos = x + y * MAX_WIDTH;
				auto label = (wxString::Format("%03d", pos));
				auto extent = dc.GetTextExtent(label);
				if ((extent.GetWidth() < m_cellwidth - 2) && (extent.GetHeight() < m_cellheight - 2))
				{
					dc.DrawText(label, { x * m_cellwidth + 2, y * m_cellheight + 2 });
				}
			}
		}
	}
}

void SpriteFrameEditorCtrl::PatchTilesBitmap()
{
	if (m_tiles_bmp == nullptr)
	{
		RenderTilesBitmap();
		return;
	}
	wxAlphaPixelData data(*m_tiles_bmp);
	if (!data)
	{
		RenderTilesBitmap();
		return;
	}
	// Re-render just the changed tiles into the cached bitmap; the pixel editor fires one
	// change per painted pixel, and a full re-render for each made drawing crawl.
	const int tw = static_cast<int>(m_tiles->GetTileWidth());
	const int th = static_cast<int>(m_tiles->GetTileHeight());
	const auto& pal = GetSelectedPalette();
	for (const int pos : m_redraw_list)
	{
		if ((pos < 0) || (pos >= static_cast<int>(m_tiles->GetTileCount())) || (pos >= MAX_WIDTH * MAX_HEIGHT))
		{
			continue;
		}
		const auto tile_bytes = m_tiles->GetTile(pos);
		const int x0 = (pos % MAX_WIDTH) * tw;
		const int y0 = (pos / MAX_WIDTH) * th;
		const int lightness = IsTileInSprite(pos) ? 100 : 50;
		wxAlphaPixelData::Iterator p(data);
		for (int y = 0; y < th; ++y)
		{
			p.MoveTo(data, x0, y0 + y);
			for (int x = 0; x < tw; ++x)
			{
				const std::size_t i = x + y * tw;
				const uint32_t c = (i < tile_bytes.size()) ? pal.getBGRA(tile_bytes[i]) : 0;
				wxColour colour(c & 0xFFFFFF);
				if (lightness != 100)
				{
					colour = colour.ChangeLightness(lightness);
				}
				const unsigned char a = c >> 24;
				// The bitmap stores premultiplied alpha, as AlphaBlend expects.
				p.Red() = (colour.Red() * a) / 255;
				p.Green() = (colour.Green() * a) / 255;
				p.Blue() = (colour.Blue() * a) / 255;
				p.Alpha() = a;
				++p;
			}
		}
	}
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
	if ((m_mode == Mode::DRAW) && (m_sprite != nullptr))
	{
		m_drawing = true;
		m_secondary_active = false;
		m_last_drawn = { -1, -1 };
		StartDrawAction(MouseToLogical(evt.GetPosition()));
		evt.Skip();
		return;
	}
	if ((m_mode == Mode::SUBSPRITE) && (m_sprite != nullptr))
	{
		const wxPoint logical = MouseToLogical(evt.GetPosition());
		int edges = 0;
		if (HitTestSubSpriteHandles(logical, edges))
		{
			m_resizing_subsprite = true;
			m_drag_subsprite = m_selected_subsprite;
			m_drag_edges = edges;
			m_drag_undo_pushed = false;
			CaptureMouse();
		}
		else
		{
			const int tile = ConvertXYToTile(evt.GetPosition());
			const int ss = (tile >= 0) ? GetSubspriteAt(tile) : -1;
			if (ss > 0)
			{
				if (ss != m_selected_subsprite)
				{
					SelectSubSprite(ss);
					FireEvent(EVT_SUBSPRITE_SELECT, ss);
				}
				const auto& s = m_sprite->GetSubSprite(ss - 1);
				const wxPoint t = { logical.x / m_cellwidth, logical.y / m_cellheight };
				m_drag_offset = { t.x - (s.x / static_cast<int>(m_tiles->GetTileWidth()) + ORIGIN_X),
				                  t.y - (s.y / static_cast<int>(m_tiles->GetTileHeight()) + ORIGIN_Y) };
				m_drag_subsprite = ss;
				m_dragging_subsprite = true;
				m_drag_undo_pushed = false;
				CaptureMouse();
			}
			else if (m_selected_subsprite != -1)
			{
				SelectSubSprite(-1);
				FireEvent(EVT_SUBSPRITE_SELECT, -1);
			}
		}
		evt.Skip();
		return;
	}
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

void SpriteFrameEditorCtrl::OnMouseUp(wxMouseEvent& evt)
{
	if (m_dragging_subsprite || m_resizing_subsprite)
	{
		EndSubSpriteDrag();
	}
	if (m_mode == Mode::DRAW)
	{
		if (evt.LeftUp() && (m_sel_drag != SelDrag::None))
		{
			if (evt.RightIsDown())
			{
				CancelSelectionDrag();
			}
			else
			{
				FinishSelectionDrag();
			}
			m_drawing = false;
			evt.Skip();
			return;
		}
		if (evt.LeftUp())
		{
			if (evt.RightIsDown())
			{
				// Releasing the left button with the right still held cancels the
				// operation in progress, matching classic paint programs.
				CancelShape();
				CancelStroke();
				m_drawing = false;
				m_secondary_active = false;
				m_last_drawn = { -1, -1 };
				evt.Skip();
				return;
			}
			m_drawing = false;
			m_secondary_active = false;
		}
		if (evt.RightUp())
		{
			m_secondary_active = false;
			if (!evt.LeftIsDown())
			{
				m_drawing = false;
			}
		}
		if (!m_drawing)
		{
			m_last_drawn = { -1, -1 };
			if (m_shape_active)
			{
				CommitShape();
			}
			EndStroke();
		}
	}
	evt.Skip();
}

// Fires the deferred change notification if the finished stroke painted anything.
void SpriteFrameEditorCtrl::EndStroke()
{
	if (m_stroke_dirty)
	{
		m_stroke_dirty = false;
		FlushSpriteTileSync();
		FireEvent(EVT_SPRITE_FRAME_CHANGE, std::to_string(m_selectedtile));
	}
}

void SpriteFrameEditorCtrl::OnCaptureLost(wxMouseCaptureLostEvent& /*evt*/)
{
	// Capture already gone - just drop the drag state.
	m_dragging_subsprite = false;
	m_resizing_subsprite = false;
	m_drag_subsprite = -1;
	m_drag_edges = 0;
	m_sel_drag = SelDrag::None;
	m_sel_snapshot_valid = false;
	m_sel_op_changed = false;
}

void SpriteFrameEditorCtrl::EndSubSpriteDrag()
{
	if (HasCapture())
	{
		ReleaseMouse();
	}
	m_dragging_subsprite = false;
	m_resizing_subsprite = false;
	m_drag_subsprite = -1;
	m_drag_edges = 0;
}

void SpriteFrameEditorCtrl::OnRightDown(wxMouseEvent& evt)
{
	if ((m_mode == Mode::DRAW) && (m_sprite != nullptr))
	{
		m_drawing = true;
		m_secondary_active = true;
		m_last_drawn = { -1, -1 };
		StartDrawAction(MouseToLogical(evt.GetPosition()));
		evt.Skip();
		return;
	}
	if ((m_mode != Mode::SUBSPRITE) || (m_sprite == nullptr) ||
	    m_dragging_subsprite || m_resizing_subsprite)
	{
		evt.Skip();
		return;
	}
	const int tile = ConvertXYToTile(evt.GetPosition());
	const int ss = (tile >= 0) ? GetSubspriteAt(tile) : -1;
	const wxPoint logical = MouseToLogical(evt.GetPosition());
	const int tx = logical.x / m_cellwidth;
	const int ty = logical.y / m_cellheight;
	if ((ss > 0) && (ss != m_selected_subsprite))
	{
		SelectSubSprite(ss);
		FireEvent(EVT_SUBSPRITE_SELECT, ss);
	}
	wxMenu menu;
	auto* add = menu.Append(wxID_ANY, "Add Subsprite Here\tIns");
	add->Enable((tile >= 0) && (ss <= 0) &&
		(m_sprite->GetSubSpriteCount() < Landstalker::SpriteFrame::MAX_SUBSPRITES));
	auto* del = menu.Append(wxID_ANY, "Delete Subsprite\tDel");
	del->Enable(ss > 0);
	menu.Bind(wxEVT_MENU, [this, tx, ty](wxCommandEvent&) { AddSubSpriteAt(tx, ty); }, add->GetId());
	menu.Bind(wxEVT_MENU, [this, ss](wxCommandEvent&) { FireEvent(EVT_SUBSPRITE_DELETE, ss); }, del->GetId());
	PopupMenu(&menu);
}

void SpriteFrameEditorCtrl::AddSubSpriteAt(int tx, int ty)
{
	if ((m_sprite == nullptr) ||
	    (m_sprite->GetSubSpriteCount() >= Landstalker::SpriteFrame::MAX_SUBSPRITES) ||
	    (tx < 0) || (ty < 0) || (tx >= MAX_WIDTH) || (ty >= MAX_HEIGHT))
	{
		return;
	}
	const int tw = static_cast<int>(m_tiles->GetTileWidth());
	const int th = static_cast<int>(m_tiles->GetTileHeight());
	const Landstalker::SpriteFrame::SubSprite new_s((tx - ORIGIN_X) * tw, (ty - ORIGIN_Y) * th, 1, 1);
	if (CheckSubSpriteCollision(new_s, -1))
	{
		return;
	}
	PushUndo();
	// AddSubSpriteBefore picks its own free spot; the reference lets it be moved to the click.
	auto& s = m_sprite->AddSubSpriteBefore(0);
	s = new_s;
	m_selected_subsprite = 1;
	FireEvent(EVT_SUBSPRITE_UPDATE);
	FireEvent(EVT_SUBSPRITE_SELECT, 1);
}

void SpriteFrameEditorCtrl::OnDoubleClick(wxMouseEvent& evt)
{
	// Rapid clicks arrive as down/up/dclick/up; treating the dclick as another mouse-down
	// stops every second pencil click being dropped.
	if (m_mode == Mode::DRAW)
	{
		OnMouseDown(evt);
		return;
	}
	evt.Skip();
}

void SpriteFrameEditorCtrl::OnMouseMove(wxMouseEvent& evt)
{
	if (m_sprite == nullptr) return;
	if (m_mode == Mode::DRAW)
	{
		MouseDrawMove(MouseToLogical(evt.GetPosition()));
		evt.Skip();
		return;
	}
	if (m_mode == Mode::SUBSPRITE)
	{
		const wxPoint logical = MouseToLogical(evt.GetPosition());
		if (m_resizing_subsprite)
		{
			DoSubSpriteResize(logical);
		}
		else if (m_dragging_subsprite)
		{
			DoSubSpriteDrag(logical);
		}
		else
		{
			const int tile = ConvertXYToTile(evt.GetPosition());
			const int hovered_subsprite = (tile >= 0) ? GetSubspriteAt(tile) : -1;
			if (hovered_subsprite != m_hovered_subsprite)
			{
				m_hovered_subsprite = hovered_subsprite;
				Refresh();
			}
			UpdateSubSpriteCursor(logical);
		}
		evt.Skip();
		return;
	}
	if (!m_enablehover) return;
	int sel = ConvertXYToTile(evt.GetPosition());
	int hovered_tile = (evt.GetModifiers() & wxMOD_CONTROL) ? -1 : sel;
	if (hovered_tile != m_hoveredtile)
	{
		const int old = m_hoveredtile;
		m_hoveredtile = hovered_tile;
		FireEvent(EVT_SPRITE_FRAME_HOVER, std::to_string(m_hoveredtile));
		RefreshTileRect(old);
		RefreshTileRect(m_hoveredtile);
	}
	int hovered_subsprite = GetSubspriteAt(sel);
	if (hovered_subsprite != m_hovered_subsprite)
	{
		// Only the subsprite border highlight changes, and that is repainted as an overlay -
		// no need to re-render any tiles.
		m_hovered_subsprite = hovered_subsprite;
		Refresh();
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
	if (m_dragging_subsprite || m_resizing_subsprite)
	{
		// The mouse is captured; the drag continues outside the window.
		evt.Skip();
		return;
	}
	if (m_mode == Mode::DRAW)
	{
		if (m_sel_drag != SelDrag::None)
		{
			// The mouse is captured; the selection drag continues outside the window.
			evt.Skip();
			return;
		}
		// The stroke pauses outside the window; OnMouseEnter decides whether it resumes
		// from the real button state. An in-progress shape is cancelled outright: its
		// anchor would be stale by the time the pointer returns.
		CancelShape();
		m_last_drawn = { -1, -1 };
		EndStroke();
		if (IsPixelHoverValid())
		{
			RefreshGlobalPixel(m_hoveredpixel);
			m_hoveredpixel = { -1, -1 };
			FireEvent(EVT_SPRITE_FRAME_HOVER, std::to_string(m_hoveredtile));
		}
	}
	SetMouseCursor(wxStockCursor::wxCURSOR_ARROW);
	if (!m_enablehover) return;
	if (m_hoveredtile != -1)
	{
		const int old = m_hoveredtile;
		m_hoveredtile = -1;
		FireEvent(EVT_SPRITE_FRAME_HOVER, std::to_string(m_hoveredtile));
		RefreshTileRect(old);
	}
	if (m_hovered_subsprite != -1)
	{
		m_hovered_subsprite = -1;
		Refresh();
	}
	evt.Skip();
}

void SpriteFrameEditorCtrl::OnMouseEnter(wxMouseEvent& evt)
{
	if (m_mode == Mode::DRAW)
	{
		// Resume a stroke that left the canvas with the button still held, or end it if
		// the button was released while outside - that release never reaches this window.
		if (m_drawing)
		{
			if (evt.LeftIsDown())
			{
				m_secondary_active = false;
			}
			else if (evt.RightIsDown())
			{
				m_secondary_active = true;
			}
			else
			{
				m_drawing = false;
				m_secondary_active = false;
			}
		}
		m_last_drawn = { -1, -1 };
	}
	evt.Skip();
}

void SpriteFrameEditorCtrl::OnTilesetFocus(wxFocusEvent& evt)
{
	FireEvent(EVT_SPRITE_FRAME_ACTIVATE, "");
	evt.Skip();
}

bool SpriteFrameEditorCtrl::HandleKeyDown(int key, int modifiers)
{
	const bool pixel_select = (m_mode == Mode::DRAW) && (m_tool == Tool::PixelSelect);
	switch (key)
	{
	case WXK_ESCAPE:
		// In draw mode Esc cancels whatever is in flight (shape, stroke, drag, pending
		// paste), then clears the pixel selection; otherwise the subsprite selection.
		if ((m_mode == Mode::DRAW) && CancelActiveDrawOp())
		{
			break;
		}
		ClearSelections();
		break;
	// Movement keys: in subsprite mode they nudge the selected subsprite (Shift resizes);
	// in select mode they move the tile selection. All other subsprite bindings (Tab,
	// Insert/Delete, priority) are likewise only live in subsprite mode.
	case WXK_UP:
	case 'w':
	case 'W':
		if (m_mode == Mode::SUBSPRITE)
		{
			if ((modifiers == 0) || (modifiers == wxMOD_CONTROL))
			{
				MoveSubSpriteUp();
			}
			else if (modifiers == wxMOD_SHIFT)
			{
				ContractSubSpriteHeight();
			}
		}
		else if ((m_mode == Mode::SELECT) && (modifiers == 0))
		{
			MoveSelectionUp();
		}
		else
		{
			return false;
		}
		break;
	case WXK_DOWN:
	case 's':
	case 'S':
		if (m_mode == Mode::SUBSPRITE)
		{
			if ((modifiers == 0) || (modifiers == wxMOD_CONTROL))
			{
				MoveSubSpriteDown();
			}
			else if (modifiers == wxMOD_SHIFT)
			{
				ExpandSubSpriteHeight();
			}
		}
		else if ((m_mode == Mode::SELECT) && (modifiers == 0))
		{
			MoveSelectionDown();
		}
		else
		{
			return false;
		}
		break;
	case WXK_LEFT:
	case 'a':
	case 'A':
		if (pixel_select && (modifiers == wxMOD_CONTROL) && (key != WXK_LEFT))
		{
			SelectAllPixels();
		}
		else if (m_mode == Mode::SUBSPRITE)
		{
			if ((modifiers == 0) || (modifiers == wxMOD_CONTROL))
			{
				MoveSubSpriteLeft();
			}
			else if (modifiers == wxMOD_SHIFT)
			{
				ContractSubSpriteWidth();
			}
		}
		else if ((m_mode == Mode::SELECT) && (modifiers == 0))
		{
			MoveSelectionLeft();
		}
		else
		{
			return false;
		}
		break;
	case WXK_RIGHT:
	case 'd':
	case 'D':
		if (m_mode == Mode::SUBSPRITE)
		{
			if ((modifiers == 0) || (modifiers == wxMOD_CONTROL))
			{
				MoveSubSpriteRight();
			}
			else if (modifiers == wxMOD_SHIFT)
			{
				ExpandSubSpriteWidth();
			}
		}
		else if ((m_mode == Mode::SELECT) && (modifiers == 0))
		{
			MoveSelectionRight();
		}
		else
		{
			return false;
		}
		break;
	case WXK_TAB:
		if (m_mode != Mode::SUBSPRITE)
		{
			return false;
		}
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
		if (m_mode == Mode::SUBSPRITE)
		{
			if ((modifiers == 0) || (modifiers == wxMOD_SHIFT))
			{
				DeleteSubSprite();
			}
		}
		else if (modifiers == 0)
		{
			// With a pixel selection Delete wipes the rectangle to the secondary
			// colour; otherwise it clears the selected tile.
			if (pixel_select && HasPixelSelection())
			{
				FillSelection(m_secondary_colour);
			}
			else
			{
				ClearCell();
			}
		}
		else
		{
			return false;
		}
		break;
	case WXK_INSERT:
		if (m_mode != Mode::SUBSPRITE)
		{
			return false;
		}
		InsertSubSprite();
		break;
	case '[':
	case '{':
		if (m_mode != Mode::SUBSPRITE)
		{
			return false;
		}
		IncreaseSubSpritePriority();
		break;
	case ']':
	case '}':
		if (m_mode != Mode::SUBSPRITE)
		{
			return false;
		}
		DecreaseSubSpritePriority();
		break;
	case 'x':
	case 'X':
		if (modifiers == wxMOD_CONTROL)
		{
			if (pixel_select)
			{
				CutSelection();
			}
			else
			{
				CutCell();
			}
		}
		break;
	case 'c':
	case 'C':
		if (modifiers == wxMOD_CONTROL)
		{
			if (pixel_select)
			{
				CopySelection();
			}
			else
			{
				CopyCell();
			}
		}
		break;
	case 'v':
	case 'V':
		if (modifiers == wxMOD_CONTROL)
		{
			if (pixel_select)
			{
				PastePixels();
			}
			else
			{
				PasteCell();
			}
		}
		break;
	case 'b':
	case 'B':
		if ((modifiers == wxMOD_CONTROL) && pixel_select)
		{
			SelectHoveredCell();
		}
		break;
	case 'h':
	case 'H':
		if ((modifiers == wxMOD_CONTROL) && pixel_select)
		{
			FlipSelection(true);
		}
		break;
	case 'e':
	case 'E':
		if ((modifiers == wxMOD_CONTROL) && pixel_select)
		{
			FlipSelection(false);
		}
		break;
	case 'p':
	case 'P':
		if (modifiers == wxMOD_CONTROL)
		{
			SwapCell();
		}
		break;
	// The main-row plus shares a key with equals, so the unshifted key cycles the primary
	// colour and the shifted one ('+' proper) the secondary; the numpad keys distinguish
	// by the Shift modifier alone.
	case '+':
	case '=':
	case WXK_NUMPAD_ADD:
		if (m_mode != Mode::DRAW)
		{
			return false;
		}
		CycleColour(1, (modifiers & wxMOD_SHIFT) != 0);
		break;
	case '-':
	case '_':
	case WXK_NUMPAD_SUBTRACT:
		if (m_mode != Mode::DRAW)
		{
			return false;
		}
		CycleColour(-1, (modifiers & wxMOD_SHIFT) != 0);
		break;
	default:
		return false;
	}
	return true;
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
	// Everything is about to be rewritten from the canvas; pending per-tile syncs are moot.
	m_pending_sync.clear();
	int cur_tile = 0;
	m_sprite->PrepareSubSprites();
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

void SpriteFrameEditorCtrl::RenderTilesBitmap()
{
	// Every cell rendered once at native resolution (with out-of-sprite tiles darkened),
	// rebuilt only when the sprite or its tiles change. Building a bitmap per tile per
	// paint made opening the editor crawl.
	if ((m_tiles == nullptr) || (m_sprite == nullptr))
	{
		return;
	}
	const int tw = static_cast<int>(m_tiles->GetTileWidth());
	const int th = static_cast<int>(m_tiles->GetTileHeight());
	const auto& pal = GetSelectedPalette();
	wxImage img(MAX_WIDTH * tw, MAX_HEIGHT * th);
	img.SetAlpha();
	std::vector<uint32_t> tile_pixels;
	for (int pos = 0; pos < MAX_WIDTH * MAX_HEIGHT; ++pos)
	{
		if (pos >= static_cast<int>(m_tiles->GetTileCount()))
		{
			break;
		}
		const auto tile_bytes = m_tiles->GetTile(pos);
		tile_pixels.clear();
		tile_pixels.reserve(tile_bytes.size());
		for (const auto& b : tile_bytes)
		{
			tile_pixels.push_back(pal.getBGRA(b));
		}
		WriteTileToImage(img, (pos % MAX_WIDTH) * tw, (pos / MAX_WIDTH) * th,
			tile_pixels, tw, th, IsTileInSprite(pos) ? 100 : 50);
	}
	m_tiles_bmp = std::make_unique<wxBitmap>(img, 32);
	m_tiles_bmp_dirty = false;
}

void SpriteFrameEditorCtrl::RefreshTileRect(int tile)
{
	// Repaints a single cell. Hover changes happen on every mouse move, and a full-window
	// Refresh for each is what made the cursor lag.
	if (tile < 0)
	{
		return;
	}
	wxRect rect(((tile % MAX_WIDTH) - GetVisibleColumnsBegin()) * m_cellwidth,
	            ((tile / MAX_WIDTH) - GetVisibleRowsBegin()) * m_cellheight,
	            m_cellwidth + 1, m_cellheight + 1);
	rect.Inflate(1, 1);
	RefreshRect(rect);
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
	m_tiles_bmp_dirty = true;
	wxVarHScrollHelper::RefreshAll();
	wxVarVScrollHelper::RefreshAll();
	Refresh();
}

const Landstalker::Palette& SpriteFrameEditorCtrl::GetSelectedPalette()
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

void SpriteFrameEditorCtrl::SetActivePalette(const std::shared_ptr<Landstalker::Palette> pal)
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
	Refresh();
}

std::shared_ptr<Landstalker::Tileset> SpriteFrameEditorCtrl::GetTileset()
{
	return m_tiles;
}

bool SpriteFrameEditorCtrl::GetSelectionEnabled() const
{
	return m_enableselection;
}

void SpriteFrameEditorCtrl::SetSelectionEnabled(bool enabled)
{
	if (m_enableselection != enabled)
	{
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

bool SpriteFrameEditorCtrl::GetHitboxEnabled() const
{
	return m_enablehitbox;
}

void SpriteFrameEditorCtrl::SetHitboxEnabled(bool enabled)
{
	if (m_enablehitbox != enabled)
	{
		m_enablehitbox = enabled;
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

Landstalker::Tile SpriteFrameEditorCtrl::GetSelectedTile() const
{
	return Landstalker::Tile(m_selectedtile);
}

std::pair<int, int> SpriteFrameEditorCtrl::GetSelectedTilePosition() const
{
	return GetTilePosition(m_selectedtile);
}

bool SpriteFrameEditorCtrl::IsHoverValid() const
{
	return m_hoveredtile != -1 && (m_hoveredtile < static_cast<int>(m_tiles->GetTileCount())) && IsTileInSprite(m_hoveredtile);
}

Landstalker::Tile SpriteFrameEditorCtrl::GetHoveredTile() const
{
	return Landstalker::Tile(m_hoveredtile);
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
	if (tile != m_selectedtile)
	{
		const int old = m_selectedtile;
		FireEvent(EVT_SPRITE_FRAME_SELECT, std::to_string(tile));
		m_selectedtile = tile;
		// The selection border is an overlay: repaint just the two affected cells rather
		// than the whole window.
		RefreshTileRect(old);
		RefreshTileRect(tile);
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
