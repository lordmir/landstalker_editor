#include "RoomViewerCtrl.h"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <unordered_map>

#include "EditorFrame.h"
#include "EntityPropertiesWindow.h"
#include "WarpPropertyWindow.h"

wxDEFINE_EVENT(EVT_ENTITY_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(EVT_WARP_UPDATE, wxCommandEvent);

wxBEGIN_EVENT_TABLE(RoomViewerCtrl, wxScrolledCanvas)
EVT_ERASE_BACKGROUND(RoomViewerCtrl::OnEraseBackground)
EVT_PAINT(RoomViewerCtrl::OnPaint)
EVT_SIZE(RoomViewerCtrl::OnSize)
EVT_MOTION(RoomViewerCtrl::OnMouseMove)
EVT_LEAVE_WINDOW(RoomViewerCtrl::OnMouseLeave)
EVT_LEFT_UP(RoomViewerCtrl::OnLeftClick)
EVT_LEFT_DCLICK(RoomViewerCtrl::OnLeftDblClick)
EVT_RIGHT_UP(RoomViewerCtrl::OnRightClick)
EVT_RIGHT_DCLICK(RoomViewerCtrl::OnRightDblClick)
wxEND_EVENT_TABLE()

RoomViewerCtrl::RoomViewerCtrl(wxWindow* parent, RoomViewerFrame* frame)
	: wxScrolledCanvas(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxWANTS_CHARS),
      m_g(nullptr),
	  m_roomnum(0),
      m_frame(frame),
      m_width(1),
      m_height(1),
      m_buffer_width(1),
      m_buffer_height(1),
      m_redraw(false),
      m_repaint(false),
      m_zoom(1.0),
      m_show_entities(true),
      m_show_warps(true),
      m_show_swaps(true),
      m_show_entity_hitboxes(true),
      m_is_warp_pending(false),
      m_bmp(std::make_unique<wxBitmap>()),
      m_scroll_rate(SCROLL_RATE),
      m_selected(NO_SELECTION),
      m_hovered(NO_SELECTION)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxBLACK);
    m_warp_brush = std::make_unique<wxBrush>(*wxRED, wxBRUSHSTYLE_BDIAGONAL_HATCH);
    m_layer_opacity = { {Layer::BACKGROUND1, 0xFF}, {Layer::BACKGROUND2, 0xFF}, {Layer::BG_SPRITES, 0xFF },
                        {Layer::FOREGROUND, 0xFF}, {Layer::FG_SPRITES, 0xFF}, {Layer::HEIGHTMAP, 0x80} };
    m_layer_bufs = { {Layer::BACKGROUND1, std::make_unique<ImageBuffer>()}, {Layer::BACKGROUND2, std::make_unique<ImageBuffer>()},
                     {Layer::BG_SPRITES,  std::make_unique<ImageBuffer>()}, {Layer::FOREGROUND,  std::make_unique<ImageBuffer>()},
                     {Layer::FG_SPRITES,  std::make_unique<ImageBuffer>()} };
    m_layers = { {Layer::BACKGROUND1, std::make_unique<wxBitmap>()},              {Layer::BACKGROUND2, std::make_unique<wxBitmap>()},
                 {Layer::BG_SPRITES_WIREFRAME_BG, std::make_unique<wxBitmap>()},  {Layer::BG_SPRITES,  std::make_unique<wxBitmap>()},
                 {Layer::BG_SPRITES_WIREFRAME_FG, std::make_unique<wxBitmap>()},  {Layer::FOREGROUND,  std::make_unique<wxBitmap>()},
                 {Layer::SWAPS, std::make_unique<wxBitmap>()},                    {Layer::HEIGHTMAP, std::make_unique<wxBitmap>()},
                 {Layer::FG_SPRITES_WIREFRAME_BG, std::make_unique<wxBitmap>()},  {Layer::FG_SPRITES, std::make_unique<wxBitmap>()},
                 {Layer::FG_SPRITES_WIREFRAME_FG,  std::make_unique<wxBitmap>()}, {Layer::WARPS, std::make_unique<wxBitmap>()} };
}

RoomViewerCtrl::~RoomViewerCtrl()
{
}

void RoomViewerCtrl::SetGameData(std::shared_ptr<GameData> gd)
{
	m_g = gd;
	Refresh(true);
}

void RoomViewerCtrl::ClearGameData()
{
	m_g = nullptr;
	Refresh(true);
}

void RoomViewerCtrl::SetRoomNum(uint16_t roomnum)
{
	m_roomnum = roomnum;
    m_selected = NO_SELECTION;
    m_hovered = NO_SELECTION;
    if (m_is_warp_pending)
    {
        for (const auto& warp : m_warps)
        {
            if (!warp.IsValid())
            {
                m_pending_warp = warp;
            }
        }
    }
    if (m_g != nullptr)
    {
        m_entities = m_g->GetSpriteData()->GetRoomEntities(roomnum);
        m_warps = m_g->GetRoomData()->GetWarpsForRoom(roomnum);
        if (m_is_warp_pending)
        {
            m_warps.push_back(m_pending_warp);
        }
        FireEvent(EVT_ENTITY_UPDATE);
        FireEvent(EVT_WARP_UPDATE);
    }
    UpdateRoomDescText(m_roomnum);
    RefreshGraphics();
}

bool RoomViewerCtrl::GetEntitiesVisible() const
{
    return m_show_entities;
}

bool RoomViewerCtrl::GetEntitiesHitboxVisible() const
{
    return m_show_entity_hitboxes;
}

bool RoomViewerCtrl::GetWarpsVisible() const
{
    return m_show_warps;
}

bool RoomViewerCtrl::GetTileSwapsVisible() const
{
    return m_show_swaps;
}

void RoomViewerCtrl::SetEntitiesVisible(bool visible)
{
    m_show_entities = visible;
    RefreshGraphics();
}

void RoomViewerCtrl::SetEntitiesHitboxVisible(bool visible)
{
    m_show_entity_hitboxes = visible;
    RefreshGraphics();
}

void RoomViewerCtrl::SetWarpsVisible(bool visible)
{
    m_show_warps = visible;
    RefreshGraphics();
}

void RoomViewerCtrl::SetTileSwapsVisible(bool visible)
{
    m_show_swaps = visible;
    RefreshGraphics();
}

bool RoomViewerCtrl::EntitiesEnabled() const
{
    return m_show_entities;
}

void RoomViewerCtrl::SelectEntity(int selection)
{
    if (selection != m_selected && (selection == NO_SELECTION
        || (selection > ENTITY_IDX_OFFSET && selection <= ENTITY_IDX_OFFSET + static_cast<int>(m_entities.size()))))
    {
        if (IsWarpSelected() && m_show_warps)
        {
            m_selected = selection;
            UpdateLayer(Layer::WARPS, DrawRoomWarps(m_roomnum));
            UpdateLayer(Layer::SWAPS, DrawRoomSwaps(m_roomnum));
            FireEvent(EVT_WARP_UPDATE);
        }
        else
        {
            m_selected = selection;
        }
        FireEvent(EVT_ENTITY_UPDATE);
        RefreshStatusbar();
        RedrawAllSprites();
        ForceRedraw();
    }
}

void RoomViewerCtrl::ClearSelection()
{
    if (m_selected != NO_SELECTION)
    {
        m_selected = NO_SELECTION;
        FireEvent(EVT_WARP_UPDATE);
        FireEvent(EVT_ENTITY_UPDATE);
        FireEvent(EVT_TILESWAP_SELECT, NO_SELECTION);
        FireEvent(EVT_DOOR_SELECT, NO_SELECTION);
        RefreshStatusbar();
        RedrawAllSprites();
        if (m_show_warps)
        {
            m_layers[Layer::WARPS] = DrawRoomWarps(m_roomnum);
        }
        if (m_show_swaps)
        {
            m_layers[Layer::SWAPS] = DrawRoomSwaps(m_roomnum);
        }
        ForceRedraw();
    }
}

int RoomViewerCtrl::GetTotalEntities() const
{
    return m_entities.size();
}

bool RoomViewerCtrl::IsEntitySelected() const
{
    return (m_selected > 0 && m_selected <= static_cast<int>(m_entities.size()));
}

int RoomViewerCtrl::GetSelectedEntityIndex() const
{
    if (IsEntitySelected())
    {
        return m_selected;
    }
    else
    {
        return -1;
    }
}

const Entity& RoomViewerCtrl::GetSelectedEntity() const
{
    assert(m_selected > static_cast<int>(ENTITY_IDX_OFFSET) && m_selected <= static_cast<int>(ENTITY_IDX_OFFSET + m_entities.size()));
    return m_entities[m_selected - ENTITY_IDX_OFFSET - 1];
}

const std::vector<Entity>& RoomViewerCtrl::GetEntities() const
{
    return m_entities;
}

const std::vector<WarpList::Warp>& RoomViewerCtrl::GetWarps() const
{
    return m_warps;
}

void RoomViewerCtrl::SelectWarp(int selection)
{
    bool warp_currently_selected = (m_selected > WARP_IDX_OFFSET && m_selected <= static_cast<int>(m_warps.size() + WARP_IDX_OFFSET));
    int cur_sel = warp_currently_selected ? m_selected : NO_SELECTION;
    int new_sel = (selection > 0 && selection <= static_cast<int>(m_warps.size())) ? selection + WARP_IDX_OFFSET : NO_SELECTION;

    if ((warp_currently_selected && new_sel == NO_SELECTION) || (cur_sel != new_sel && new_sel != NO_SELECTION))
    {
        if (m_selected != NO_SELECTION)
        {
            m_selected = new_sel;
            RedrawAllSprites();
            m_layers[Layer::SWAPS] = DrawRoomSwaps(m_roomnum);
            FireEvent(EVT_ENTITY_UPDATE);
        }
        else
        {
            m_selected = new_sel;
        }
        RefreshStatusbar();
        FireEvent(EVT_WARP_UPDATE);
        if (m_show_warps)
        {
            m_layers[Layer::WARPS] = DrawRoomWarps(m_roomnum);
            ForceRedraw();
        }
    }
}

int RoomViewerCtrl::GetTotalWarps() const
{
    return m_warps.size();
}

bool RoomViewerCtrl::IsWarpSelected() const
{
    return (m_selected > WARP_IDX_OFFSET && m_selected <= static_cast<int>(m_warps.size() + WARP_IDX_OFFSET));
}

const WarpList::Warp& RoomViewerCtrl::GetSelectedWarp() const
{
    assert(IsWarpSelected());
    int selection = m_selected - WARP_IDX_OFFSET - 1;
    auto it = m_warps.cbegin();
    std::advance(it, selection);
    return *it;
}

int RoomViewerCtrl::GetSelectedWarpIndex() const
{
    if (IsWarpSelected())
    {
        return m_selected - WARP_IDX_OFFSET;
    }
    else
    {
        return NO_SELECTION;
    }
}

void RoomViewerCtrl::SetZoom(double zoom)
{
    m_zoom = zoom;
    RefreshRoom(m_roomnum);
}

void RoomViewerCtrl::SetLayerOpacity(Layer layer, uint8_t opacity)
{
    assert(m_layer_opacity.find(layer) != m_layer_opacity.cend());
    m_layer_opacity[layer] = opacity;
}

uint8_t RoomViewerCtrl::GetLayerOpacity(Layer layer) const
{
    assert(m_layer_opacity.find(layer) != m_layer_opacity.cend());
    return m_layer_opacity.find(layer)->second;
}

void RoomViewerCtrl::RedrawRoom()
{
    DrawRoom(m_roomnum);
    ForceRedraw();
}

void RoomViewerCtrl::DrawRoom(uint16_t roomnum)
{
    if (m_g == nullptr)
    {
        return;
    }
    auto map = m_g->GetRoomData()->GetMapForRoom(roomnum)->GetData();
    auto blocksets = m_g->GetRoomData()->GetBlocksetsForRoom(roomnum);
    auto tileset = m_g->GetRoomData()->GetTilesetForRoom(roomnum)->GetData();
    auto blockset = m_g->GetRoomData()->GetCombinedBlocksetForRoom(roomnum);
    m_swaps = m_g->GetRoomData()->GetTileSwaps(roomnum);
    m_doors = m_g->GetRoomData()->GetDoors(roomnum);
    m_rpalette = PreparePalettes(roomnum);

    m_warp_poly.clear();
    m_link_poly.clear();
    m_entity_poly.clear();
    m_layers.clear();
    m_preview_swaps.clear();
    m_preview_doors.clear();
    m_width = map->GetPixelWidth();
    m_height = map->GetPixelHeight();
    UpdateBuffer();

    if (m_layer_opacity[Layer::BACKGROUND1] > 0)
    {
        m_layer_bufs[Layer::BACKGROUND1]->Resize(m_width, m_height);
        m_layer_bufs[Layer::BACKGROUND1]->Insert3DMapLayer(0, 0, 0, Tilemap3D::Layer::BG, map, tileset, blockset);
        UpdateLayer(Layer::BACKGROUND1, m_layer_bufs[Layer::BACKGROUND1]->MakeImage(
            m_rpalette, true, m_layer_opacity[Layer::BACKGROUND1]));
    }
    if (m_layer_opacity[Layer::BACKGROUND2] > 0)
    {
        m_layer_bufs[Layer::BACKGROUND2]->Resize(m_width, m_height);
        m_layer_bufs[Layer::BACKGROUND2]->Insert3DMapLayer(0, 0, 0, Tilemap3D::Layer::FG, map, tileset, blockset);
        UpdateLayer(Layer::BACKGROUND2, m_layer_bufs[Layer::BACKGROUND2]->MakeImage(
            m_rpalette, true, m_layer_opacity[Layer::BACKGROUND2]));
    }
    if (m_layer_opacity[Layer::FOREGROUND] > 0)
    {
        m_layer_bufs[Layer::FOREGROUND]->Resize(m_width, m_height);
        m_layer_bufs[Layer::FOREGROUND]->Insert3DMapLayer(0, 0, 0, Tilemap3D::Layer::FG, map, tileset, blockset);
        UpdateLayer(Layer::FOREGROUND, m_layer_bufs[Layer::FOREGROUND]->MakeImage(
            m_rpalette, true, m_layer_opacity[Layer::FOREGROUND]));
    }
    if (m_layer_opacity[Layer::HEIGHTMAP] > 0)
    {
        UpdateLayer(Layer::HEIGHTMAP, DrawHeightmapVisualisation(map, m_layer_opacity[Layer::HEIGHTMAP]));
    }
    if (m_show_swaps)
    {
        UpdateLayer(Layer::SWAPS, DrawRoomSwaps(m_roomnum));
    }
    if (m_show_warps)
    {
        UpdateLayer(Layer::WARPS, DrawRoomWarps(m_roomnum));
    }
    if (m_show_entities || m_show_entity_hitboxes)
    {
        RedrawAllSprites();
    }
    auto mp = wxGetMousePosition();
    std::string s;
    CheckMousePosForLink({ mp.x, mp.y }, s);
    SetCursor(m_selected == NO_SELECTION ? wxCURSOR_ARROW : wxCURSOR_HAND);
}

