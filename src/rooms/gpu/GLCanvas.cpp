#include "GLCanvas.h"
#include "GLCanvasHeightmapHitTest.h"
#include "GLCanvasKeyboardInput.h"
#include "GLCanvasMouseInput.h"
#include "GLCanvasObjectCoordinator.h"
#include "GLCanvasRoomMode.h"
#include "RoomProjection.h"
#include <rooms/RoomViewerFrame.h>
#include <main/EditorFrame.h>
#include <algorithm>
#include <cmath>

using namespace Landstalker;

namespace {
// File-local rendering/math helpers used only by GLCanvas.
int GLCanvasAttributes[] = {
    WX_GL_RGBA,
    WX_GL_DOUBLEBUFFER,
    WX_GL_STENCIL_SIZE, 8,
    0
};


uint8_t OpacityToByte(float opacity) {
    return static_cast<uint8_t>(std::lround(std::clamp(opacity, 0.0f, 1.0f) * 255.0f));
}

constexpr float kHeightmapEditorMaxZExtent = 32.0f;
constexpr float kHeightmapEditorZScaleStep = 0.25f;

using PickPoint = RoomProjection::PickPoint;
using RoomProjection::ScreenToHeightmapPoint;

}  // namespace

wxDEFINE_EVENT(EVT_GPU_EDITOR_MODE_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_GPU_LAYER_OPACITY_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_GPU_LAYER_BLOCK_SELECT, wxCommandEvent);
wxDEFINE_EVENT(EVT_GPU_HEIGHTMAP_TARGET_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_GPU_OPEN_ROOM_ACTIONS, wxCommandEvent);
wxDEFINE_EVENT(EVT_GPU_OPEN_ROOM_SHOP, wxCommandEvent);

wxBEGIN_EVENT_TABLE(GLCanvas, wxGLCanvas)
    EVT_PAINT(GLCanvas::OnPaint)
    EVT_KEY_DOWN(GLCanvas::OnKeyDown)
    EVT_KEY_UP(GLCanvas::OnKeyUp)
    EVT_KILL_FOCUS(GLCanvas::OnKillFocus)
    EVT_MOTION(GLCanvas::OnMouseMove)
    EVT_LEFT_DOWN(GLCanvas::OnLeftDown)
    EVT_LEFT_DCLICK(GLCanvas::OnLeftDClick)
    EVT_LEFT_UP(GLCanvas::OnLeftUp)
    EVT_MIDDLE_DOWN(GLCanvas::OnMiddleDown)
    EVT_MIDDLE_UP(GLCanvas::OnMiddleUp)
    EVT_RIGHT_DOWN(GLCanvas::OnRightDown)
    EVT_RIGHT_UP(GLCanvas::OnRightUp)
    EVT_LEAVE_WINDOW(GLCanvas::OnMouseLeave)
    EVT_MOUSEWHEEL(GLCanvas::OnMouseWheel)
    EVT_SIZE(GLCanvas::OnSize)
    EVT_IDLE(GLCanvas::OnIdle)
wxEND_EVENT_TABLE()

GLCanvas::GLCanvas(wxWindow* parent, std::shared_ptr<GameData> gd)
    : wxGLCanvas(parent, wxID_ANY, GLCanvasAttributes, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE | wxWANTS_CHARS),
      m_gd(gd),
      m_keyboard_input(std::make_unique<GLCanvasKeyboardInput>(*this)),
      m_mouse_input(std::make_unique<GLCanvasMouseInput>(*this)),
      m_mapRenderer(gd),
      m_heightmapRenderer(gd),
      m_spriteRenderer(gd),
      m_room_info_overlay(*this)
{
    m_fps_stopwatch.Start();
    m_anim_stopwatch.Start();
    m_last_anim_ms = m_anim_stopwatch.Time();
    m_last_frame_ms = m_last_anim_ms;
}

GLCanvas::~GLCanvas() {
    if (m_gd) {
        PersistCurrentRoomEdits();
    }
    delete m_context;
}

void GLCanvas::SetRoomNum(uint16_t roomnum) {
    if (m_initialized && m_current_room == roomnum) {
        SetFocus();
        Refresh();
        return;
    }
    if (!m_initialized) {
        m_current_room = roomnum;
        Refresh();
        return;
    }
    LoadRoom(roomnum);
    SetFocus();
    Refresh();
}

