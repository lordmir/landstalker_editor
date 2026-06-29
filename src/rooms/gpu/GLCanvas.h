#ifndef GL_CANVAS_H
#define GL_CANVAS_H

#include "GLLoader.h"
#include <wx/wx.h>
#include <wx/glcanvas.h>
#include <wx/stopwatch.h>
#include <memory>
#include <vector>
#include <map>
#include <set>
#include <landstalker/main/GameData.h>
#include <landstalker/rooms/WarpList.h>
#include "SpriteInstance.h"
#include "MapRenderer.h"
#include "HeightmapRenderer.h"
#include "SpriteRenderer.h"

wxDECLARE_EVENT(EVT_GPU_EDITOR_MODE_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(EVT_GPU_LAYER_OPACITY_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(EVT_GPU_LAYER_BLOCK_SELECT, wxCommandEvent);

struct WarpInstance {
    uint32_t instance_id;
    uint32_t warp_key;
    Landstalker::WarpList::Warp warp;
    bool current_room_is_room1;
    float x;
    float y;
    float width;
    float height;
    float floor_z;
    float z_extent;
    float room_left;
    float room_top;

    uint16_t DestinationRoom() const { return current_room_is_room1 ? warp.room2 : warp.room1; }
};

class MyGLCanvas : public wxGLCanvas {
public:
    enum class EditorMode {
        Room,
        BackgroundLayer,
        ForegroundLayer,
        Heightmap
    };

    MyGLCanvas(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd);
    virtual ~MyGLCanvas();

    void SetRoomNum(uint16_t roomnum);
    void LoadRoom(uint16_t roomnum);
    void ReloadCurrentRoomFromGameData();
    void CommitPendingEdits();
    void SelectEntityByIndex(int selection);
    void SelectWarpByIndex(int selection);
    void SelectTileSwapByIndex(int selection);
    void SelectDoorByIndex(int selection);
    int GetSelectedEntityIndex() const { return SelectedEntityListIndex(); }
    int GetSelectedWarpIndex() const { return SelectedWarpListIndex(); }
    int GetSelectedTileSwapIndex() const { return SelectedTileSwapListIndex(); }
    int GetSelectedDoorIndex() const { return SelectedDoorListIndex(); }
    bool HasObjectSelection() const {
        return GetSelectedEntityIndex() > 0 || GetSelectedWarpIndex() > 0 ||
               GetSelectedTileSwapIndex() > 0 || GetSelectedDoorIndex() > 0;
    }
    void SetZoom(double zoom);
    void SetAlpha(bool visible);
    bool GetAlpha() const { return m_alpha; }
    void SetBackgroundOpacity(float opacity);
    void SetForegroundOpacity(float opacity);
    void SetSpriteOpacity(float opacity);
    uint8_t GetBackgroundOpacityByte() const;
    uint8_t GetForegroundOpacityByte() const;
    uint8_t GetSpriteOpacityByte() const;
    void SetHeightmapVisible(bool visible);
    void SetEntitiesVisible(bool visible);
    bool GetEntitiesVisible() const { return m_show_entities; }
    void SetEntitiesHitboxVisible(bool visible);
    bool GetEntitiesHitboxVisible() const { return m_show_hitboxes; }
    void SetWarpsVisible(bool visible);
    bool GetWarpsVisible() const { return m_show_warps; }
    void SetTileSwapsVisible(bool visible);
    bool GetTileSwapsVisible() const { return m_show_tile_swaps; }
    void SetEditorMode(EditorMode mode);
    EditorMode GetEditorMode() const { return m_editor_mode; }
    void SetLayerPriorityHighlight(bool enabled);
    bool GetLayerPriorityHighlight() const { return m_layer_priority_highlight; }
    void ToggleLayerPriorityHighlight();
    bool HandleKeyDown(wxKeyEvent& evt);
    void SetSelectedBlockId(int block);
    bool HasSelectedLayerCell() const;
    bool HasSelectedHeightmapCell() const;
    bool CanInsertSelectedHeightmapRow() const;
    bool CanInsertSelectedHeightmapColumn() const;
    bool CanDeleteSelectedHeightmapRow() const;
    bool CanDeleteSelectedHeightmapColumn() const;
    bool CanIncreaseSelectedHeightmapHeight() const;
    bool CanDecreaseSelectedHeightmapHeight() const;
    bool CanDeleteSelectedTilemapRow() const;
    bool CanDeleteSelectedTilemapColumn() const;
    uint8_t GetSelectedHeightmapType() const;
    bool IsSelectedHeightmapPlayerPassable() const;
    bool IsSelectedHeightmapNpcPassable() const;
    bool IsSelectedHeightmapRaftTrack() const;
    void SetSelectedHeightmapType(uint8_t type);
    void ToggleSelectedHeightmapPlayerPassable();
    void ToggleSelectedHeightmapNpcPassable();
    void ToggleSelectedHeightmapRaftTrack();
    void AdjustSelectedHeightmapHeight(int delta);
    bool CanNudgeHeightmap(int left_delta, int top_delta) const;
    void NudgeHeightmap(int left_delta, int top_delta);
    void InsertSelectedHeightmapRowBefore();
    void InsertSelectedHeightmapRowAfter();
    void DeleteSelectedHeightmapRow();
    void InsertSelectedHeightmapColumnBefore();
    void InsertSelectedHeightmapColumnAfter();
    void DeleteSelectedHeightmapColumn();
    void ClearCurrentTilemap();
    void InsertSelectedTilemapRowBefore();
    void InsertSelectedTilemapRowAfter();
    void DeleteSelectedTilemapRow();
    void InsertSelectedTilemapColumnBefore();
    void InsertSelectedTilemapColumnAfter();
    void DeleteSelectedTilemapColumn();
    bool OpenSelectedObjectProperties();
    void AddEntity();
    void DeleteSelectedObject();
    void ReorderSelectedObject(int delta);
    void AddWarpHalf();

private:

    enum class HeightmapViewMode {
        Flat = 1,
        Raised = 2,
        Full = 3,
        FullWithTilemap = 4
    };

    friend class GLCanvasEntityEditor;
    friend class GLCanvasWarpEditor;
    friend class GLCanvasTileDoorEditor;
    friend class GLCanvasObjectCoordinator;
    friend class GLCanvasHeightmapMode;
    friend class GLCanvasLayerEditMode;
    friend class GLCanvasRoomMode;

    void OnTimer(wxTimerEvent& evt);
    void OnKeyDown(wxKeyEvent& evt);
    void OnMouseMove(wxMouseEvent& evt);
    void OnLeftDown(wxMouseEvent& evt);
    void OnLeftDClick(wxMouseEvent& evt);
    void OnLeftUp(wxMouseEvent& evt);
    void OnMiddleDown(wxMouseEvent& evt);
    void OnMiddleUp(wxMouseEvent& evt);
    void OnRightDown(wxMouseEvent& evt);
    void OnRightUp(wxMouseEvent& evt);
    void OnMouseLeave(wxMouseEvent& evt);
    void OnMouseWheel(wxMouseEvent& evt);
    void OnSize(wxSizeEvent& evt);
    void OnPaint(wxPaintEvent& evt);
    void LoadRoomFromGameData(uint16_t roomnum, bool persist_edits, bool center_camera);
    void NavigateToRoom(uint16_t roomnum);
    void NotifyRoomNavigationChanged();
    void ClearObjectSelection();
    void NotifySelectionChanged();
    void NotifyRoomDataChanged(bool entities, bool warps, bool swaps, bool doors);
    void NotifyHeightmapChanged(bool moved);
    void NotifyLayerOpacityChanged();
    void NotifyLayerBlockSelected();
    wxWindow* EventTarget() const;
    bool SelectObjectAt(const wxPoint& point);
    void CancelActiveDrag();
    int SelectedEntityListIndex() const;
    int SelectedWarpListIndex() const;
    int SelectedTileSwapListIndex() const;
    int SelectedDoorListIndex() const;
    int HitTestEntity(const wxPoint& point) const;
    int HitTestEntityBody(const wxPoint& point) const;
    int HitTestEntityZControl(const wxPoint& point) const;
    int HitTestWarp(const wxPoint& point) const;
    int HitTestWarpResizeControl(const wxPoint& point) const;
    int HitTestTileSwapRegion(const wxPoint& point) const;
    int HitTestTileSwapRegionResizeControl(const wxPoint& point) const;
    int HitTestDoor(const wxPoint& point) const;
    int HitTestRoomInfoLink(const wxPoint& point) const;
    void StartEntityDrag(int entity_idx, const wxMouseEvent& evt, bool z_axis_only, bool shadow_drag = false);
    void UpdateEntityDrag(const wxMouseEvent& evt);
    void EndEntityDrag();
    void StartWarpDrag(int warp_idx, const wxMouseEvent& evt);
    void StartWarpResizeDrag(int warp_idx, int axis, const wxMouseEvent& evt);
    void UpdateWarpDrag(const wxMouseEvent& evt);
    void EndWarpDrag();
    void StartDoorDrag(int door_idx, const wxMouseEvent& evt);
    void UpdateDoorDrag(const wxMouseEvent& evt);
    void EndDoorDrag();
    void StartTileSwapRegionDrag(int region_idx, int resize_axis, const wxMouseEvent& evt);
    void UpdateTileSwapRegionDrag(const wxMouseEvent& evt);
    void EndTileSwapRegionDrag();
    void UpdateEntityProjection(SpriteInstance& inst);
    void UpdateWarpFloor(WarpInstance& warp);
    void CenterCameraOnRoom();
    void FocusCameraOnSelectedObjectIfNeeded();
    void EnsureWorldRectVisible(float min_x, float min_y, float max_x, float max_y);
    void PersistCurrentRoomEdits();
    void RefreshEntityMetadata(SpriteInstance& inst);
    void CopySelectedEntity();
    void PasteEntity();
    void CutSelectedEntity();
    void SelectNextObject(int direction);
    void SelectNextTileSwapRegion(int direction);
    void CycleSelectedEntityId(int delta);
    void CycleSelectedEntityPalette();
    void SetSelectedEntityOrientation(Landstalker::Orientation orientation);
    void SetSelectedEntityToFloor();
    void AddDoor();
    void AddTileSwap();
    std::pair<int, int> MouseHeightmapCell() const;
    std::pair<float, float> FindNearestFreeWarpCell(float preferred_x, float preferred_y) const;
    void ResizeSelectedWarp(float dx, float dy);
    void RotateSelectedWarp(float dx, float dy);
    void CycleSelectedWarpType(int delta);
    void CycleSelectedDoorSize(int delta);
    void CycleSelectedTileSwapShape(int delta);
    void CycleSelectedTileSwapId(int delta);
    void ResizeSelectedTileSwapRegion(float dx, float dy);
    void ToggleSelectedTileSwapPreview();
    void ToggleSelectedDoorPreview();
    void ClearTileSwapPreview();
    void NudgeSelectedObject(float dx, float dy, float dz);
    void RenderWarps();
    void RenderDoors();
    void RenderTileSwapOutlines();
    void RenderEntityControls();
    void RenderSelectedEntityTooltip();
    void RenderSelectedWarpTooltip();
    void RenderSelectedDoorTooltip();
    void RenderSelectedTileSwapRegionTooltip();
    void RenderRoomInfoTable(int width, int height);
    void WriteSelectedEntityOcclusionDebugLog();
    void WriteEntityDrawOrderDebugLog();
    float FloorUnderRect(float min_x, float min_y, float max_x, float max_y) const;
    float FloorUnderPoint(float x, float y) const;
    bool ShadowOccludedByHeightmap(float min_x, float min_y, float max_x, float max_y, float z) const;
    bool EntityCollidesWithHeightmap(const SpriteInstance& inst) const;
    float FloorUnderHitbox(float center_x, float center_y, float half_base) const;
    int FindInstanceIndex(uint32_t instance_id) const;
    int FindWarpIndex(uint32_t instance_id) const;
    void RefreshObjectPlacementsFromHeightmap();
    bool IsLayerEditMode() const;
    bool IsHeightmapEditMode() const;
    bool IsAnyEditMode() const;
    Landstalker::Tilemap3D::Layer CurrentEditLayer() const;
    void ApplyHeightmapViewMode();
    bool BackgroundCellAt(const wxPoint& point, int& cell_x, int& cell_y) const;
    bool SelectBackgroundCellAt(const wxPoint& point);
    bool SelectHeightmapCellAt(const wxPoint& point);
    void ClampBackgroundSelection();
    void MoveBackgroundSelection(int dx, int dy);
    std::shared_ptr<Landstalker::Tilemap3D> CurrentRoomMap() const;
    int SelectedBackgroundBlockIndex() const;
    uint16_t SelectedBackgroundBlockId() const;
    int SelectedHeightmapCellX() const;
    int SelectedHeightmapCellY() const;
    uint16_t SelectedHeightmapCellValue() const;
    void CopySelectedBackgroundBlock();
    void CopySelectedHeightmapCell();
    void ClearBackgroundClipboard();
    void PasteSelectedBackgroundBlock();
    void PasteSelectedHeightmapCell();
    void ReloadCurrentRoomMapView();
    void RenderBackgroundEditorOverlay(int width, int height);
    void RenderHeightmapEditorOverlay(int width, int height);
    float ZoomFactor() const;
    float ScreenToWorldX(int screen_x) const;
    float ScreenToWorldY(int screen_y) const;
    void PanCameraByStep(int dx, int dy, float speed = 20.0f);
    void ChangeZoomStep(int delta, float anchor_x, float anchor_y);
    void ResizeSelectedTileSwapByDelta(int dw, int dh);
    void UpdateStatusBar();
    void RenderStencilOverlay(int width, int height, GLint ref, GLint mask, float r, float g, float b, float a) const;
    std::set<uint32_t> FindCollidedEntityIds() const;

    std::shared_ptr<Landstalker::GameData> m_gd;
    wxGLContext* m_context;
    
    MapRenderer m_mapRenderer;
    HeightmapRenderer m_heightmapRenderer;
    SpriteRenderer m_spriteRenderer;

    std::vector<SpriteInstance> m_instances;
    std::vector<WarpInstance> m_warps;
    std::vector<Landstalker::Entity> m_room_entities;
    bool m_pending_warp_half;
    uint16_t m_pending_warp_room;
    uint32_t m_pending_warp_instance_id;
    Landstalker::WarpList::Warp m_pending_warp;
    bool m_entity_clipboard_valid;
    Landstalker::Entity m_entity_clipboard;
    wxTimer m_timer;
    wxStopWatch m_fps_stopwatch;
    wxStopWatch m_room_stopwatch;
    long m_frame_count;
    float m_fps;
    bool m_alpha;
    bool m_show_heightmap;
    bool m_show_entities;
    bool m_show_warps;
    bool m_show_tile_swaps;
    int m_hovered_entity_idx;
    int m_selected_entity_idx;
    int m_hovered_warp_idx;
    int m_selected_warp_idx;
    int m_hovered_tileswap_region_idx;
    int m_selected_tileswap_region_idx;
    int m_hovered_door_idx;
    int m_selected_door_idx;
    bool m_dragging_entity;
    bool m_drag_z_axis_only;
    uint32_t m_drag_instance_id;
    wxPoint m_drag_start_mouse;
    float m_drag_start_x;
    float m_drag_start_y;
    float m_drag_start_z;
    float m_drag_plane_z;
    float m_drag_cursor_offset_x;
    float m_drag_cursor_offset_y;
    bool m_drag_floor_snap;
    bool m_dragging_warp;
    uint32_t m_drag_warp_instance_id;
    int m_drag_warp_resize_axis;
    float m_drag_warp_start_x;
    float m_drag_warp_start_y;
    float m_drag_warp_start_width;
    float m_drag_warp_start_height;
    float m_drag_warp_start_floor_z;
    bool m_dragging_door;
    int m_drag_door_idx;
    int m_drag_door_start_x;
    int m_drag_door_start_y;
    bool m_dragging_tileswap_region;
    int m_drag_tileswap_region_idx;
    int m_drag_tileswap_resize_axis;
    int m_drag_tileswap_start_x;
    int m_drag_tileswap_start_y;
    int m_drag_tileswap_start_width;
    int m_drag_tileswap_start_height;
    bool m_dragging_pan;
    wxPoint m_drag_pan_start_mouse;
    float m_drag_pan_start_cam_x;
    float m_drag_pan_start_cam_y;
    int m_bg_opacity_idx;
    int m_fg_opacity_idx;
    int m_sprite_opacity_idx;
    int m_entity_occlusion_idx;
    bool m_debug_occlusion;
    bool m_show_hitboxes;
    EditorMode m_editor_mode;
    HeightmapViewMode m_heightmap_view_mode;
    float m_non_heightmap_z_extent;
    bool m_foreground_show_background_underlay;
    bool m_background_show_block_ids;
    bool m_layer_priority_highlight;
    bool m_background_has_selection;
    int m_background_selected_x;
    int m_background_selected_y;
    bool m_background_has_hover;
    int m_background_hover_x;
    int m_background_hover_y;
    bool m_background_clipboard_valid;
    uint16_t m_background_clipboard_block_id;
    bool m_heightmap_clipboard_valid;
    uint16_t m_heightmap_clipboard_cell;
    bool m_tileswap_preview_active;
    int m_tileswap_preview_swap_index;
    bool m_door_preview_active;
    int m_door_preview_idx;
    std::shared_ptr<Landstalker::Tilemap3D> m_tileswap_preview_map;
    uint16_t m_current_room;
    float m_cam_x, m_cam_y;
    int m_zoom_step_idx;
    bool m_gl_init_failed;
    bool m_initialized;
    wxPoint m_last_mouse_pos;
    struct RoomInfoLink {
        wxRect rect;
        uint16_t room;
    };
    std::vector<RoomInfoLink> m_room_info_links;

    wxDECLARE_EVENT_TABLE();
};

#endif // GL_CANVAS_H