void RoomViewerCtrl::RefreshRoom(bool redraw_tiles)
{
    if (m_g == nullptr)
    {
        return;
    }
    auto map = m_g->GetRoomData()->GetMapForRoom(m_roomnum)->GetData();
    UpdateBuffer();
    auto q = PrepareSprites(m_roomnum);
    if (redraw_tiles)
    {
        auto blocksets = m_g->GetRoomData()->GetBlocksetsForRoom(m_roomnum);
        auto tileset = m_g->GetRoomData()->GetTilesetForRoom(m_roomnum)->GetData();
        auto blockset = m_g->GetRoomData()->GetCombinedBlocksetForRoom(m_roomnum);
        auto pswaps = GetPreviewSwaps();
        auto pdoors = GetPreviewDoors();
        if (m_layer_opacity[Layer::BACKGROUND1] > 0)
        {
            m_layer_bufs[Layer::BACKGROUND1]->Resize(m_width, m_height);
            m_layer_bufs[Layer::BACKGROUND1]->Insert3DMapLayer(0, 0, 0, Tilemap3D::Layer::BG, map, tileset, blockset, true,
                                                               pswaps, pdoors);
            UpdateLayer(Layer::BACKGROUND1, m_layer_bufs[Layer::BACKGROUND1]->MakeImage(
                m_rpalette, true, m_layer_opacity[Layer::BACKGROUND1]));
        }
        if (m_layer_opacity[Layer::BACKGROUND2] > 0)
        {
            m_layer_bufs[Layer::BACKGROUND2]->Resize(m_width, m_height);
            m_layer_bufs[Layer::BACKGROUND2]->Insert3DMapLayer(0, 0, 0, Tilemap3D::Layer::FG, map, tileset, blockset, true,
                                                               pswaps, pdoors);
            UpdateLayer(Layer::BACKGROUND2, m_layer_bufs[Layer::BACKGROUND2]->MakeImage(
                m_rpalette, true, m_layer_opacity[Layer::BACKGROUND2]));
        }
        if (m_layer_opacity[Layer::FOREGROUND] > 0)
        {
            m_layer_bufs[Layer::FOREGROUND]->Resize(m_width, m_height);
            m_layer_bufs[Layer::FOREGROUND]->Insert3DMapLayer(0, 0, 0, Tilemap3D::Layer::FG, map, tileset, blockset, true,
                                                              pswaps, pdoors);
            UpdateLayer(Layer::FOREGROUND, m_layer_bufs[Layer::FOREGROUND]->MakeImage(
                m_rpalette, true, m_layer_opacity[Layer::FOREGROUND]));
        }
    }
    if (m_layer_opacity[Layer::HEIGHTMAP] > 0)
    {
        m_layers[Layer::HEIGHTMAP] = DrawHeightmapVisualisation(map, m_layer_opacity[Layer::HEIGHTMAP]);
    }
    if (m_show_warps)
    {
        m_layers[Layer::WARPS] = DrawRoomWarps(m_roomnum);
    }
    if (m_show_swaps)
    {
        m_layers[Layer::SWAPS] = DrawRoomSwaps(m_roomnum);
    }
    DrawSpriteHitboxes(q);
    UpdateScroll();
    ForceRedraw();
}

void RoomViewerCtrl::RedrawAllSprites()
{
    m_rpalette = PreparePalettes(m_roomnum);
    auto q = PrepareSprites(m_roomnum);
    m_entity_poly.clear();
    if (m_show_entities)
    {
        m_layer_bufs[Layer::FG_SPRITES]->Resize(m_width, m_height);
        m_layer_bufs[Layer::BG_SPRITES]->Resize(m_width, m_height);
        DrawSprites(q);
        if (m_layer_opacity[Layer::BG_SPRITES] > 0)
        {
            UpdateLayer(Layer::BG_SPRITES, m_layer_bufs[Layer::BG_SPRITES]->MakeImage(m_rpalette,
                true, m_layer_opacity[Layer::BG_SPRITES], m_layer_opacity[Layer::BG_SPRITES]));
        }
        if (m_layer_opacity[Layer::FG_SPRITES] > 0)
        {
            UpdateLayer(Layer::FG_SPRITES, m_layer_bufs[Layer::FG_SPRITES]->MakeImage(m_rpalette,
                true, m_layer_opacity[Layer::FG_SPRITES], m_layer_opacity[Layer::FG_SPRITES]));
        }
    }
    if (m_show_entities || m_show_entity_hitboxes)
    {
        DrawSpriteHitboxes(q);
        AddEntityClickRegions(q);
    }
}

void RoomViewerCtrl::UpdateLayer(const Layer& layer, std::unique_ptr<wxBitmap> bmp)
{
    if (m_layers.find(layer) == m_layers.cend())
    {
        m_layers.insert({ layer, nullptr });
    }
    m_layers[layer] = std::move(bmp);
}

void RoomViewerCtrl::UpdateLayer(const Layer& layer, const wxImage& img)
{
    if (m_layers.find(layer) == m_layers.cend())
    {
        m_layers.insert({ layer, std::make_unique<wxBitmap>(img)});
    }
    else
    {
        m_layers[layer] = std::make_unique<wxBitmap>(img);
    }
}

void RoomViewerCtrl::RefreshStatusbar()
{
    auto rd = m_g->GetRoomData()->GetRoom(m_roomnum);
    std::ostringstream ss;
    std::string txt;
    ss << "Room: " << std::dec << std::uppercase << std::setw(3) << std::setfill('0') << rd->index
        << " Tileset: 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(rd->tileset)
        << " Palette: 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(rd->room_palette)
        << " PriBlockset: 0x" << std::hex << std::uppercase << std::setw(1) << std::setfill('0') << static_cast<unsigned>(rd->pri_blockset)
        << " SecBlockset: 0x" << std::hex << std::uppercase << std::setw(1) << std::setfill('0') << static_cast<unsigned>(rd->sec_blockset)
        << " BGM: 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(rd->bgm)
        << " Map: " << rd->map;
    FireUpdateStatusEvent(ss.str(), 0);
    if (IsEntitySelected())
    {
        auto& entity = GetSelectedEntity();
        int idx = GetSelectedEntityIndex();
        txt = StrPrintf("Selected Entity %d (%04.1f, %04.1f, %04.1f) - %s",
            idx, entity.GetXDbl(), entity.GetYDbl(), entity.GetZDbl(), entity.GetTypeName().c_str());
        FireUpdateStatusEvent(txt, 1);
    }
    else if (IsWarpSelected())
    {
        auto& warp = GetSelectedWarp();
        int idx = GetSelectedWarpIndex();
        txt = StrPrintf("Selected Warp %d (%02d, %02d), (%01dx%01d %s) -> %03d (%s)",
                        idx, m_roomnum == warp.room1 ? warp.x1 : warp.x2,
                        m_roomnum == warp.room1 ? warp.y1 : warp.y2, warp.x_size, warp.y_size,
                        (warp.type == WarpList::Warp::Type::NORMAL ? "NML" :
                        (warp.type == WarpList::Warp::Type::STAIR_SE ? "SSE" :
                        (warp.type == WarpList::Warp::Type::STAIR_SW ? "SSW" : "UNK"))),
                        m_roomnum == warp.room1 ? warp.room2 : warp.room1,
                        m_g ? m_g->GetRoomData()->GetRoom(m_roomnum == warp.room1 ? warp.room2 : warp.room1)->name.c_str() : "?");
    }
    else if (IsTileSwapSelected())
    {
        const TileSwap& swap = GetSelectedTileSwap();
        int idx = GetSelectedTileSwapIndex();
        txt = StrPrintf("Selected Tile Swap %d %s HM (%02d,%02d) -> (%02d,%02d) (%02dx%02d) Map (%02d,%02d) -> (%02d,%02d) (%02dx%02d)",
            idx, swap.mode == TileSwap::Mode::FLOOR ? "Floor" : swap.mode == TileSwap::Mode::WALL_NE ? "NE Wall" : swap.mode == TileSwap::Mode::WALL_NW ? "NW Wall" : "?",
            swap.heightmap.src_x, swap.heightmap.src_y, swap.heightmap.dst_x, swap.heightmap.dst_y, swap.heightmap.width, swap.heightmap.height,
            swap.map.src_x, swap.map.src_y, swap.map.dst_x, swap.map.dst_y, swap.map.width, swap.map.height);
    }
    else if (IsDoorSelected())
    {
        const Door& door = GetSelectedDoor();
        int idx = GetSelectedDoorIndex();
        txt = StrPrintf("Selected Door %d (%02d,%02d) (%02dx%02d)",
                        idx, door.x, door.y, door.SIZE_NAMES.count(door.size) ? door.SIZE_NAMES.at(door.size).c_str() : "?");
    }
    FireUpdateStatusEvent(txt, 1);
    if (GetErrorCount() > 0)
    {
        FireUpdateStatusEvent(StrPrintf("Total Errors: %d, Error #1 : %s",
            GetErrorCount(), GetErrorText(0).c_str()), 2);
    }
    else
    {
        FireUpdateStatusEvent("", 2);
    }
}

std::vector<std::shared_ptr<Palette>> RoomViewerCtrl::PreparePalettes(uint16_t roomnum)
{
    auto palette = std::vector<std::shared_ptr<Palette>>{ m_g->GetRoomData()->GetPaletteForRoom(roomnum)->GetData() };
    palette.emplace_back();
    palette.emplace_back(m_g->GetGraphicsData()->GetPlayerPalette()->GetData());
    palette.emplace_back(m_g->GetGraphicsData()->GetHudPalette()->GetData());
    m_errors.clear();
    std::array<int, 3> sprite_palette_alloc = { -1, -1, -1 };
    m_layer_bufs[Layer::FG_SPRITES]->Resize(m_width, m_height);
    for (const auto& entity : m_entities)
    {
        auto s_pal = m_g->GetSpriteData()->GetSpritePaletteIdxs(entity.GetType());
        const uint8_t pal_slot = entity.GetPalette();
        if (pal_slot == 1 || pal_slot == 3)
        {
            const int lo = s_pal.first;
            const int hi = s_pal.second;
            if (lo != -1)
            {
                if (sprite_palette_alloc[pal_slot - 1] == -1)
                {
                    sprite_palette_alloc[pal_slot - 1] = lo;
                }
                else if (sprite_palette_alloc[pal_slot - 1] != lo)
                {
                    m_errors.push_back(StrPrintf("Possible Palette Clash - Slot%d Lo orig %02X, req %02X.",
                        pal_slot, sprite_palette_alloc[pal_slot - 1], lo));
                }
            }
            if (hi != -1) // Hi Palette
            {
                if (pal_slot == 3)
                {
                    m_errors.push_back(StrPrintf("Possible Palette Clash - Slot%d Hi specified, req %02X.",
                            pal_slot, hi));
                }
                else
                {
                    if (sprite_palette_alloc[pal_slot] == -1)
                    {
                        sprite_palette_alloc[pal_slot] = hi;
                    }
                    else if (sprite_palette_alloc[pal_slot] != hi)
                    {
                        m_errors.push_back(StrPrintf("Possible Palette Clash - Slot%d Hi orig %02X, req %02X.",
                            pal_slot, sprite_palette_alloc[pal_slot], hi));
                    }
                }
            }
        }
    }
    palette[3] = std::make_shared<Palette>(std::vector<std::shared_ptr<Palette>>{ palette[3], m_g->GetSpriteData()->GetSpritePalette(sprite_palette_alloc[2], -1) });
    palette[1] = m_g->GetSpriteData()->GetSpritePalette(sprite_palette_alloc[0], sprite_palette_alloc[1]);
    return palette;
}