void GLCanvas::SetHeightmapZScale(float scale) {
    float clamped = std::clamp(scale, 0.0f, 1.0f);
    clamped = std::round(clamped / kHeightmapEditorZScaleStep) * kHeightmapEditorZScaleStep;
    clamped = std::clamp(clamped, 0.0f, 1.0f);
    if (std::abs(m_heightmap_z_scale - clamped) < 0.001f) {
        return;
    }

    m_heightmap_z_scale = clamped;
    if (IsHeightmapEditMode()) {
        ApplyHeightmapEditZExtent();
        RefreshObjectPlacementsFromHeightmap();
        UpdateStatusBar();
        Refresh();
    }
}

void GLCanvas::AdjustHeightmapZScale(int delta) {
    SetHeightmapZScale(m_heightmap_z_scale + static_cast<float>(delta) * kHeightmapEditorZScaleStep);
}

void GLCanvas::SetAlpha(bool visible) {
    if (m_alpha == visible) {
        return;
    }
    m_alpha = visible;
    Refresh();
}

void GLCanvas::SetBackgroundOpacity(float opacity) {
    m_bg_opacity = std::clamp(opacity, 0.0f, 1.0f);
    m_mapRenderer.SetBackgroundOpacity(m_bg_opacity);
    Refresh();
}

void GLCanvas::SetForegroundOpacity(float opacity) {
    m_fg_opacity = std::clamp(opacity, 0.0f, 1.0f);
    m_mapRenderer.SetForegroundOpacity(m_fg_opacity);
    Refresh();
}

void GLCanvas::SetSpriteOpacity(float opacity) {
    m_sprite_opacity = std::clamp(opacity, 0.0f, 1.0f);
    m_spriteRenderer.SetOpacity(m_sprite_opacity);
    Refresh();
}

uint8_t GLCanvas::GetBackgroundOpacityByte() const {
    return OpacityToByte(m_bg_opacity);
}

uint8_t GLCanvas::GetForegroundOpacityByte() const {
    return OpacityToByte(m_fg_opacity);
}

uint8_t GLCanvas::GetSpriteOpacityByte() const {
    return OpacityToByte(m_sprite_opacity);
}

void GLCanvas::SetHeightmapVisible(bool visible) {
    if (m_show_heightmap == visible) {
        return;
    }
    m_show_heightmap = visible;
    Refresh();
}

void GLCanvas::SetEntitiesVisible(bool visible) {
    if (m_show_entities == visible) {
        return;
    }
    m_show_entities = visible;
    Refresh();
}

void GLCanvas::SetEntitiesHitboxVisible(bool visible) {
    if (m_show_hitboxes == visible) {
        return;
    }
    m_show_hitboxes = visible;
    Refresh();
}

void GLCanvas::SetWarpsVisible(bool visible) {
    if (m_show_warps == visible) {
        return;
    }
    m_show_warps = visible;
    Refresh();
}

void GLCanvas::SetTileSwapsVisible(bool visible) {
    if (m_show_tile_swaps == visible) {
        return;
    }
    m_show_tile_swaps = visible;
    Refresh();
}

void GLCanvas::SetLayerPriorityHighlight(bool enabled) {
    if (m_layer_priority_highlight == enabled) {
        return;
    }
    m_layer_priority_highlight = enabled;
    Refresh();

    wxWindow* target = EventTarget();
    if (target) {
        wxCommandEvent evt(EVT_GPU_EDITOR_MODE_CHANGE);
        evt.SetInt(static_cast<int>(m_editor_mode));
        evt.SetClientData(this);
        wxPostEvent(target, evt);
    }
}

void GLCanvas::ToggleLayerPriorityHighlight() {
    SetLayerPriorityHighlight(!m_layer_priority_highlight);
}

void GLCanvas::ResetLayerEditState() {
    m_layer_dragging_select = false;
    m_layer_selection_add = false;
    m_layer_selection_subtract = false;
    m_layer_selection_parallelogram = false;
    m_layer_selection_drag_base.clear();
    m_layer_dragging_selection_move = false;
    m_layer_selection_move_anchor_x = -1;
    m_layer_selection_move_anchor_y = -1;
    m_layer_selection_move_delta_x = 0;
    m_layer_selection_move_delta_y = 0;
    m_layer_selection_move_values.clear();
    m_layer_dragging_draw = false;
    m_layer_dragging_line = false;
    m_layer_draw_dirty = false;
    m_layer_last_draw_x = -1;
    m_layer_last_draw_y = -1;
    m_layer_line_start_x = -1;
    m_layer_line_start_y = -1;
    m_layer_line_end_x = -1;
    m_layer_line_end_y = -1;
    m_layer_line_preview_cells.clear();
}

void GLCanvas::ResetHeightmapEditState() {
    m_heightmap_dragging_select = false;
    m_heightmap_dragging_draw = false;
    m_heightmap_dragging_line = false;
    m_heightmap_dragging_selection_move = false;
    m_heightmap_draw_dirty = false;
    m_heightmap_selection_add = false;
    m_heightmap_selection_subtract = false;
    m_heightmap_last_draw_x = -1;
    m_heightmap_last_draw_y = -1;
    m_heightmap_line_start_x = -1;
    m_heightmap_line_start_y = -1;
    m_heightmap_line_end_x = -1;
    m_heightmap_line_end_y = -1;
    m_heightmap_selection_move_anchor_x = -1;
    m_heightmap_selection_move_anchor_y = -1;
    m_heightmap_selection_move_delta_x = 0;
    m_heightmap_selection_move_delta_y = 0;
    m_heightmap_line_preview_cells.clear();
    m_heightmap_selection_move_values.clear();
    m_heightmap_selection_drag_base.clear();
}

void GLCanvas::SetDrawingTool(DrawingTool tool) {
    if (m_drawing_tool == tool) {
        return;
    }
    if (m_layer_dragging_draw) {
        CommitLayerDrawStroke();
    }
    m_drawing_tool = tool;
    ResetHeightmapEditState();
    ResetLayerEditState();
    if (HasCapture()) {
        ReleaseMouse();
    }
    Refresh();
    wxWindow* target = EventTarget();
    if (target) {
        wxCommandEvent evt(EVT_GPU_EDITOR_MODE_CHANGE);
        evt.SetInt(static_cast<int>(m_editor_mode));
        evt.SetClientData(this);
        wxPostEvent(target, evt);
    }
}

void GLCanvas::LoadRoom(uint16_t roomnum) {
    LoadRoomFromGameData(roomnum, true, true);
}

void GLCanvas::NavigateToRoom(uint16_t roomnum) {
    if (!m_gd) {
        return;
    }
    LoadRoom(roomnum);
    SetFocus();
    NotifyRoomNavigationChanged();
}

void GLCanvas::ReloadCurrentRoomFromGameData() {
    LoadRoomFromGameData(m_current_room, false, false);
    Refresh();
}

void GLCanvas::CommitPendingEdits() {
    if (m_gd && m_initialized) {
        PersistCurrentRoomEdits();
    }
}

wxWindow* GLCanvas::EventTarget() const {
    for (wxWindow* window = GetParent(); window != nullptr; window = window->GetParent()) {
        if (dynamic_cast<RoomViewerFrame*>(window) != nullptr) {
            return window;
        }
    }
    return GetParent();
}

void GLCanvas::NotifyRoomNavigationChanged() {
    if (!m_gd) {
        return;
    }
    wxWindow* target = EventTarget();
    if (!target) {
        return;
    }

    auto room = m_gd->GetRoomData()->GetRoom(m_current_room);
    if (!room) {
        return;
    }

    wxCommandEvent evt(EVT_GO_TO_NAV_ITEM);
    evt.SetString(wxString(L"Rooms/") + room->GetDisplayName());
    evt.SetInt(static_cast<int>(m_current_room));
    evt.SetClientData(this);
    wxPostEvent(target, evt);

    wxCommandEvent props_evt(EVT_PROPERTIES_UPDATE);
    props_evt.SetClientData(target);
    wxPostEvent(target, props_evt);
}

void GLCanvas::NotifyHeightmapChanged(bool /*moved*/) {
    wxWindow* target = EventTarget();
    if (!target) {
        return;
    }

    wxCommandEvent props_evt(EVT_PROPERTIES_UPDATE);
    props_evt.SetClientData(target);
    wxPostEvent(target, props_evt);
}