std::vector<RoomViewerCtrl::SpriteQ> RoomViewerCtrl::PrepareSprites(uint16_t roomnum)
{
    auto map = m_g->GetRoomData()->GetMapForRoom(roomnum)->GetData();
    std::vector<SpriteQ> sprites;
    int i = 1;
    for (const auto& entity : m_entities)
    {
        SpriteQ s;
        if (m_g->GetSpriteData()->HasFrontAndBack(entity.GetType()))
        {
            auto sprite = m_g->GetSpriteData()->GetSpriteFromEntity(entity.GetType());
            int anim = 0;
            if ((entity.GetOrientation() == Orientation::SW || entity.GetOrientation() == Orientation::SE))
            {
                anim = 1;
            }
            auto frame = m_g->GetSpriteData()->GetSpriteFrame(sprite, anim, 0);
            s.frame = frame->GetData();
        }
        else
        {
            auto frame = m_g->GetSpriteData()->GetDefaultEntityFrame(entity.GetType());
            s.frame = frame->GetData();
        }
        s.id = i;
        s.entity = entity;
        s.palette = entity.GetPalette();
        s.hflip = (entity.GetOrientation() == Orientation::NW || entity.GetOrientation() == Orientation::SE);
        s.background = false;
        s.selected = (i == m_selected);

        auto hitbox = m_g->GetSpriteData()->GetEntityHitbox(entity.GetType());

        int x = entity.GetX() + 0x080;
        int y = entity.GetY() - 0x080;
        int z = entity.GetZ();

        if (hitbox.first >= 0x0C)
        {
            x += 0x80;
            y += 0x80;
        }

        auto xy = map->EntityPositionToPixel(x, y, z);
        s.x = xy.x;
        s.y = xy.y;
        sprites.push_back(s);
        i++;
    }
    // Fix draw order
    std::sort(sprites.begin(), sprites.end(), [this](const SpriteQ& lhs, const SpriteQ& rhs) -> bool
        {
            if (lhs.selected || rhs.selected)
            {
                return rhs.selected;
            }
            // Draw objects furthest away from camera first
            auto hitbox_lhs = m_g->GetSpriteData()->GetEntityHitbox(lhs.entity.GetType());
            auto hitbox_rhs = m_g->GetSpriteData()->GetEntityHitbox(rhs.entity.GetType());
            int dist_lhs = lhs.entity.GetX() + lhs.entity.GetY() + hitbox_lhs.first;
            int dist_rhs = rhs.entity.GetX() + rhs.entity.GetY() + hitbox_rhs.first;
            if (dist_lhs != dist_rhs)
            {
                return dist_lhs < dist_rhs;
            }
            // Next draw left-most objects
            int left_lhs = lhs.entity.GetY() - hitbox_lhs.first;
            int left_rhs = rhs.entity.GetY() - hitbox_rhs.first;
            if (left_lhs != left_rhs)
            {
                return left_lhs < left_rhs;
            }
            // Finally, sort by height
            int height_lhs = lhs.entity.GetZ() + hitbox_lhs.second;
            int height_rhs = rhs.entity.GetZ() + hitbox_rhs.second;
            return height_lhs < height_rhs;
        });
    return sprites;
}

void RoomViewerCtrl::DrawSpriteHitboxes(const std::vector<SpriteQ>& q)
{
    std::unordered_map<Layer, wxImage> layers;
    std::unordered_map<Layer, wxGraphicsContext*> ctxs;
    layers.insert({ Layer::BG_SPRITES_WIREFRAME_BG, wxImage(m_buffer_width, m_buffer_height) });
    layers.insert({ Layer::BG_SPRITES_WIREFRAME_FG, wxImage(m_buffer_width, m_buffer_height) });
    layers.insert({ Layer::FG_SPRITES_WIREFRAME_BG, wxImage(m_buffer_width, m_buffer_height) });
    layers.insert({ Layer::FG_SPRITES_WIREFRAME_FG, wxImage(m_buffer_width, m_buffer_height) });
    
    
    for (auto& img : layers)
    {
        img.second.InitAlpha();
        SetOpacity(img.second, 0x00);
        auto ctx = wxGraphicsContext::Create(img.second);
        ctx->Scale(m_zoom, m_zoom);
        ctx->SetPen(*wxTRANSPARENT_PEN);
        ctx->SetBrush(*wxBLACK_BRUSH);
        ctxs.insert({ img.first, ctx });
    }
    for (const auto& s : q)
    {
        auto hitbox = m_g->GetSpriteData()->GetEntityHitbox(s.entity.GetType());

        /* wxPoint2DDouble shadow_points[] = {
            wxPoint2DDouble(s.x + hitbox.first * 2, s.y),
            wxPoint2DDouble(s.x, s.y + hitbox.first),
            wxPoint2DDouble(s.x - hitbox.first * 2, s.y),
            wxPoint2DDouble(s.x, s.y - hitbox.first),
            wxPoint2DDouble(s.x + hitbox.first * 2, s.y)
        }; */
        wxPoint2DDouble hitbox_fg_points[] = {
            wxPoint2DDouble(s.x + hitbox.first * 2, s.y),
            wxPoint2DDouble(s.x, s.y + hitbox.first),
            wxPoint2DDouble(s.x - hitbox.first * 2, s.y),
            wxPoint2DDouble(s.x - hitbox.first * 2, s.y - hitbox.second),
            wxPoint2DDouble(s.x, s.y + hitbox.first - hitbox.second),
            wxPoint2DDouble(s.x, s.y + hitbox.first),
            wxPoint2DDouble(s.x, s.y + hitbox.first - hitbox.second),
            wxPoint2DDouble(s.x + hitbox.first * 2, s.y - hitbox.second),
            wxPoint2DDouble(s.x + hitbox.first * 2, s.y),
            wxPoint2DDouble(s.x + hitbox.first * 2, s.y - hitbox.second),
            wxPoint2DDouble(s.x, s.y - hitbox.first - hitbox.second),
            wxPoint2DDouble(s.x - hitbox.first * 2, s.y - hitbox.second)
        };

        auto fg_ctx = s.background ? ctxs[Layer::BG_SPRITES_WIREFRAME_FG] : ctxs[Layer::FG_SPRITES_WIREFRAME_FG];
        auto bg_ctx = s.background ? ctxs[Layer::BG_SPRITES_WIREFRAME_BG] : ctxs[Layer::FG_SPRITES_WIREFRAME_BG];
        if (s.selected || m_show_entity_hitboxes)
        {
            auto dotted_pen = bg_ctx->CreatePen(wxGraphicsPenInfo(s.selected ? wxColor(0xFF, 50, 50) : wxColor(0xA0, 0xA0, 0xA0)).Style(wxPENSTYLE_DOT));
            bg_ctx->SetPen(dotted_pen);
            bg_ctx->StrokeLine(s.x + hitbox.first * 2, s.y, s.x, s.y - hitbox.first);
            bg_ctx->StrokeLine(s.x, s.y - hitbox.first, s.x - hitbox.first * 2, s.y);
            bg_ctx->StrokeLine(s.x, s.y - hitbox.first, s.x, s.y - hitbox.first - hitbox.second);
            auto solid_pen = bg_ctx->CreatePen(wxGraphicsPenInfo(s.selected ? wxColor(0xFF, 50, 50) : wxColor(0xA0, 0xA0, 0xA0)).Style(wxPENSTYLE_SOLID));
            fg_ctx->SetPen(solid_pen);
            fg_ctx->StrokeLines(sizeof(hitbox_fg_points) / sizeof(hitbox_fg_points[0]), hitbox_fg_points);
        }
    }
    for (auto& ctx : ctxs)
    {
        delete ctx.second;
    }
    ctxs.clear();
    for (auto& layer : layers)
    {
        UpdateLayer(layer.first, layer.second);
    }
}

void RoomViewerCtrl::DrawSprites(const std::vector<SpriteQ>& q)
{
    for (const auto& s : q)
    {
        if (s.frame != nullptr)
        {
            if (s.background)
            {
                m_layer_bufs[Layer::BG_SPRITES]->InsertSprite(s.x, s.y, s.palette, *s.frame, s.hflip);
            }
            else
            {
                m_layer_bufs[Layer::FG_SPRITES]->InsertSprite(s.x, s.y, s.palette, *s.frame, s.hflip);
            }
        }
    }
}

void RoomViewerCtrl::UpdateRoomDescText(uint16_t /*roomnum*/)
{
    if (m_g == nullptr)
    {
        return;
    }
    RefreshStatusbar();
}

std::unique_ptr<wxBitmap> RoomViewerCtrl::DrawRoomWarps(uint16_t roomnum)
{
    m_warp_poly.clear();
    m_link_poly.clear();
    auto map = m_g->GetRoomData()->GetMapForRoom(roomnum)->GetData();
    auto warps = m_g->GetRoomData()->GetWarpsForRoom(roomnum);

    wxImage img(m_buffer_width, m_buffer_height);
    img.InitAlpha();
    SetOpacity(img, 0x00);
    wxGraphicsContext* gc = wxGraphicsContext::Create(img);
    gc->Scale(m_zoom, m_zoom);
    gc->SetPen(*wxWHITE_PEN);
    gc->SetBrush(*wxBLACK_BRUSH);
    for (std::size_t i = 0; i < m_warps.size(); ++i)
    {
        DrawWarp(*gc, i, map, TILE_WIDTH, TILE_HEIGHT, false);
    }
    wxColour bkColor(*wxBLACK);
    wxColour textColor(*wxWHITE);
    int line = 0;
    if (m_g->GetRoomData()->HasClimbDestination(roomnum))
    {
        AddRoomLink(gc, "Climb Destination:", m_g->GetRoomData()->GetClimbDestination(roomnum), 5, 5 + line * 16);
        line++;
    }
    if (m_g->GetRoomData()->HasFallDestination(roomnum))
    {
        AddRoomLink(gc, "Fall Destination:", m_g->GetRoomData()->GetFallDestination(roomnum), 5, 5 + line * 16);
        line++;
    }
    auto txns = m_g->GetRoomData()->GetTransitions(roomnum);
    for (const auto& t : txns)
    {
        std::string label = StrPrintf("Transition when flag %04d is %s:", t.flag, (t.src_rm == roomnum) ? "SET" : "CLEAR");
        uint16_t dest = (t.src_rm == roomnum) ? t.dst_rm : t.src_rm;
        AddRoomLink(gc, label, dest, 5, 5 + line * 16);
        line++;
    }
    delete gc;
    return std::make_unique<wxBitmap>(img);
}

void RoomViewerCtrl::UpdateWarpProperties(int warp)
{
    if (warp > 0 && warp <= static_cast<int>(m_warps.size()))
    {
        bool pend = m_is_warp_pending && !m_warps[warp - 1].IsValid();
        WarpPropertyWindow dlg(m_frame, m_roomnum, warp, &m_warps[warp - 1], *m_g);
        if (dlg.ShowModal() == wxID_OK)
        {
            m_g->GetRoomData()->SetWarpsForRoom(m_roomnum, m_warps);
            if (pend && m_warps[warp-1].IsValid())
            {
                m_is_warp_pending = false;
            }
            RefreshStatusbar();
            FireEvent(EVT_WARP_UPDATE);
            UpdateLayer(Layer::WARPS, DrawRoomWarps(m_roomnum));
            ForceRedraw();
        }
    }
}

std::unique_ptr<wxBitmap> RoomViewerCtrl::DrawHeightmapVisualisation(std::shared_ptr<Tilemap3D> map, uint8_t opacity)
{
    wxImage hm_img(m_buffer_width, m_buffer_height);
    hm_img.InitAlpha();
    SetOpacity(hm_img, 0x00);
    wxGraphicsContext* hm_gc = wxGraphicsContext::Create(hm_img);
    hm_gc->Scale(m_zoom, m_zoom);
    hm_gc->SetPen(*wxWHITE_PEN);
    hm_gc->SetBrush(*wxBLACK_BRUSH);
    auto preview_swaps = GetPreviewSwaps();
    for (int y = 0; y < map->GetHeightmapHeight(); ++y)
    {
        for (int x = 0; x < map->GetHeightmapWidth(); ++x)
        {
            // Only display cells that are not completely restricted
            wxColour wall_colour = *wxWHITE;
            bool draw_outline = false;
            for (const auto& door : m_g->GetRoomData()->GetDoors(m_roomnum))
            {
                if ((x >= door.x && x < door.x + Door::SIZES.at(door.size).first) &&
                    (y >= door.y && y < door.y + Door::SIZES.at(door.size).second))
                {
                    wall_colour = wxColor(255, 0, 255);
                    draw_outline = true;
                }
            }
            for (const auto& swap : m_g->GetRoomData()->GetTileSwaps(m_roomnum))
            {
                if ((x >= swap.heightmap.src_x && x < swap.heightmap.src_x + swap.heightmap.width) &&
                    (y >= swap.heightmap.src_y && y < swap.heightmap.src_y + swap.heightmap.height))
                {
                    wall_colour = *wxGREEN;
                    draw_outline = true;
                }
                if ((x >= swap.heightmap.dst_x && x < swap.heightmap.dst_x + swap.heightmap.width) &&
                    (y >= swap.heightmap.dst_y && y < swap.heightmap.dst_y + swap.heightmap.height))
                {
                    wall_colour = wxColor(0, 255, 255);
                    draw_outline = true;
                }
            }
            if ((map->GetHeight({ x, y }) > 0 || (map->GetCellProps({ x, y }) != 0x04)) || draw_outline)
            {
                HMPoint2D p = { x, y };
                for (const auto& s : preview_swaps)
                {
                    int rx = x - s.heightmap.dst_x;
                    int ry = y - s.heightmap.dst_y;
                    if (rx >= 0 && rx < s.heightmap.width && ry >= 0 && ry < s.heightmap.height)
                    {
                        p = { rx + s.heightmap.src_x, ry + s.heightmap.src_y };
                    }
                }
                int z = map->GetHeight(p);
                auto xy(map->Iso3DToPixel({ x + 12, y + 12, z }));
                DrawHeightmapCell(*hm_gc, xy.x, xy.y, z, TILE_WIDTH, TILE_HEIGHT,
                    map->GetCellProps(p), map->GetCellType(p), draw_outline, wall_colour);
            }
        }
    }
    delete hm_gc;
    SetOpacity(hm_img, opacity);
    return std::make_unique<wxBitmap>(hm_img);
}