void GLCanvas::NotifyHeightmapTargetChanged() {
    wxWindow* target = EventTarget();
    if (!target) {
        return;
    }

    wxCommandEvent evt(EVT_GPU_HEIGHTMAP_TARGET_CHANGE);
    evt.SetClientData(this);
    wxPostEvent(target, evt);
}

void GLCanvas::NotifyLayerOpacityChanged() {
    wxWindow* target = EventTarget();
    if (!target) {
        return;
    }

    wxCommandEvent evt(EVT_GPU_LAYER_OPACITY_CHANGE);
    evt.SetClientData(this);
    wxPostEvent(target, evt);
}

void GLCanvas::NotifyLayerBlockSelected() {
    if (!IsLayerEditMode() || !m_background_has_selection) {
        return;
    }

    wxWindow* target = EventTarget();
    if (!target) {
        return;
    }

    wxCommandEvent evt(EVT_GPU_LAYER_BLOCK_SELECT);
    evt.SetInt(static_cast<int>(SelectedBackgroundBlockId()));
    evt.SetClientData(this);
    wxPostEvent(target, evt);
}

void GLCanvas::LoadRoomFromGameData(uint16_t roomnum, bool persist_edits, bool center_camera) {
    if (!m_gd) {
        return;
    }
    const bool room_changed = !m_initialized || m_current_room != roomnum;
    if (persist_edits && m_initialized) {
        PersistCurrentRoomEdits();
    }
    // Cancel any in-progress insertion or drag before switching rooms.
    GLCanvasObjectCoordinator objects(*this);
    objects.PrepareForRoomLoad();
    GLCanvasRoomMode(*this).CancelActiveDrag();
    ResetHeightmapEditState();
    ResetLayerEditState();
    m_current_room = roomnum;
    if (room_changed && !m_restoring_history) {
        ClearUndoRedoHistory();
    }
    m_mapRenderer.LoadRoom(roomnum);
    m_heightmapRenderer.LoadRoom(roomnum);
    m_spriteRenderer.LoadRoom(roomnum);
    if (room_changed) {
        m_background_has_selection = false;
        m_background_selected_x = 0;
        m_background_selected_y = 0;
        m_heightmap_selection_anchor_x = 0;
        m_heightmap_selection_anchor_y = 0;
        m_heightmap_selection_drag_anchor_x = 0;
        m_heightmap_selection_drag_anchor_y = 0;
        m_heightmap_selected_cells.clear();
        m_heightmap_clipboard_valid = false;
        m_heightmap_clipboard_cell = 0;
        NotifyHeightmapTargetChanged();
    }
    objects.LoadRoomObjects(roomnum);
    if (center_camera) {
        CenterCameraOnRoom();
    }
    ClampBackgroundSelection();
    UpdateStatusBar();
}