void RoomViewerCtrl::DrawHeightmapCell(wxGraphicsContext& gc, int x, int y, int zz, int width, int height, int restrictions, int classification, bool draw_walls, wxColor border_colour)
{
    int z = zz;// draw_walls ? zz : 0;
    wxPoint2DDouble tile_points[] = {
        wxPoint2DDouble(x + width / 2, y),
        wxPoint2DDouble(x + width    , y + height / 2),
        wxPoint2DDouble(x + width / 2, y + height),
        wxPoint2DDouble(x            , y + height / 2),
        wxPoint2DDouble(x + width / 2, y)
    };
    wxPoint2DDouble tile_highlight[] = {
        wxPoint2DDouble(x + width / 2, y + 1),
        wxPoint2DDouble(x + width - 2, y + height / 2),
        wxPoint2DDouble(x + width / 2, y + height - 1),
        wxPoint2DDouble(x         + 2, y + height / 2),
        wxPoint2DDouble(x + width / 2, y + 1)
    };
    wxPoint2DDouble left_wall[] = {
        wxPoint2DDouble(x            , y + height / 2),
        wxPoint2DDouble(x + width / 2, y + height),
        wxPoint2DDouble(x + width / 2, y + height + z * height),
        wxPoint2DDouble(x            , y + height / 2 + z * height),
        wxPoint2DDouble(x            , y + height / 2)
    };
    wxPoint2DDouble right_wall[] = {
        wxPoint2DDouble(x + width / 2, y + height),
        wxPoint2DDouble(x + width    , y + height / 2),
        wxPoint2DDouble(x + width    , y + height / 2 + z * height),
        wxPoint2DDouble(x + width / 2, y + height + z * height),
        wxPoint2DDouble(x + width / 2, y + height)
    };
    wxColor bg;
    switch(restrictions)
    {
    case -1:
        bg = *wxBLACK;
        break;
    case 0x0:
        bg.Set(zz * 3, 48 + zz * 8, zz * 3);
        break;
    case 0x4:
        bg.Set(48 + zz * 8, zz * 3, zz * 3);
        break;
    case 0x2:
        bg.Set(32 + zz * 3, 32 + zz * 3, 48 + zz * 12);
        break;
    case 0x6:
        bg.Set(48 + zz * 8, 32 + zz * 3, 48 + zz * 8);
        break;
    default:
        bg.Set(48 + zz * 8, 48 + zz * 8, zz * 3);
        break;
    }
    gc.SetPen(*wxWHITE);
    gc.SetBrush(wxBrush(bg));
    //gc.SetTextForeground(*wxWHITE);
    gc.DrawLines(sizeof(tile_points) / sizeof(tile_points[0]), tile_points);
    if (draw_walls)
    {
        gc.SetPen(wxPen(border_colour, 2));
        gc.DrawLines(sizeof(tile_highlight) / sizeof(tile_highlight[0]), tile_highlight);
        gc.SetPen(*wxWHITE);
    }
    bg = bg.ChangeLightness(70);
    gc.SetBrush(wxBrush(bg));
    gc.DrawLines(sizeof(left_wall) / sizeof(left_wall[0]), left_wall);
    bg = bg.ChangeLightness(70);
    gc.SetBrush(wxBrush(bg));
    gc.DrawLines(sizeof(right_wall) / sizeof(right_wall[0]), right_wall);
    if (classification > 0)
    {
        // Set font and transparent colour
        gc.SetFont(wxFont(height / 2, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
            wxFONTWEIGHT_NORMAL, false),
            wxColour(255, 255, 255, 255));
        std::ostringstream ss;
        ss << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << classification;
        double textwidth, textheight;
        gc.GetTextExtent(ss.str(), &textwidth, &textheight);
        gc.DrawText(ss.str(), x + (width - textwidth) / 2, y + (height - textheight) / 2);
    }
}

void RoomViewerCtrl::DrawTileSwaps(wxGraphicsContext& gc, uint16_t roomnum)
{
    m_swap_regions.clear();
    if (!m_g)
    {
        return;
    }
    auto map = m_g->GetRoomData()->GetMapForRoom(roomnum)->GetData();
    auto swaps = m_g->GetRoomData()->GetTileSwaps(roomnum);
    int si = SWAP_IDX_OFFSET;
    gc.SetBrush(*wxTRANSPARENT_BRUSH);
    for (const auto& s : swaps)
    {
        std::pair<int, int> src, dst;
        auto lines = s.GetMapRegionPoly(TileSwap::Region::UNDEFINED, CELL_WIDTH, CELL_HEIGHT);
        std::vector<wxPoint2DDouble> wxpoints = ToWxPoints2DDouble(lines);
        auto sp = GetScreenPosition(s.GetTileOffset(TileSwap::Region::SOURCE, map, Tilemap3D::Layer::FG), roomnum, Tilemap3D::Layer::FG);
        auto dp = GetScreenPosition(s.GetTileOffset(TileSwap::Region::DESTINATION, map, Tilemap3D::Layer::FG), roomnum, Tilemap3D::Layer::FG);
        m_swap_regions.emplace_back(
            ToWxPoints2DDouble(TileSwap::OffsetRegionPoly(lines, sp)),
            ToWxPoints2DDouble(TileSwap::OffsetRegionPoly(lines, dp)));
    }
    auto draw_poly = [this, &gc](int index, const std::pair<std::vector<wxPoint2DDouble>, std::vector<wxPoint2DDouble>>& points)
        {
            bool hovered = (m_hovered == index);
            bool selected = (m_selected == index);
            bool preview = (std::find(m_preview_swaps.cbegin(), m_preview_swaps.cend(), index - SWAP_IDX_OFFSET) != m_preview_swaps.cend());
            gc.SetPen(wxPen(hovered ? wxColor(128, 128, 255) : *wxBLUE, selected ? 3 : 2, preview ? wxPENSTYLE_SOLID : wxPENSTYLE_SHORT_DASH));
            gc.DrawLines(points.first.size(), points.first.data());
            gc.SetPen(wxPen(hovered ? wxColor(255, 128, 128) : *wxRED, selected ? 3 : 2, preview ? wxPENSTYLE_SOLID : wxPENSTYLE_SHORT_DASH));
            gc.DrawLines(points.second.size(), points.second.data());
        };
    int hovered_poly = NO_SELECTION;
    int selected_poly = NO_SELECTION;
    for (const auto& r : m_swap_regions)
    {
        ++si;
        bool hovered = (m_hovered == si);
        bool selected = (m_selected == si);
        if (hovered)
        {
            hovered_poly = si;
        }
        if (selected)
        {
            selected_poly = si;
        }
        if (!hovered && !selected)
        {
            draw_poly(si, r);
        }
    }
    if (selected_poly != NO_SELECTION)
    {
        draw_poly(selected_poly, m_swap_regions[selected_poly - SWAP_IDX_OFFSET - 1]);
    }
    if (hovered_poly != NO_SELECTION)
    {
        draw_poly(hovered_poly, m_swap_regions[hovered_poly - SWAP_IDX_OFFSET - 1]);
    }
}

void RoomViewerCtrl::DrawDoors(wxGraphicsContext& gc, uint16_t roomnum)
{
    m_door_regions.clear();
    if (!m_g)
    {
        return;
    }
    auto map = m_g->GetRoomData()->GetMapForRoom(roomnum)->GetData();
    auto doors = m_g->GetRoomData()->GetDoors(roomnum);
    int di = DOOR_IDX_OFFSET;
    for (const auto& d : doors)
    {
        if (d.x < map->GetHeightmapWidth() && d.y < map->GetHeightmapHeight())
        {
            auto lines = d.GetMapRegionPoly(map, CELL_WIDTH, CELL_HEIGHT);
            auto wxpoints = ToWxPoints2DDouble(lines);
            auto pos = GetScreenPosition(d.GetTileOffset(map, Tilemap3D::Layer::FG), roomnum, Tilemap3D::Layer::FG);
            m_door_regions.push_back(ToWxPoints2DDouble(Door::OffsetRegionPoly(map, lines, pos)));
        }
    }
    auto draw_poly = [this, &gc](int index, const std::vector<wxPoint2DDouble>& points)
        {
            bool hovered = (m_hovered == index);
            bool selected = (m_selected == index);
            bool preview = (std::find(m_preview_doors.cbegin(), m_preview_doors.cend(), index - DOOR_IDX_OFFSET) != m_preview_doors.cend());
            gc.SetPen(wxPen(hovered ? wxColor(255, 128, 255) : wxColor(255, 0, 255), selected ? 3 : 2, preview ? wxPENSTYLE_SOLID : wxPENSTYLE_SHORT_DASH));
            gc.DrawLines(points.size(), points.data());
        };
    int hovered_poly = NO_SELECTION;
    int selected_poly = NO_SELECTION;
    for (const auto& r : m_door_regions)
    {
        ++di;
        bool hovered = (m_hovered == di);
        bool selected = (m_selected == di);
        if (hovered)
        {
            hovered_poly = di;
        }
        if (selected)
        {
            selected_poly = di;
        }
        if (!hovered && !selected)
        {
            draw_poly(di, r);
        }
    }
    if (selected_poly != NO_SELECTION)
    {
        draw_poly(selected_poly, m_door_regions[selected_poly - DOOR_IDX_OFFSET - 1]);
    }
    if (hovered_poly != NO_SELECTION)
    {
        draw_poly(hovered_poly, m_door_regions[hovered_poly - DOOR_IDX_OFFSET - 1]);
    }
}

std::unique_ptr<wxBitmap> RoomViewerCtrl::DrawRoomSwaps(uint16_t roomnum)
{
    wxImage img = wxImage(m_buffer_width, m_buffer_height);
    img.InitAlpha();
    SetOpacity(img, 0x00);
    wxGraphicsContext* gc = wxGraphicsContext::Create(img);
    gc->Scale(m_zoom, m_zoom);
    gc->SetPen(*wxWHITE_PEN);
    gc->SetBrush(*wxBLACK_BRUSH);
    DrawTileSwaps(*gc, roomnum);
    DrawDoors(*gc, roomnum);
    delete gc;
    return std::make_unique<wxBitmap>(img);
}

std::vector<wxPoint2DDouble> RoomViewerCtrl::ToWxPoints2DDouble(const std::vector<std::pair<int, int>>& points)
{
    std::vector<wxPoint2DDouble> wxpoints;
    std::transform(points.cbegin(), points.cend(),
        std::back_inserter(wxpoints),
        [](const auto& p) {return wxPoint2DDouble(p.first, p.second); });
    return wxpoints;
}

std::pair<int, int> RoomViewerCtrl::GetScreenPosition(const std::pair<int, int>& iso_pos, uint16_t roomnum, Tilemap3D::Layer layer) const
{
    if (m_g)
    {
        auto map = m_g->GetRoomData()->GetMapForRoom(roomnum)->GetData();
        int xp = (map->GetHeight() - 1 + iso_pos.first - iso_pos.second) * CELL_WIDTH + (map->GetLeft()) * CELL_WIDTH / 2 + (layer == Tilemap3D::Layer::BG ? CELL_WIDTH : 0);
        int yp = (iso_pos.first + iso_pos.second + map->GetTop()) * CELL_HEIGHT / 2;
        return { xp, yp };
    }
    return {0, 0};
}

const std::vector<std::string>& RoomViewerCtrl::GetErrors() const
{
    return m_errors;
}

void RoomViewerCtrl::DrawWarp(wxGraphicsContext& gc, int index, std::shared_ptr<Tilemap3D> map, int tile_width, int tile_height, bool adjust_z)
{
    int x = 0;
    int y = 0;
    const auto& warp = m_warps.at(index);
    if (warp.room1 == m_roomnum)
    {
        x = warp.x1;
        y = warp.y1;
    }
    else if (warp.room2 == m_roomnum)
    {
        x = warp.x2;
        y = warp.y2;
    }
    PixelPoint2D xy{0,0};
    if (adjust_z == false)
    {
        int z = map->GetHeight({ x - 12,y - 12 });
        xy = map->Iso3DToPixel({ x,y, z });
    }
    else
    {
        xy.x = (x - y + map->GetHeightmapHeight() - 1) * TILE_WIDTH;
        xy.y = (x + y) * TILE_HEIGHT;
    }

    std::vector<wxPoint2DDouble> tile_points = {
        wxPoint2DDouble(xy.x + tile_width / 2, xy.y),
        wxPoint2DDouble(xy.x + (warp.x_size + 1) * tile_width / 2, xy.y + warp.x_size * tile_height / 2),
        wxPoint2DDouble(xy.x + (warp.x_size - warp.y_size + 1) * tile_width / 2, xy.y + (warp.x_size + warp.y_size) * tile_height / 2),
        wxPoint2DDouble(xy.x + (1 - warp.y_size) * tile_width / 2, xy.y + warp.y_size * tile_height / 2),
        wxPoint2DDouble(xy.x + tile_width / 2, xy.y)
    };
    gc.SetBrush(*wxTRANSPARENT_BRUSH);
    if (GetSelectedWarpIndex() == index + 1)
    {
        gc.SetPen(wxPen(*wxRED, 3));
        gc.DrawLines(tile_points.size(), &tile_points[0]);
    }
    switch (warp.type)
    {
    case WarpList::Warp::Type::NORMAL:
        gc.SetPen(*wxYELLOW_PEN);
        break;
    case WarpList::Warp::Type::STAIR_SE:
        gc.SetPen(*wxGREEN_PEN);
        break;
    case WarpList::Warp::Type::STAIR_SW:
        gc.SetPen(*wxCYAN_PEN);
        break;
    default:
        gc.SetPen(*wxRED_PEN);
        break;
    }
    
    if (!warp.IsValid())
    {
        gc.SetBrush(*m_warp_brush);
    }
    gc.DrawLines(tile_points.size(), &tile_points[0]);
    m_warp_poly.push_back({ index, tile_points });
}