void GLCanvas::UpdateStatusBar() {
    wxWindow* target = EventTarget();
    if (!target) {
        return;
    }

    auto name = m_gd->GetRoomData()->GetRoomDisplayName(m_current_room);
    const char* mode_name = "ROOM";
    if (m_editor_mode == EditorMode::BackgroundLayer) {
        mode_name = "BG_EDIT";
    } else if (m_editor_mode == EditorMode::ForegroundLayer) {
        mode_name = "FG_EDIT";
    } else if (m_editor_mode == EditorMode::Heightmap) {
        mode_name = "HM_EDIT";
    }

    auto map = CurrentRoomMap();
    auto describe_hm_cell = [&](int cell_x, int cell_y) {
        uint16_t value = map->GetHeightmapCell({cell_x, cell_y});
        return wxString::Format("%d,%d Z:%X R:%X T:%02X",
            cell_x, cell_y, (value >> 8) & 0x0F, (value >> 12) & 0x0F, value & 0xFF);
    };

    wxString hover_text = "HOVER: -";
    wxString selection_text = "SEL: -";
    if (map) {
        if (IsLayerEditMode()) {
            if (m_background_has_hover) {
                hover_text = wxString::Format("HOVER: %d,%d", m_background_hover_x, m_background_hover_y);
            }
            if (m_background_has_selection) {
                selection_text = wxString::Format("SEL: %d,%d", m_background_selected_x, m_background_selected_y);
            }
        } else {
            int hover_x = m_heightmapRenderer.GetHoverX();
            int hover_y = m_heightmapRenderer.GetHoverY();
            if (hover_x >= 0 && hover_y >= 0 &&
                hover_x < map->GetHeightmapWidth() && hover_y < map->GetHeightmapHeight()) {
                hover_text = "HOVER: " + describe_hm_cell(hover_x, hover_y);
            }
            if (IsHeightmapEditMode() && HasSelectedHeightmapCell()) {
                selection_text = "SEL: " + describe_hm_cell(PrimaryHeightmapCellX(), PrimaryHeightmapCellY());
                if (m_heightmap_selected_cells.size() > 1) {
                    selection_text << wxString::Format(" (+%zu)", m_heightmap_selected_cells.size() - 1);
                }
            }
        }
    }

    const char* occlusion_names[] = {"TOP", "GHOST", "HIDE"};
    const wxString field_text[] = {
        wxString::Format("MODE: %s - Room %d (%ls)", mode_name, m_current_room, name.c_str()),
        hover_text,
        selection_text,
        wxString::Format("FPS: %.2f | Entities: %zu | Cam: %.0f, %.0f | HM: %s %.0f | BG: %.1f FG: %.1f SPR: %.1f | OCC: %s BOX: %s",
            m_fps, m_instances.size(), m_cam_x, m_cam_y,
            m_show_heightmap ? "ON" : "OFF", m_heightmapRenderer.GetZExtent(),
            m_bg_opacity, m_fg_opacity, m_sprite_opacity,
            occlusion_names[m_entity_occlusion_idx % 3], m_show_hitboxes ? "ON" : "OFF")
    };
    for (int field = 0; field < 4; ++field) {
        wxCommandEvent evt(EVT_STATUSBAR_UPDATE);
        evt.SetString(field_text[field]);
        evt.SetInt(field);
        evt.SetClientData(target);
        wxPostEvent(target, evt);
    }
}

void GLCanvas::OnSize(wxSizeEvent& evt) {
    if (m_initialized) {
        CenterCameraOnRoom();
        Refresh();
    }
    evt.Skip();
}

bool GLCanvas::IsLayerEditMode() const {
    return m_editor_mode == EditorMode::BackgroundLayer || m_editor_mode == EditorMode::ForegroundLayer;
}

bool GLCanvas::IsHeightmapEditMode() const {
    return m_editor_mode == EditorMode::Heightmap;
}

bool GLCanvas::IsAnyEditMode() const {
    return IsLayerEditMode() || IsHeightmapEditMode();
}

Tilemap3D::Layer GLCanvas::CurrentEditLayer() const {
    return m_editor_mode == EditorMode::ForegroundLayer ? Tilemap3D::Layer::FG : Tilemap3D::Layer::BG;
}

void GLCanvas::ApplyHeightmapEditZExtent() {
    m_heightmapRenderer.SetZExtent(m_heightmap_z_scale * kHeightmapEditorMaxZExtent);
}

void GLCanvas::SetEditorMode(EditorMode mode) {
    if (m_editor_mode == mode) {
        return;
    }

    if (m_editor_mode != EditorMode::Heightmap && mode == EditorMode::Heightmap) {
        m_non_heightmap_z_extent = m_heightmapRenderer.GetZExtent();
        ApplyHeightmapEditZExtent();
    } else if (m_editor_mode == EditorMode::Heightmap && mode != EditorMode::Heightmap) {
        m_heightmapRenderer.SetZExtent(m_non_heightmap_z_extent);
    }

    m_editor_mode = mode;
    if (IsAnyEditMode()) {
        // Entering heightmap/layer modes should start with no active cell selection.
        m_background_has_selection = false;
        m_heightmap_selected_cells.clear();
        m_heightmap_selection_drag_base.clear();
        m_layer_selection_drag_base.clear();
        m_layer_selection_move_values.clear();
        m_heightmap_selection_move_values.clear();
        NotifyHeightmapTargetChanged();
        if (m_drawing_tool != DrawingTool::Select) {
            SetDrawingTool(DrawingTool::Select);
        }
        if (IsHeightmapEditMode()) {
            m_heightmapRenderer.SetHoverPoint(ScreenToWorldX(m_last_mouse_pos.x), ScreenToWorldY(m_last_mouse_pos.y));
        } else {
            m_heightmapRenderer.ClearHover();
        }
        if (!IsLayerEditMode()) {
            m_background_has_hover = false;
        }
        SetCursor(wxCursor(wxCURSOR_ARROW));
    }
    UpdateStatusBar();
    Refresh();

    wxCommandEvent evt(EVT_GPU_EDITOR_MODE_CHANGE);
    evt.SetInt(static_cast<int>(m_editor_mode));
    evt.SetClientData(this);
    wxWindow* target = EventTarget();
    if (target) {
        wxPostEvent(target, evt);
    }
}