void RoomViewerCtrl::AddWarp()
{
    if (m_is_warp_pending)
    {
        if (m_pending_warp.room1 != m_roomnum && m_pending_warp.room2 != m_roomnum)
        {
            if (m_pending_warp.room1 == 0xFFFF)
            {
                m_pending_warp.room1 = m_roomnum;
                m_pending_warp.x1 = 31;
                m_pending_warp.y1 = 31;
            }
            else
            {
                m_pending_warp.room2 = m_roomnum;
                m_pending_warp.x2 = 31;
                m_pending_warp.y2 = 31;
            }
            m_is_warp_pending = false;
            int idx = 1;
            for (auto it = m_warps.begin(); it != m_warps.end(); )
            {
                if (it->IsValid())
                {
                    idx++;
                    it++;
                }
                else
                {
                    *it = m_pending_warp;
                    m_is_warp_pending = false;
                    break;
                }
            }
            if (m_is_warp_pending)
            {
                m_warps.push_back(m_pending_warp);
                idx = m_warps.size();
                m_is_warp_pending = false;
            }
            m_g->GetRoomData()->SetWarpsForRoom(m_roomnum, m_warps);
            SelectWarp(idx);
        }
        else
        {
            wxMessageBox("There is already a pending warp in this room. Please select another room to place the destination.");
        }
    }
    else
    {
        m_pending_warp.room1 = m_roomnum;
        m_pending_warp.room2 = 0xFFFF;
        m_pending_warp.x1 = 31;
        m_pending_warp.y1 = 31;
        m_pending_warp.x2 = 31;
        m_pending_warp.y2 = 31;
        m_pending_warp.x_size = 1;
        m_pending_warp.y_size = 1;
        m_pending_warp.type = WarpList::Warp::Type::NORMAL;
        m_is_warp_pending = true;
        m_warps.push_back(m_pending_warp);
        m_selected = WARP_IDX_OFFSET + m_warps.size();
    }
    if (m_show_warps)
    {
        m_layers[Layer::WARPS] = DrawRoomWarps(m_roomnum);
        ForceRedraw();
    }
    FireEvent(EVT_WARP_UPDATE);
}

void RoomViewerCtrl::AddRoomLink(wxGraphicsContext* gc, const std::string& label, uint16_t room, int x, int y)
{
    wxFont font(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    double w, h;
    gc->SetFont(font, *wxWHITE);
    gc->GetTextExtent(label, &w, &h);
    const double LINK_X = x + std::max<double>(145, w + 20);
    const double LINK_Y = y;
    gc->DrawText(label, x, y);
    gc->SetFont(font, *wxYELLOW);
    auto roomlabel = StrPrintf("Room %03d", room);
    gc->DrawText(roomlabel, LINK_X, LINK_Y);
    gc->GetTextExtent(roomlabel, &w, &h);
    m_link_poly.push_back({ room, {
        {LINK_X, LINK_Y},
        {LINK_X + w, LINK_Y},
        {LINK_X + w, LINK_Y + h},
        {LINK_X, LINK_Y + h},
        {LINK_X, LINK_Y}
    } });
}

void RoomViewerCtrl::RefreshHeightmap()
{
    if (IsShownOnScreen())
    {
        auto map = m_g->GetRoomData()->GetMapForRoom(m_roomnum)->GetData();
        if (m_layer_opacity[Layer::HEIGHTMAP] > 0)
        {
            m_layers[Layer::HEIGHTMAP] = DrawHeightmapVisualisation(map, m_layer_opacity[Layer::HEIGHTMAP]);
        }
        if (m_show_warps)
        {
            m_layers[Layer::WARPS] = DrawRoomWarps(m_roomnum);
        }
        if (m_show_swaps)
        {
            m_layers[Layer::SWAPS] = DrawRoomSwaps(m_roomnum);
        }
        ForceRedraw();
    }
}

void RoomViewerCtrl::RefreshLayers()
{
    if (IsShownOnScreen())
    {
        auto map = m_g->GetRoomData()->GetMapForRoom(m_roomnum)->GetData();
        if (m_layer_opacity[Layer::BACKGROUND1] > 0
            || m_layer_opacity[Layer::BACKGROUND2] > 0
            || m_layer_opacity[Layer::FOREGROUND] > 0)
        {
            DrawRoom(m_roomnum);
        }
        ForceRedraw();
    }
}

void RoomViewerCtrl::DeleteSelectedWarp()
{
    if (IsWarpSelected())
    {
        int idx = GetSelectedWarpIndex();
        const auto& warp = GetSelectedWarp();
        if (m_is_warp_pending && !warp.IsValid())
        {
            m_is_warp_pending = false;
        }
        m_warps.erase(m_warps.begin() + idx - 1);
        if (m_warps.size() == 0)
        {
            m_selected = -1;
        }
        else if (idx > static_cast<int>(m_warps.size()))
        {
            m_selected = WARP_IDX_OFFSET + m_warps.size();
        }
        if (m_show_warps)
        {
            m_layers[Layer::WARPS] = DrawRoomWarps(m_roomnum);
        }
        if (m_show_swaps)
        {
            m_layers[Layer::SWAPS] = DrawRoomSwaps(m_roomnum);
        }
        if (m_show_warps || m_show_swaps)
        {
            ForceRedraw();
        }
        m_g->GetRoomData()->SetWarpsForRoom(m_roomnum, m_warps);
        RefreshStatusbar();
        FireEvent(EVT_WARP_UPDATE);
    }
}

void RoomViewerCtrl::SelectTileSwap(int selection)
{
    bool swap_currently_selected = (m_selected > SWAP_IDX_OFFSET && m_selected <= static_cast<int>(m_warps.size() + SWAP_IDX_OFFSET));
    int cur_sel = swap_currently_selected ? m_selected : NO_SELECTION;
    int new_sel = (selection > 0 && selection <= static_cast<int>(m_swaps.size())) ? selection + SWAP_IDX_OFFSET : NO_SELECTION;

    if ((swap_currently_selected && new_sel == NO_SELECTION) || (cur_sel != new_sel && new_sel != NO_SELECTION))
    {
        if (m_selected != NO_SELECTION)
        {
            m_selected = new_sel;
            RedrawAllSprites();
            m_layers[Layer::WARPS] = DrawRoomWarps(m_roomnum);
            FireEvent(EVT_ENTITY_UPDATE);
        }
        else
        {
            m_selected = new_sel;
        }
        RefreshStatusbar();
        if (m_show_swaps)
        {
            m_layers[Layer::SWAPS] = DrawRoomSwaps(m_roomnum);
            ForceRedraw();
            FireEvent(EVT_TILESWAP_SELECT, new_sel - SWAP_IDX_OFFSET);
        }
    }
}

bool RoomViewerCtrl::IsTileSwapSelected() const
{
    return (m_selected > SWAP_IDX_OFFSET && m_selected <= static_cast<int>(m_swaps.size() + SWAP_IDX_OFFSET));
}

const TileSwap& RoomViewerCtrl::GetSelectedTileSwap() const
{
    assert(IsTileSwapSelected());
    int selection = m_selected - SWAP_IDX_OFFSET - 1;
    auto it = m_swaps.cbegin();
    std::advance(it, selection);
    return *it;
}

int RoomViewerCtrl::GetSelectedTileSwapIndex() const
{
    if (IsTileSwapSelected())
    {
        return m_selected - SWAP_IDX_OFFSET;
    }
    else
    {
        return NO_SELECTION;
    }
}

void RoomViewerCtrl::UpdateSwaps()
{
    if (m_g)
    {
        auto old_swap_size = m_swaps.size();
        m_swaps = m_g->GetRoomData()->GetTileSwaps(m_roomnum);
        if (m_swaps.size() != old_swap_size)
        {
            ClearAllPreviews();
        }
        RefreshRoom(true);
    }
}

void RoomViewerCtrl::SelectDoor(int selection)
{
    bool door_currently_selected = (m_selected > DOOR_IDX_OFFSET && m_selected <= static_cast<int>(m_doors.size() + DOOR_IDX_OFFSET));
    int cur_sel = door_currently_selected ? m_selected : NO_SELECTION;
    int new_sel = (selection > 0 && selection <= static_cast<int>(m_doors.size())) ? selection + DOOR_IDX_OFFSET : NO_SELECTION;

    if ((door_currently_selected && new_sel == NO_SELECTION) || (cur_sel != new_sel && new_sel != NO_SELECTION))
    {
        if (m_selected != NO_SELECTION)
        {
            m_selected = new_sel;
            RedrawAllSprites();
            m_layers[Layer::WARPS] = DrawRoomWarps(m_roomnum);
            FireEvent(EVT_ENTITY_UPDATE);
        }
        else
        {
            m_selected = new_sel;
        }
        RefreshStatusbar();
        if (m_show_swaps)
        {
            m_layers[Layer::SWAPS] = DrawRoomSwaps(m_roomnum);
            ForceRedraw();
            FireEvent(EVT_DOOR_SELECT, new_sel - DOOR_IDX_OFFSET);
        }
    }
}

bool RoomViewerCtrl::IsDoorSelected() const
{
    return (m_selected > DOOR_IDX_OFFSET && m_selected <= static_cast<int>(m_doors.size() + DOOR_IDX_OFFSET));
}

const Door& RoomViewerCtrl::GetSelectedDoor() const
{
    assert(IsDoorSelected());
    int selection = m_selected - DOOR_IDX_OFFSET - 1;
    auto it = m_doors.cbegin();
    std::advance(it, selection);
    return *it;
}

int RoomViewerCtrl::GetSelectedDoorIndex() const
{
    if (IsDoorSelected())
    {
        return m_selected - DOOR_IDX_OFFSET;
    }
    else
    {
        return NO_SELECTION;
    }
}

void RoomViewerCtrl::UpdateDoors()
{
    if (m_g)
    {
        auto old_swap_size = m_doors.size();
        m_doors = m_g->GetRoomData()->GetDoors(m_roomnum);
        if (m_doors.size() != old_swap_size)
        {
            ClearAllPreviews();
        }
        RefreshRoom(true);
    }
}

void RoomViewerCtrl::SetOpacity(wxImage& image, uint8_t opacity)
{
    uint8_t* alpha = image.GetAlpha();
    for (int i = 0; i < (image.GetHeight() * image.GetWidth()); i++)
    {
        *alpha = (*alpha < opacity) ? *alpha : opacity;
        alpha++;
    }
}

void RoomViewerCtrl::ForceRedraw()
{
    m_redraw = true;
    m_repaint = true;
    Refresh(true);
}

void RoomViewerCtrl::ForceRepaint()
{
    m_repaint = true;
    Refresh(true);
}

void RoomViewerCtrl::UpdateScroll()
{
    m_scroll_rate = std::ceil(SCROLL_RATE * m_zoom);
    SetVirtualSize(std::ceil(m_zoom * m_width), std::ceil(m_zoom * m_height));
    SetScrollRate(m_scroll_rate, m_scroll_rate);
    AdjustScrollbars();
}

void RoomViewerCtrl::UpdateBuffer()
{
    m_buffer_width = std::ceil(m_width * m_zoom);
    m_buffer_height = std::ceil(m_height * m_zoom);
    m_bmp->Create(m_buffer_width, m_buffer_height);
}

void RoomViewerCtrl::RefreshGraphics()
{
    if (m_g == nullptr)
    {
        return;
    }
    if (!m_show_warps)
    {
        m_warp_poly.clear();
    }
    if (!m_show_entities && !m_show_entity_hitboxes)
    {
        m_entity_poly.clear();
    }
    DrawRoom(m_roomnum);
    UpdateScroll();
    ForceRedraw();
}

int RoomViewerCtrl::GetErrorCount() const
{
    return m_errors.size();
}

std::string RoomViewerCtrl::GetErrorText(int errnum) const
{
    return m_errors.at(errnum);
}

const std::string& RoomViewerCtrl::GetStatusText() const
{
    return m_status_text;
}


bool RoomViewerCtrl::Pnpoly(const std::vector<wxPoint2DDouble>& poly, int x, int y)
{
    std::size_t i, j;
    bool c = false;
    for (i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
        if (((poly[i].m_y > y) != (poly[j].m_y > y)) &&
            (x < (poly[j].m_x - poly[i].m_x) * (y - poly[i].m_y) / (poly[j].m_y - poly[i].m_y) + poly[i].m_x))
            c = !c;
    }
    return c;
}

void RoomViewerCtrl::GoToRoom(uint16_t room)
{
    const auto& name = m_g->GetRoomData()->GetRoom(room)->name;
    FireEvent(EVT_GO_TO_NAV_ITEM, "Rooms/" + name);
}

void RoomViewerCtrl::FireUpdateStatusEvent(const std::string& data, int pane)
{
    wxCommandEvent evt(EVT_STATUSBAR_UPDATE);
    evt.SetString(data);
    evt.SetInt(pane);
    evt.SetClientData(m_frame);
    wxPostEvent(m_frame, evt);
}

void RoomViewerCtrl::FireEvent(const wxEventType& e, long userdata)
{
    wxCommandEvent evt(e);
    evt.SetInt(userdata);
    evt.SetExtraLong(userdata);
    evt.SetClientData(m_frame);
    wxPostEvent(m_frame, evt);
}

void RoomViewerCtrl::FireEvent(const wxEventType& e, const std::string& userdata)
{
    wxCommandEvent evt(e);
    evt.SetString(userdata);
    evt.SetClientData(m_frame);
    wxPostEvent(m_frame, evt);
}

void RoomViewerCtrl::FireEvent(const wxEventType& e)
{
    wxCommandEvent evt(e);
    evt.SetClientData(m_frame);
    wxPostEvent(m_frame, evt);
}

void RoomViewerCtrl::OnDraw(wxDC& dc)
{
    int sx, sy;
    GetViewStart(&sx, &sy);
    sx *= m_scroll_rate;
    sy *= m_scroll_rate;
    int sw, sh;
    GetClientSize(&sw, &sh);
    wxMemoryDC mdc(*m_bmp);
    if (m_redraw)
    {
        mdc.SetBackground(*wxBLACK_BRUSH);
        mdc.Clear();
        for (const auto& layer : m_layers)
        {
            if (layer.first == Layer::BACKGROUND1 ||
                layer.first == Layer::BACKGROUND2 ||
                layer.first == Layer::FOREGROUND ||
                layer.first == Layer::BG_SPRITES ||
                layer.first == Layer::FG_SPRITES)
            {
                mdc.SetUserScale(m_zoom, m_zoom);
            }
            else
            {
                mdc.SetUserScale(1.0, 1.0);
            }
            mdc.DrawBitmap(*layer.second, 0, 0, true);
        }
        m_redraw = false;
    }
    mdc.SetUserScale(1.0, 1.0);
    dc.SetBackground(*wxBLACK_BRUSH);
    dc.Clear();
    dc.Blit(sx, sy, sw, sh, &mdc, sx, sy, wxCOPY);
    mdc.SelectObject(wxNullBitmap);
}

void RoomViewerCtrl::OnPaint(wxPaintEvent& /*evt*/)
{
	wxBufferedPaintDC dc(this);
	this->PrepareDC(dc);
	this->OnDraw(dc);
}

void RoomViewerCtrl::OnEraseBackground(wxEraseEvent& /*evt*/)
{
}

void RoomViewerCtrl::AddEntityClickRegions(const std::vector<SpriteQ>& q)
{

    for (const auto& s : q)
    {
        auto hitbox = m_g->GetSpriteData()->GetEntityHitbox(s.entity.GetType());


        m_entity_poly.push_back({ s.id, {
            {(double)(s.x + hitbox.first * 2), (double)(s.y)},
            {(double)(s.x + hitbox.first * 2), (double)(s.y - hitbox.second)},
            {(double)(s.x), (double)(s.y - hitbox.first - hitbox.second)},
            {(double)(s.x - hitbox.first * 2), (double)(s.y - hitbox.second)},
            {(double)(s.x - hitbox.first * 2), (double)(s.y)},
            {(double)(s.x), (double)(s.y + hitbox.first)},
            {(double)(s.x + hitbox.first * 2), (double)(s.y)}
        } });
    }
}

void RoomViewerCtrl::OnSize(wxSizeEvent& evt)
{
	Refresh(false);
	evt.Skip();
}

void RoomViewerCtrl::OnMouseMove(wxMouseEvent& evt)
{
    auto xy = GetAbsoluteCoordinates(evt.GetX(), evt.GetY());
    std::string status_text = m_status_text;
    if (m_g != nullptr)
    {
        auto map = m_g->GetRoomData()->GetMapForRoom(m_roomnum)->GetData();
        if (map)
        {
            auto r = map->PixelToHMPoint({ xy.first, xy.second });
            status_text = StrPrintf("(%d,%d) [%d,%d]", xy.first, xy.second, r.x, r.y);
            if (CheckMousePosForLink(xy, status_text))
            {
                Refresh();
                SetCursor(m_hovered != NO_SELECTION ? wxCURSOR_HAND : wxCURSOR_ARROW);
            }
            if (status_text != m_status_text)
            {
                FireUpdateStatusEvent(status_text, 0);
            }
        }
    }
    evt.Skip();
}

void RoomViewerCtrl::UpdateEntityProperties(int entity)
{
    if (entity > 0 && entity <= static_cast<int>(m_entities.size()))
    {
        EntityPropertiesWindow dlg(m_frame, entity, &m_entities[entity - 1]);
        if (dlg.ShowModal() == wxID_OK)
        {
            m_g->GetSpriteData()->SetRoomEntities(m_roomnum, m_entities);
            FireEvent(EVT_ENTITY_UPDATE);
            RefreshStatusbar();
            RedrawAllSprites();
            ForceRedraw();
        }
    }
}

void RoomViewerCtrl::AddEntity()
{
    DoAddEntity();
    RefreshStatusbar();
    FireEvent(EVT_ENTITY_UPDATE);
    RedrawAllSprites();
    ForceRedraw();
}

void RoomViewerCtrl::DeleteSelectedEntity()
{
    DoDeleteEntity(m_selected);
    RefreshStatusbar();
    FireEvent(EVT_ENTITY_UPDATE);
    RedrawAllSprites();
    ForceRedraw();
}

void RoomViewerCtrl::MoveSelectedEntityUp()
{
    if (m_selected > 1)
    {
        DoMoveEntityUp(m_selected);
        m_selected--;
        RefreshStatusbar();
        FireEvent(EVT_ENTITY_UPDATE);
        RedrawAllSprites();
        ForceRedraw();
    }
}

void RoomViewerCtrl::MoveSelectedEntityDown()
{
    if (m_selected < static_cast<int>(m_entities.size()))
    {
        DoMoveEntityDown(m_selected);
        m_selected++;
        RefreshStatusbar();
        FireEvent(EVT_ENTITY_UPDATE);
        RedrawAllSprites();
        ForceRedraw();
    }
}

void RoomViewerCtrl::OnMouseLeave(wxMouseEvent& evt)
{
    SetCursor(wxCURSOR_ARROW);
    evt.Skip();
}

void RoomViewerCtrl::OnLeftClick(wxMouseEvent& evt)
{
    auto xy = GetAbsoluteCoordinates(evt.GetX(), evt.GetY());
    bool selection_made = false;
    if (m_g != nullptr)
    {
        auto map = m_g->GetRoomData()->GetMapForRoom(m_roomnum)->GetData();
        if (map)
        {
            std::string status_text;
            if (CheckMousePosForLink(xy, status_text))
            {
                SetCursor(m_hovered != NO_SELECTION ? wxCURSOR_HAND : wxCURSOR_ARROW);
            }
            selection_made = UpdateSelection(m_hovered, evt.ControlDown() ? Action::DO_ACTION : Action::NORMAL);
            m_selected = m_hovered;
        }
    }
    evt.Skip();
}

std::pair<int, int> RoomViewerCtrl::GetAbsoluteCoordinates(int screenx, int screeny)
{
    int x, y;
    GetViewStart(&x, &y);
    x *= m_scroll_rate;
    y *= m_scroll_rate;
    x += screenx;
    y += screeny;
    x = std::ceil(x / m_zoom);
    y = std::ceil(y / m_zoom);
    return { x, y };
}

bool RoomViewerCtrl::HandleKeyUp(unsigned int /*key*/, unsigned int /*modifiers*/)
{
    return true;
}

bool RoomViewerCtrl::HandleKeyDown(unsigned int key, unsigned int modifiers)
{
    if (key == WXK_ESCAPE)
    {
        ClearSelection();
        return false;
    }
    if (HandleNormalModeKeyDown(key, modifiers) == false)
    {
        if (IsEntitySelected())
        {
            return !HandleNEntityKeyDown(key, modifiers);
        }
        if (IsWarpSelected())
        {
            return !HandleNWarpKeyDown(key, modifiers);
        }
        if (IsTileSwapSelected() || IsDoorSelected())
        {
            return !HandleNSwapKeyDown(key, modifiers);
        }
    }
    else
    {
        return false;
    }
    return true;
}

bool RoomViewerCtrl::HandleNormalModeKeyDown(unsigned int key, unsigned int modifiers)
{
    if (m_redraw)
    {
        return true;
    }
    bool key_handled = false;
    switch (key)
    {
    case WXK_TAB:
        if (modifiers == 0)
        {
            if (IsEntitySelected())
            {
                SelectEntity((GetSelectedEntityIndex()) % m_entities.size() + 1);
            }
            else
            {
                SelectEntity(1);
            }
            key_handled = true;
        }
        else if (modifiers == wxMOD_SHIFT)
        {
            if (IsEntitySelected())
            {
                SelectEntity(GetSelectedEntityIndex() == 1 ? m_entities.size() : GetSelectedEntityIndex() - 1);
            }
            else
            {
                SelectEntity(m_entities.size());
            }
            key_handled = true;
        }
        break;
    case '.':
        if ((modifiers & wxMOD_ALT) > 0)
        {
            if (!m_swaps.empty() || !m_doors.empty())
            {
                int sel = NO_SELECTION;
                if (IsTileSwapSelected())
                {
                    sel = GetSelectedTileSwapIndex() - 1;
                }
                else if (IsDoorSelected())
                {
                    sel = m_swaps.size() + GetSelectedDoorIndex() - 1;
                }
                if (++sel >= static_cast<int>(m_swaps.size() + m_doors.size()))
                {
                    sel = 0;
                }
                if (sel >= static_cast<int>(m_swaps.size()))
                {
                    SelectDoor(sel - m_swaps.size() + 1);
                }
                else
                {
                    SelectTileSwap(sel + 1);
                }
                key_handled = true;
            }
        }
        else
        {
            if (IsEntitySelected())
            {
                SelectEntity((GetSelectedEntityIndex()) % m_entities.size() + 1);
            }
            else
            {
                SelectEntity(1);
            }
            key_handled = true;
        }
        break;
    case ',':
        if ((modifiers & wxMOD_ALT) > 0)
        {
            if (!m_swaps.empty() || !m_doors.empty())
            {
                int sel = NO_SELECTION;
                if (IsTileSwapSelected())
                {
                    sel = GetSelectedTileSwapIndex() - 1;
                }
                else if (IsDoorSelected())
                {
                    sel = m_swaps.size() + GetSelectedDoorIndex() - 1;
                }
                if (--sel < 0)
                {
                    sel = m_swaps.size() + m_doors.size() - 1;
                }
                if (sel >= static_cast<int>(m_swaps.size()))
                {
                    SelectDoor(sel - m_swaps.size() + 1);
                }
                else
                {
                    SelectTileSwap(sel + 1);
                }
                key_handled = true;
            }
        }
        else
        {
            if (IsEntitySelected())
            {
                SelectEntity(GetSelectedEntityIndex() == 1 ? m_entities.size() : GetSelectedEntityIndex() - 1);
            }
            else
            {
                SelectEntity(m_entities.size());
            }
            key_handled = true;
        }
        break;
    case '>':
        if (IsWarpSelected())
        {
            SelectWarp((GetSelectedWarpIndex()) % m_warps.size() + 1);
        }
        else
        {
            SelectWarp(1);
        }
        key_handled = true;
        break;
    case '<':
        if (IsWarpSelected())
        {
            SelectWarp(GetSelectedWarpIndex() == 1 ? m_warps.size() : GetSelectedWarpIndex() - 1);
        }
        else
        {
            SelectWarp(m_warps.size());
        }
        key_handled = true;
        break;
    case WXK_INSERT:
        if (modifiers == wxMOD_SHIFT)
        {
            AddWarp();
        }
        else if (modifiers == wxMOD_CONTROL)
        {
            FireEvent(EVT_TILESWAP_ADD);
        }
        else if (modifiers == wxMOD_ALT)
        {
            FireEvent(EVT_DOOR_ADD);
        }
        else
        {
            AddEntity();
        }
        key_handled = true;
        break;
    }
    return key_handled;
}

bool RoomViewerCtrl::HandleNEntityKeyDown(unsigned int key, unsigned int modifiers)
{
    if (m_redraw)
    {
        return true;
    }
    bool refresh_entities = false;
    bool key_handled = false;
    if (IsEntitySelected())
    {
        auto& ent = m_entities[GetSelectedEntityIndex() - 1];
        switch (key)
        {
        case WXK_UP:
        case 'W':
        case 'w':
            ent.SetX(ent.GetX() - 0x80);
            refresh_entities = true;
            key_handled = true;
            break;
        case WXK_DOWN:
        case 'S':
        case 's':
            ent.SetX(ent.GetX() + 0x80);
            refresh_entities = true;
            key_handled = true;
            break;
        case WXK_LEFT:
        case 'A':
        case 'a':
            ent.SetY(ent.GetY() + 0x80);
            refresh_entities = true;
            key_handled = true;
            break;
        case WXK_RIGHT:
        case 'D':
        case 'd':
            ent.SetY(ent.GetY() - 0x80);
            refresh_entities = true;
            key_handled = true;
            break;
        case WXK_PAGEUP:
            ent.SetZ(ent.GetZ() + 0x80);
            refresh_entities = true;
            key_handled = true;
            break;
        case WXK_PAGEDOWN:
            ent.SetZ(ent.GetZ() - 0x80);
            refresh_entities = true;
            key_handled = true;
            break;
        case '+':
            ent.SetType((ent.GetType() + 1) & 0xFF);
            m_g->GetRoomData()->CleanupChests(*m_g);
            refresh_entities = true;
            key_handled = true;
            break;
        case '-':
            ent.SetType((ent.GetType() - 1) & 0xFF);
            m_g->GetRoomData()->CleanupChests(*m_g);
            refresh_entities = true;
            key_handled = true;
            break;
        case WXK_DELETE:
            if (modifiers == wxMOD_SHIFT)
            {
                DeleteSelectedEntity();
                key_handled = true;
            }
            break;
        case 'R':
        case 'r':
            if (modifiers == wxMOD_SHIFT)
            {
                ent.SetOrientation(static_cast<Orientation>(
                    (static_cast<int>(ent.GetOrientation()) + 1) & 0x03));
                refresh_entities = true;
                key_handled = true;
            }
            else if (modifiers == 0)
            {
                ent.SetOrientation(static_cast<Orientation>(
                    (static_cast<int>(ent.GetOrientation()) - 1) & 0x03));
                refresh_entities = true;
                key_handled = true;
            }
            break;
        case 'P':
        case 'p':
            if (modifiers == wxMOD_SHIFT)
            {
                ent.SetPalette((ent.GetPalette() + 1) & 0x03);
                refresh_entities = true;
                key_handled = true;
            }
            else if (modifiers == 0)
            {
                ent.SetPalette((ent.GetPalette() - 1) & 0x03);
                refresh_entities = true;
                key_handled = true;
            }
            break;
        case 'U':
        case 'u':
            if (m_entities.size() < 15)
            {
                m_entities.push_back(ent);
                m_entities.back().SetX(m_entities.back().GetX() + 0x100);
                m_selected = m_entities.size();
                refresh_entities = true;
                key_handled = true;
            }
            break;
        case '[':
            if (m_selected > 1)
            {
                DoMoveEntityUp(m_selected);
                m_selected--;
                refresh_entities = true;
                key_handled = true;
            }
            break;
        case ']':
            if (m_selected < static_cast<int>(m_entities.size()))
            {
                DoMoveEntityDown(m_selected);
                m_selected++;
                refresh_entities = true;
                key_handled = true;
            }
            break;
        case WXK_RETURN:
            UpdateEntityProperties(m_selected);
            key_handled = true;
            break;
        }
    }
    if (refresh_entities)
    {
        m_g->GetSpriteData()->SetRoomEntities(m_roomnum, m_entities);
        RefreshStatusbar();
        FireEvent(EVT_ENTITY_UPDATE);
        RedrawAllSprites();
        ForceRedraw();
    }
    return key_handled;
}

bool RoomViewerCtrl::HandleNWarpKeyDown(unsigned int key, unsigned int modifiers)
{
    if (m_redraw)
    {
        return true;
    }
    bool refresh_warps = false;
    bool key_handled = false;
    if (IsWarpSelected())
    {
        auto& warp = m_warps[GetSelectedWarpIndex() - 1];
        switch (key)
        {
        case WXK_UP:
        case 'W':
        case 'w':
            if (warp.room1 == m_roomnum)
            {
                warp.x1 = std::clamp(warp.x1 - 1, 0, 63);
            }
            else
            {
                warp.x2 = std::clamp(warp.x2 - 1, 0, 63);
            }
            refresh_warps = true;
            key_handled = true;
            break;
        case WXK_DOWN:
        case 'S':
        case 's':
            if (warp.room1 == m_roomnum)
            {
                warp.x1 = std::clamp(warp.x1 + 1, 0, 63);
            }
            else
            {
                warp.x2 = std::clamp(warp.x2 + 1, 0, 63);
            }
            refresh_warps = true;
            key_handled = true;
            break;
        case WXK_LEFT:
        case 'A':
        case 'a':
            if (warp.room1 == m_roomnum)
            {
                warp.y1 = std::clamp(warp.y1 + 1, 0, 63);
            }
            else
            {
                warp.y2 = std::clamp(warp.y2 + 1, 0, 63);
            }
            refresh_warps = true;
            key_handled = true;
            break;
        case WXK_RIGHT:
        case 'D':
        case 'd':
            if (warp.room1 == m_roomnum)
            {
                warp.y1 = std::clamp(warp.y1 - 1, 0, 63);
            }
            else
            {
                warp.y2 = std::clamp(warp.y2 - 1, 0, 63);
            }
            refresh_warps = true;
            key_handled = true;
            break;
        case WXK_PAGEUP:
            warp.y_size = 1;
            warp.x_size = 1 + warp.x_size % 3;
            refresh_warps = true;
            key_handled = true;
            break;
        case WXK_PAGEDOWN:
            warp.y_size = 1 + warp.y_size % 3;
            warp.x_size = 1;
            refresh_warps = true;
            key_handled = true;
            break;
        case '+':
            warp.type = static_cast<WarpList::Warp::Type>((static_cast<int>(warp.type) + 1) % 4);
            refresh_warps = true;
            key_handled = true;
            break;
        case '-':
            warp.type = static_cast<WarpList::Warp::Type>((static_cast<int>(warp.type) - 1) % 4);
            refresh_warps = true;
            key_handled = true;
            break;
        case '/':
            if (!m_is_warp_pending)
            {
                if (warp.room1 == m_roomnum)
                {
                    warp.room2 = 0xFFFF;
                }
                else
                {
                    warp.room1 = 0xFFFF;
                }
                m_pending_warp = warp;
                m_is_warp_pending = true;
            }
            refresh_warps = true;
            key_handled = true;
            break;
        case 'U':
        case 'u':
            if(warp.IsValid())
            {
                m_warps.push_back(warp);
                if (m_warps.back().room1 == m_roomnum)
                {
                    m_warps.back().x1 -= m_warps.back().x_size;
                    m_warps.back().x1 = std::clamp<uint8_t>(m_warps.back().x1, 0, 63);
                }
                else
                {
                    m_warps.back().x2 -= m_warps.back().x_size;
                    m_warps.back().x2 = std::clamp<uint8_t>(m_warps.back().x2, 0, 63);
                }
                m_selected = m_warps.size() + WARP_IDX_OFFSET;
                refresh_warps = true;
                key_handled = true;
            }
            break;
        case WXK_DELETE:
            if (modifiers == wxMOD_SHIFT)
            {
                DeleteSelectedWarp();
                key_handled = true;
            }
            break;
        case WXK_RETURN:
            UpdateWarpProperties(GetSelectedWarpIndex());
            key_handled = true;
            break;
        case WXK_SPACE:
            const auto& w = GetSelectedWarp();
            GoToRoom(w.room1 == m_roomnum ? w.room2 : w.room1);
            key_handled = true;
            break;
        }
    }
    if (refresh_warps)
    {
        m_g->GetRoomData()->SetWarpsForRoom(m_roomnum, m_warps);
        RefreshStatusbar();
        FireEvent(EVT_WARP_UPDATE);
        m_layers[Layer::WARPS] = DrawRoomWarps(m_roomnum);
        ForceRedraw();
    }
    return key_handled;
}

bool RoomViewerCtrl::HandleNSwapKeyDown(unsigned int key, unsigned int modifiers)
{
#define INCR(X) X = std::clamp(X + 1, 0 , 0x3F)
#define DECR(X) X = std::clamp(X - 1, 0 , 0x3F)
#define INCSZ(X) X = std::clamp(X + 1, 1 , 0x3F)
#define DECSZ(X) X = std::clamp(X - 1, 1 , 0x3F)
#define INCDSZ(X, MAX) X = static_cast<decltype(X)>((static_cast<int>(X) + MAX + 1) % MAX)
#define DECDSZ(X, MAX) X = static_cast<decltype(X)>((static_cast<int>(X) + MAX - 1) % MAX)

    bool upd = false;

    switch (key)
    {
    case WXK_DELETE:
        if (IsTileSwapSelected())
        {
            FireEvent(EVT_TILESWAP_DELETE, GetSelectedTileSwapIndex());
        }
        else if (IsDoorSelected())
        {
            FireEvent(EVT_DOOR_DELETE, GetSelectedDoorIndex());
        }
        break;
    case '[':
    case '{':
        if (IsTileSwapSelected())
        {
            FireEvent(EVT_TILESWAP_MOVE_UP, GetSelectedTileSwapIndex());
        }
        else if (IsDoorSelected())
        {
            FireEvent(EVT_DOOR_MOVE_UP, GetSelectedDoorIndex());
        }
        break;
    case ']':
    case '}':
        if (IsTileSwapSelected())
        {
            FireEvent(EVT_TILESWAP_MOVE_DOWN, GetSelectedTileSwapIndex());
        }
        else if (IsDoorSelected())
        {
            FireEvent(EVT_DOOR_MOVE_DOWN, GetSelectedDoorIndex());
        }
        break;
    case WXK_LEFT:
    case 'a':
    case 'A':
        if (IsDoorSelected())
        {
            if (modifiers == (wxMOD_CONTROL | wxMOD_SHIFT))
            {
                DECDSZ(m_doors[GetSelectedDoorIndex() - 1].size, Door::SIZES.size());
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                DECR(m_doors[GetSelectedDoorIndex() - 1].x);
                upd = true;
            }
        }
        else if (IsTileSwapSelected())
        {
            if (modifiers == (wxMOD_CONTROL | wxMOD_ALT))
            {
                DECR(m_swaps[GetSelectedTileSwapIndex() - 1].map.dst_x);
                upd = true;
            }
            else if (modifiers == (wxMOD_CONTROL | wxMOD_SHIFT))
            {
                DECSZ(m_swaps[GetSelectedTileSwapIndex() - 1].map.width);
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                DECR(m_swaps[GetSelectedTileSwapIndex() - 1].map.src_x);
                upd = true;
            }
        }
        break;
    case WXK_RIGHT:
    case 'd':
    case 'D':
        if (IsDoorSelected())
        {
            if (modifiers == (wxMOD_CONTROL | wxMOD_SHIFT))
            {
                INCDSZ(m_doors[GetSelectedDoorIndex() - 1].size, Door::SIZES.size());
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                INCR(m_doors[GetSelectedDoorIndex() - 1].x);
                upd = true;
            }
        }
        else if (IsTileSwapSelected())
        {
            if (modifiers == (wxMOD_CONTROL | wxMOD_ALT))
            {
                INCR(m_swaps[GetSelectedTileSwapIndex() - 1].map.dst_x);
                upd = true;
            }
            else if (modifiers == (wxMOD_CONTROL | wxMOD_SHIFT))
            {
                INCSZ(m_swaps[GetSelectedTileSwapIndex() - 1].map.width);
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                INCR(m_swaps[GetSelectedTileSwapIndex() - 1].map.src_x);
                upd = true;
            }
        }
        break;
    case WXK_UP:
    case 'w':
    case 'W':
        if (IsDoorSelected())
        {
            if (modifiers == (wxMOD_CONTROL | wxMOD_SHIFT))
            {
                DECDSZ(m_doors[GetSelectedDoorIndex() - 1].size, Door::SIZES.size());
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                DECR(m_doors[GetSelectedDoorIndex() - 1].y);
                upd = true;
            }
        }
        else if (IsTileSwapSelected())
        {
            if (modifiers == (wxMOD_CONTROL | wxMOD_ALT))
            {
                DECR(m_swaps[GetSelectedTileSwapIndex() - 1].map.dst_y);
                upd = true;
            }
            else if (modifiers == (wxMOD_CONTROL | wxMOD_SHIFT))
            {
                DECSZ(m_swaps[GetSelectedTileSwapIndex() - 1].map.height);
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                DECR(m_swaps[GetSelectedTileSwapIndex() - 1].map.src_y);
                upd = true;
            }
        }
        break;
    case WXK_DOWN:
    case 's':
    case 'S':
        if (IsDoorSelected())
        {
            if (modifiers == (wxMOD_CONTROL | wxMOD_SHIFT))
            {
                INCDSZ(m_doors[GetSelectedDoorIndex() - 1].size, Door::SIZES.size());
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                INCR(m_doors[GetSelectedDoorIndex() - 1].y);
                upd = true;
            }
        }
        else if (IsTileSwapSelected())
        {
            if (modifiers == (wxMOD_CONTROL | wxMOD_ALT))
            {
                INCR(m_swaps[GetSelectedTileSwapIndex() - 1].map.dst_y);
                upd = true;
            }
            else if (modifiers == (wxMOD_CONTROL | wxMOD_SHIFT))
            {
                INCSZ(m_swaps[GetSelectedTileSwapIndex() - 1].map.height);
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                INCR(m_swaps[GetSelectedTileSwapIndex() - 1].map.src_y);
                upd = true;
            }
        }
        break;
    case 'T':
    case 't':
        if (IsTileSwapSelected())
        {
            if (modifiers == (wxMOD_CONTROL | wxMOD_SHIFT))
            {
                DECDSZ(m_swaps[GetSelectedTileSwapIndex() - 1].mode, 3);
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                INCDSZ(m_swaps[GetSelectedTileSwapIndex() - 1].mode, 3);
                upd = true;
            }
        }
        break;
    case WXK_RETURN:
        if (IsTileSwapSelected())
        {
            FireEvent(EVT_TILESWAP_OPEN_PROPERTIES, GetSelectedTileSwapIndex());
        }
        else if (IsDoorSelected())
        {
            FireEvent(EVT_DOOR_OPEN_PROPERTIES, GetSelectedDoorIndex());
        }
        break;
    case 'P':
    case 'p':
        if (IsDoorSelected())
        {
            TogglePreviewDoor(GetSelectedDoorIndex());
        }
        else if (IsTileSwapSelected())
        {
            TogglePreviewSwap(GetSelectedTileSwapIndex());
        }
        else
        {
            ClearAllPreviews();
        }
        RefreshStatusbar();
        ForceRedraw();
        break;
    }

    if (upd)
    {
        if (m_g)
        {
            m_g->GetRoomData()->SetDoors(m_roomnum, m_doors);
            m_g->GetRoomData()->SetTileSwaps(m_roomnum, m_swaps);
            if (IsDoorSelected())
            {
                FireEvent(EVT_DOOR_UPDATE, GetSelectedDoorIndex());
            }
            else if (IsTileSwapSelected())
            {
                FireEvent(EVT_TILESWAP_UPDATE, GetSelectedTileSwapIndex());
            }
        }
        Refresh();
    }

    return true;
#undef DECDSZ
#undef INCDSZ
#undef DECSZ
#undef INCSZ
#undef DECR
#undef INCR
}

bool RoomViewerCtrl::SetCursor(wxStockCursor cursor_id)
{
    static wxStockCursor last_cursor_id = wxStockCursor::wxCURSOR_BLANK;
    bool result = false;
    if (cursor_id != last_cursor_id)
    {
        result = wxWindow::SetCursor(cursor_id);
        last_cursor_id = cursor_id;
    }
    return result;
}

void RoomViewerCtrl::DoAddEntity()
{
    if (m_entities.size() < 15)
    {
        m_entities.push_back(Entity());
        m_selected = m_entities.size();
        m_g->GetSpriteData()->SetRoomEntities(m_roomnum, m_entities);
    }
}

void RoomViewerCtrl::DoDeleteEntity(int entity)
{
    if (entity > 0 && entity <= static_cast<int>(m_entities.size()))
    {
        m_entities.erase(m_entities.begin() + (m_selected - 1));
        if (m_entities.size() == 0)
        {
            m_selected = -1;
        }
        else if (m_selected > static_cast<int>(m_entities.size()))
        {
            m_selected = m_entities.size();
        }
        m_g->GetSpriteData()->SetRoomEntities(m_roomnum, m_entities);
        m_g->GetRoomData()->CleanupChests(*m_g);
    }
}

void RoomViewerCtrl::DoMoveEntityUp(int entity)
{
    if (entity > 1 && entity <= static_cast<int>(m_entities.size()))
    {
        std::swap(m_entities[entity - 1], m_entities[entity - 2]);
        m_g->GetSpriteData()->SetRoomEntities(m_roomnum, m_entities);
    }
}

void RoomViewerCtrl::DoMoveEntityDown(int entity)
{
    if (entity > 0 && entity < static_cast<int>(m_entities.size()))
    {
        std::swap(m_entities[entity - 1], m_entities[entity]);
        m_g->GetSpriteData()->SetRoomEntities(m_roomnum, m_entities);
    }
}

bool RoomViewerCtrl::CheckMousePosForLink(const std::pair<int, int>& xy, std::string& status_text)
{
    int prev_hover = m_hovered;
    for (const auto& wp : m_warp_poly)
    {
        if (Pnpoly(wp.second, xy.first, xy.second))
        {
            const auto& warp = m_warps.at(wp.first);
            uint16_t room = (warp.room1 == m_roomnum) ? warp.room2 : warp.room1;
            uint8_t wx = (warp.room1 == m_roomnum) ? warp.x2 : warp.x1;
            uint8_t wy = (warp.room1 == m_roomnum) ? warp.y2 : warp.y1;
            status_text += StrPrintf(" - Right Click: Warp to room %03d (%d,%d)", room, wx, wy);
            m_hovered = WARP_IDX_OFFSET + wp.first + 1;
            return m_hovered != prev_hover;
        }
    }
    int i = 0;
    for (auto it = m_link_poly.cbegin(); it != m_link_poly.cend(); ++i, ++it)
    {
        if (Pnpoly(it->second, xy.first, xy.second))
        {
            status_text += StrPrintf(" - Right Click: Warp to room %03d", it->first);
            m_hovered = LINK_IDX_OFFSET + i;
            return m_hovered != prev_hover;
        }
    }
    for (const auto& ep : m_entity_poly)
    {
        if (Pnpoly(ep.second, xy.first, xy.second))
        {
            SetCursor(wxStockCursor::wxCURSOR_HAND);
            m_hovered = ENTITY_IDX_OFFSET + ep.first;
            status_text += StrPrintf(" - Entity (%d)", ep.first);
            return m_hovered != prev_hover;
        }
    }
    for (i = 0; i < static_cast<int>(m_swap_regions.size()); ++i)
    {
        const auto& [sp, dp] = m_swap_regions.at(i);
        if (Pnpoly(sp, xy.first, xy.second) || Pnpoly(dp, xy.first, xy.second))
        {
            m_hovered = SWAP_IDX_OFFSET + i + 1;
            if (m_hovered != prev_hover)
            {
                UpdateLayer(Layer::SWAPS, DrawRoomSwaps(m_roomnum));
                m_redraw = true;
                m_repaint = true;
            }
            status_text += StrPrintf(" - Tile Swap (%d)", i + 1);
            return m_hovered != prev_hover;
        }
    }
    for (i = 0; i < static_cast<int>(m_door_regions.size()); ++i)
    {
        const auto& dp = m_door_regions.at(i);
        if (Pnpoly(dp, xy.first, xy.second))
        {
            m_hovered = DOOR_IDX_OFFSET + i + 1;
            if (m_hovered != prev_hover)
            {
                UpdateLayer(Layer::SWAPS, DrawRoomSwaps(m_roomnum));
                m_redraw = true;
                m_repaint = true;
            }
            status_text += StrPrintf(" - Door (%d)", i + 1);
            return m_hovered != prev_hover;
        }
    }
    if (m_hovered != NO_SELECTION)
    {
        m_hovered = NO_SELECTION;
        if (prev_hover >= SWAP_IDX_OFFSET)
        {
            UpdateLayer(Layer::SWAPS, DrawRoomSwaps(m_roomnum));
            m_redraw = true;
            m_repaint = true;
        }
        return m_hovered != prev_hover;
    }
    return false;
}

bool RoomViewerCtrl::UpdateSelection(int new_selection, RoomViewerCtrl::Action action)
{
    if (new_selection == NO_SELECTION)
    {
        if (action == Action::DO_ALT_ACTION)
        {
            ClearAllPreviews();
        }
        ClearSelection();
        return true;
    }
    else if (new_selection >= LINK_IDX_OFFSET && new_selection < LINK_IDX_OFFSET + static_cast<int>(m_link_poly.size()))
    {
        GoToRoom(m_link_poly.at(new_selection - LINK_IDX_OFFSET).first);
        return true;
    }
    else if (new_selection > WARP_IDX_OFFSET && new_selection <= WARP_IDX_OFFSET + static_cast<int>(m_warp_poly.size()))
    {
        int warp_idx = new_selection - WARP_IDX_OFFSET;
        if (action == Action::DO_ALT_ACTION)
        {
            const auto& warp = m_warps.at(warp_idx - 1);
            uint16_t room = (warp.room1 == m_roomnum) ? warp.room2 : warp.room1;
            GoToRoom(room);
        }
        else if (action == Action::DO_ACTION)
        {
            UpdateWarpProperties(warp_idx);
        }
        else
        {
            SelectWarp(warp_idx);
        }
        return true;
    }
    else if (new_selection > ENTITY_IDX_OFFSET && new_selection <= ENTITY_IDX_OFFSET + static_cast<int>(m_entity_poly.size()))
    {
        SelectEntity(new_selection);
        if (action != Action::NORMAL)
        {
            UpdateEntityProperties(new_selection - ENTITY_IDX_OFFSET);
        }
        return true;
    }
    else if (new_selection >= SWAP_IDX_OFFSET && new_selection <= SWAP_IDX_OFFSET + static_cast<int>(m_swap_regions.size()))
    {
        SelectTileSwap(new_selection - SWAP_IDX_OFFSET);
        if (action == Action::DO_ALT_ACTION)
        {
            // TODO: Add preview toggle here
            TogglePreviewSwap(new_selection - SWAP_IDX_OFFSET);
        }
        if (action == Action::DO_ACTION)
        {
            FireEvent(EVT_TILESWAP_OPEN_PROPERTIES, new_selection - SWAP_IDX_OFFSET);
        }
        return true;
    }
    else if (new_selection >= DOOR_IDX_OFFSET && new_selection <= DOOR_IDX_OFFSET + static_cast<int>(m_door_regions.size()))
    {
        SelectDoor(new_selection - DOOR_IDX_OFFSET);
        if (action == Action::DO_ALT_ACTION)
        {
            // TODO: Add preview toggle here
            TogglePreviewDoor(new_selection - DOOR_IDX_OFFSET);
        }
        if (action == Action::DO_ACTION)
        {
            FireEvent(EVT_DOOR_OPEN_PROPERTIES, new_selection - DOOR_IDX_OFFSET);
        }
        return true;
    }

    return false;
}

std::vector<TileSwap> RoomViewerCtrl::GetPreviewSwaps()
{
    std::vector<TileSwap> swaps;
    for (auto idx : m_preview_swaps)
    {
        if (idx > 0 && idx <= static_cast<int>(m_swaps.size()))
        {
            swaps.push_back(m_swaps.at(idx - 1));
        }
    }
    return swaps;
}

std::vector<Door> RoomViewerCtrl::GetPreviewDoors()
{
    std::vector<Door> doors;
    for (auto idx : m_preview_doors)
    {
        if (idx > 0 && idx <= static_cast<int>(m_doors.size()))
        {
            doors.push_back(m_doors.at(idx - 1));
        }
    }
    return doors;
}

void RoomViewerCtrl::TogglePreviewSwap(int swap)
{
    auto pos = std::find(m_preview_swaps.begin(), m_preview_swaps.end(), swap);
    if (pos == m_preview_swaps.cend())
    {
        m_preview_swaps.push_back(swap);
    }
    else
    {
        m_preview_swaps.erase(pos);
    }
    RefreshRoom(true);
}

void RoomViewerCtrl::TogglePreviewDoor(int door)
{
    auto pos = std::find(m_preview_doors.begin(), m_preview_doors.end(), door);
    if (pos == m_preview_doors.cend())
    {
        m_preview_doors.push_back(door);
    }
    else
    {
        m_preview_doors.erase(pos);
    }
    RefreshRoom(true);
}

void RoomViewerCtrl::ClearAllPreviews()
{
    m_preview_doors.clear();
    m_preview_swaps.clear();
    RefreshRoom(true);
}

void RoomViewerCtrl::OnLeftDblClick(wxMouseEvent& evt)
{
    auto xy = GetAbsoluteCoordinates(evt.GetX(), evt.GetY());
    bool selection_made = false;
    if (m_g != nullptr)
    {
        auto map = m_g->GetRoomData()->GetMapForRoom(m_roomnum)->GetData();
        if (map)
        {
            std::string status_text;
            CheckMousePosForLink(xy, status_text);
            selection_made = UpdateSelection(m_hovered, Action::DO_ACTION);
            m_selected = m_hovered;
        }
    }
    evt.Skip();
}

void RoomViewerCtrl::OnRightClick(wxMouseEvent& evt)
{
    auto xy = GetAbsoluteCoordinates(evt.GetX(), evt.GetY());
    bool selection_made = false;
    if (m_g != nullptr)
    {
        auto map = m_g->GetRoomData()->GetMapForRoom(m_roomnum)->GetData();
        if (map)
        {
            std::string status_text;
            if (CheckMousePosForLink(xy, status_text))
            {

            }
            selection_made = UpdateSelection(m_hovered, Action::DO_ALT_ACTION);
            m_selected = m_hovered;
        }
    }
    evt.Skip();
}

void RoomViewerCtrl::OnRightDblClick(wxMouseEvent& evt)
{
    auto xy = GetAbsoluteCoordinates(evt.GetX(), evt.GetY());
    bool selection_made = false;
    for (const auto& ep : m_warp_poly)
    {
        if (Pnpoly(ep.second, xy.first, xy.second) && ep.first == GetSelectedWarpIndex() - 1)
        {
            UpdateWarpProperties(GetSelectedWarpIndex());
            selection_made = true;
            break;
        }
    }
    if (!selection_made)
    {
        for (const auto& ep : m_entity_poly)
        {
            if (Pnpoly(ep.second, xy.first, xy.second) && ep.first == m_selected)
            {
                UpdateEntityProperties(m_selected);
                break;
            }
        }
    }
    evt.Skip();
}

wxDECLARE_EVENT(EVT_ENTITY_SELECT, wxCommandEvent);