bool GLCanvas::HeightmapCellAt(const wxPoint& point, int& cell_x, int& cell_y) {
    auto map = CurrentRoomMap();
    if (!map || point == wxDefaultPosition) {
        return false;
    }

    m_heightmapRenderer.SetHoverPoint(ScreenToWorldX(point.x), ScreenToWorldY(point.y));
    int hover_x = m_heightmapRenderer.GetHoverX();
    int hover_y = m_heightmapRenderer.GetHoverY();
    if (hover_x < 0 || hover_y < 0 || hover_x >= map->GetHeightmapWidth() || hover_y >= map->GetHeightmapHeight()) {
        return false;
    }

    cell_x = hover_x;
    cell_y = hover_y;
    return true;
}

bool GLCanvas::HeightmapVirtualCellAt(const wxPoint& point, int& cell_x, int& cell_y) const {
    auto map = CurrentRoomMap();
    if (!map || point == wxDefaultPosition) {
        return false;
    }

    PickPoint map_point = ScreenToHeightmapPoint(
        ScreenToWorldX(point.x),
        ScreenToWorldY(point.y),
        static_cast<float>(m_mapRenderer.GetRoomLeft()),
        static_cast<float>(m_mapRenderer.GetRoomTop()));
    cell_x = static_cast<int>(std::floor(map_point.x));
    cell_y = static_cast<int>(std::floor(map_point.y));
    return true;
}

bool GLCanvas::BackgroundCellAt(const wxPoint& point, int& cell_x, int& cell_y) const {
    auto map = CurrentRoomMap();
    if (!map || point == wxDefaultPosition) {
        return false;
    }

    float zoom = std::max(ZoomFactor(), 0.0001f);
    float cell_w = 32.0f * zoom;
    float cell_h = 32.0f * zoom;
    float x_offset = CurrentEditLayer() == Tilemap3D::Layer::FG ? -32.0f : 0.0f;

    bool found = false;
    int best_x = -1;
    int best_y = -1;

    // Match draw order so overlap resolves to the top-most rendered cell.
    for (int y = 0; y < m_mapRenderer.GetRoomHeight(); ++y) {
        for (int x = 0; x < m_mapRenderer.GetRoomWidth(); ++x) {
            float world_x = 32.0f * static_cast<float>(x) - 32.0f * static_cast<float>(y) + 512.0f + x_offset;
            float world_y = 16.0f * static_cast<float>(x) + 16.0f * static_cast<float>(y) + 100.0f;
            float left = world_x * zoom + m_cam_x;
            float top = world_y * zoom + m_cam_y;
            float right = left + cell_w;
            float bottom = top + cell_h;
            if (static_cast<float>(point.x) >= left &&
                static_cast<float>(point.x) <= right &&
                static_cast<float>(point.y) >= top &&
                static_cast<float>(point.y) <= bottom) {
                found = true;
                best_x = x;
                best_y = y;
            }
        }
    }

    if (!found) {
        return false;
    }

    cell_x = best_x;
    cell_y = best_y;
    return true;
}

bool GLCanvas::BackgroundVirtualCellAt(const wxPoint& point, int& cell_x, int& cell_y) const {
    auto map = CurrentRoomMap();
    if (!map || point == wxDefaultPosition) {
        return false;
    }

    float x_offset = CurrentEditLayer() == Tilemap3D::Layer::FG ? -32.0f : 0.0f;
    float world_x = ScreenToWorldX(point.x) - x_offset - 16.0f;
    float world_y = ScreenToWorldY(point.y) - 16.0f;
    float a = (world_x - 512.0f) / 32.0f;
    float b = (world_y - 100.0f) / 16.0f;
    cell_x = static_cast<int>(std::round((a + b) * 0.5f));
    cell_y = static_cast<int>(std::round((b - a) * 0.5f));
    return true;
}

