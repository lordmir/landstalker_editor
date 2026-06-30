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
#include <landstalker/3d_maps/Doors.h>
#include <landstalker/3d_maps/TileSwaps.h>
#include <landstalker/rooms/WarpList.h>
#include "SpriteInstance.h"
#include "MapRenderer.h"
#include "HeightmapRenderer.h"
#include "SpriteRenderer.h"

wxDECLARE_EVENT(EVT_GPU_EDITOR_MODE_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(EVT_GPU_LAYER_OPACITY_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(EVT_GPU_LAYER_BLOCK_SELECT, wxCommandEvent);
wxDECLARE_EVENT(EVT_GPU_HEIGHTMAP_TARGET_CHANGE, wxCommandEvent);

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

    enum class DrawingTool {
        Select,
        Draw,
        Line,
        FilledRect,
        OutlineRect,
        FilledCircle,
        OutlineCircle,
        FloodFill,
        Stamp,
        Clear
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
    float GetHeightmapZScale() const { return m_heightmap_z_scale; }
    void SetHeightmapZScale(float scale);
    void AdjustHeightmapZScale(int delta);
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
    void SetDrawingTool(DrawingTool tool);
    DrawingTool GetDrawingTool() const { return m_drawing_tool; }
    void SetLayerPriorityHighlight(bool enabled);
    bool GetLayerPriorityHighlight() const { return m_layer_priority_highlight; }
    void ToggleLayerPriorityHighlight();
    bool HandleKeyDown(wxKeyEvent& evt);
    bool CanUndo() const;
    bool CanRedo() const;
    void Undo();
    void Redo();
    void CaptureObjectUndoState();
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
    bool HasHeightmapEditTarget() const;
    uint8_t GetSelectedHeightmapType() const;
    bool IsSelectedHeightmapPlayerPassable() const;
    bool IsSelectedHeightmapNpcPassable() const;
    bool IsSelectedHeightmapRaftTrack() const;
    void SetSelectedHeightmapType(uint8_t type);
    void ToggleSelectedHeightmapPlayerPassable();
    void ToggleSelectedHeightmapNpcPassable();
    void ToggleSelectedHeightmapRaftTrack();
    void AdjustSelectedHeightmapHeight(int delta);
    void ClearSelectedHeightmapCells();
    void ClearSelectedLayerCells();
    void AdjustSelectedBlockId(int delta);
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
    void AddDoor();
    void AddTileSwap();

private:

    enum class HeightmapViewMode {
        Flat = 1,
        Raised = 2,
        Full = 3,
        FullWithTilemap = 4
    };

    enum class PendingObjectAddType {
        None,
        Entity,
        Warp,
        Door,
        TileSwap
    };

    enum class PendingTileSwapPart {
        MapSource,
        MapDestination,
        HeightmapSource,
        HeightmapDestination
    };

    struct ObjectUndoState {
        std::vector<Landstalker::Entity> entities;
        std::vector<Landstalker::WarpList::Warp> warps;
        std::vector<Landstalker::TileSwap> swaps;
        std::vector<Landstalker::Door> doors;
        bool pending_warp_half;
        uint16_t pending_warp_room;
        uint32_t pending_warp_instance_id;
        Landstalker::WarpList::Warp pending_warp;
        int selected_entity;
        int selected_warp;
        int selected_tileswap;
        int selected_door;
    };

    friend class GLCanvasEntityEditor;
    friend class GLCanvasWarpEditor;
    friend class GLCanvasTileDoorEditor;
    friend class GLCanvasObjectCoordinator;
    friend class GLCanvasHeightmapMode;
    friend class GLCanvasLayerEditMode;
    friend class GLCanvasRoomMode;

    void OnIdle(wxIdleEvent& evt);
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
    void NotifyHeightmapTargetChanged();
    void NotifyLayerOpacityChanged();
    void NotifyLayerBlockSelected();
    wxWindow* EventTarget() const;
    bool SelectObjectAt(const wxPoint& point);
    void CancelActiveDrag();
    void CaptureUndoState();
    void RestoreUndoState(const std::shared_ptr<Landstalker::Tilemap3D>& state);
    bool IsObjectHistoryMode() const;
    std::vector<Landstalker::Entity> BuildCurrentRoomEntities() const;
    std::vector<Landstalker::WarpList::Warp> BuildCurrentRoomWarps() const;
    ObjectUndoState BuildObjectUndoState() const;
    void RestoreObjectUndoState(const ObjectUndoState& state);
    void ClearUndoRedoHistory();
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
    bool HasPendingObjectAdd() const;
    void UpdatePendingObjectAddHover();
    void CommitPendingObjectAdd();
    void CancelPendingObjectAdd();
    void RenderPendingObjectAddOverlay();
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
    bool HeightmapCellAt(const wxPoint& point, int& cell_x, int& cell_y);
    bool SelectBackgroundCellAt(const wxPoint& point);
    bool SelectHeightmapCellAt(const wxPoint& point);
    void ClearEditSelection();
    void BeginLayerSelectionDrag(int x, int y, bool add_to_selection, bool subtract_from_selection, bool parallelogram_selection);
    void UpdateLayerSelectionDrag(int x, int y);
    void FinishLayerSelectionDrag();
    bool IsLayerCellSelected(int x, int y) const;
    void BeginLayerSelectionMoveDrag(int x, int y);
    void UpdateLayerSelectionMoveDrag(int x, int y);
    void CommitLayerSelectionMoveDrag();
    void CancelLayerSelectionMoveDrag();
    std::pair<int, int> SnapLayerLineEnd(int start_x, int start_y, int end_x, int end_y) const;
    void BeginLayerLineDrag(int x, int y, bool shift_down);
    void UpdateLayerLineDrag(int x, int y, bool shift_down);
    void CommitLayerLineDrag();
    void CancelLayerLineDrag();
    void BeginHeightmapSelectionDrag(int x, int y, bool add_to_selection, bool subtract_from_selection);
    void UpdateHeightmapSelectionDrag(int x, int y);
    void FinishHeightmapSelectionDrag();
    bool IsHeightmapCellSelected(int x, int y) const;
    void BeginHeightmapSelectionMoveDrag(int x, int y);
    void UpdateHeightmapSelectionMoveDrag(int x, int y);
    void CommitHeightmapSelectionMoveDrag();
    void CancelHeightmapSelectionMoveDrag();
    bool IsHeightmapBrushTool() const;
    bool IsHeightmapShapeTool() const;
    bool IsHeightmapPreviewTool() const;
    std::pair<int, int> SnapHeightmapLineEnd(int start_x, int start_y, int end_x, int end_y) const;
    std::vector<std::pair<int, int>> BuildHeightmapLineCells(int start_x, int start_y, int end_x, int end_y) const;
    std::vector<std::pair<int, int>> BuildHeightmapRectCells(int start_x, int start_y, int end_x, int end_y, bool filled) const;
    std::vector<std::pair<int, int>> BuildHeightmapCircleCells(int start_x, int start_y, int end_x, int end_y, bool filled) const;
    std::vector<std::pair<int, int>> BuildHeightmapFloodFillCells(int x, int y) const;
    std::map<std::pair<int, int>, uint16_t> BuildHeightmapStampCells(int x, int y) const;
    void ApplyHeightmapStampAt(int x, int y);
    void ApplyHeightmapFloodFillAt(int x, int y);
    void BeginHeightmapLineDrag(int x, int y, bool shift_down);
    void UpdateHeightmapLineDrag(int x, int y, bool shift_down);
    void CommitHeightmapLineDrag();
    void CancelHeightmapLineDrag();
    void ClampBackgroundSelection();
    void MoveBackgroundSelection(int dx, int dy);
    std::shared_ptr<Landstalker::Tilemap3D> CurrentRoomMap() const;
    int SelectedBackgroundBlockIndex() const;
    uint16_t SelectedBackgroundBlockId() const;
    int SelectedHeightmapCellX() const;
    int SelectedHeightmapCellY() const;
    int PrimaryHeightmapCellX() const;
    int PrimaryHeightmapCellY() const;
    uint16_t SelectedHeightmapCellValue() const;
    void SetSelectedHeightmapCell(uint16_t value, bool refresh_object_placements);
    void ApplyPrimaryHeightmapTypeToSelection();
    void ApplyPrimaryHeightmapPropsToSelection();
    void ApplyPrimaryHeightmapHeightToSelection();
    void UpdateHeightmapClipboardFromSelectedCell();
    void CopySelectedBackgroundBlock();
    void CopyBackgroundBlockAt(int x, int y);
    void CopySelectedHeightmapCell();
    void CopyHeightmapCellAt(int x, int y);
    void ClearBackgroundClipboard();
    void PasteSelectedBackgroundBlock();
    bool PasteBackgroundBlockAt(int x, int y, bool defer_updates = false);
    void CommitLayerDrawStroke();
    void PasteSelectedHeightmapCell();
    bool PasteHeightmapCellAt(int x, int y, bool defer_updates = false);
    void CommitHeightmapDrawStroke();
    void ReloadCurrentRoomMapView();
    void RenderBackgroundEditorOverlay(int width, int height);
    void RenderHeightmapEditorOverlay(int width, int height);
    float ZoomFactor() const;
    float ScreenToWorldX(int screen_x) const;
    float ScreenToWorldY(int screen_y) const;
    void PanCameraByStep(int dx, int dy, float speed = 20.0f);
    void ChangeZoomStep(int delta, float anchor_x, float anchor_y);
    void ResizeSelectedTileSwapByDelta(int dw, int dh);
    void UpdateAnimations(float dt);
    void UpdateStatusBar();
    void RecordRenderedFrame();
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
    wxStopWatch m_fps_stopwatch;
    wxStopWatch m_room_stopwatch;
    wxStopWatch m_anim_stopwatch;
    long m_last_anim_ms;
    long m_last_frame_ms;
    long m_frame_count;
    long m_animation_update_count;
    float m_fps;
    bool m_render_deferred;
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
    DrawingTool m_drawing_tool;
    HeightmapViewMode m_heightmap_view_mode;
    float m_non_heightmap_z_extent;
    float m_heightmap_z_scale;
    bool m_heightmap_tilemap_underlay;
    bool m_layer_heightmap_overlay;
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
    bool m_layer_dragging_select;
    bool m_layer_selection_add;
    bool m_layer_selection_subtract;
    bool m_layer_selection_parallelogram;
    int m_layer_selection_anchor_x;
    int m_layer_selection_anchor_y;
    int m_layer_selection_drag_anchor_x;
    int m_layer_selection_drag_anchor_y;
    bool m_layer_dragging_selection_move;
    int m_layer_selection_move_anchor_x;
    int m_layer_selection_move_anchor_y;
    int m_layer_selection_move_delta_x;
    int m_layer_selection_move_delta_y;
    bool m_layer_dragging_draw;
    bool m_layer_dragging_line;
    bool m_layer_draw_dirty;
    int m_layer_last_draw_x;
    int m_layer_last_draw_y;
    int m_layer_line_start_x;
    int m_layer_line_start_y;
    int m_layer_line_end_x;
    int m_layer_line_end_y;
    bool m_heightmap_clipboard_valid;
    uint16_t m_heightmap_clipboard_cell;
    bool m_heightmap_dragging_select;
    bool m_heightmap_dragging_draw;
    bool m_heightmap_dragging_line;
    bool m_heightmap_dragging_selection_move;
    bool m_heightmap_draw_dirty;
    bool m_heightmap_selection_add;
    bool m_heightmap_selection_subtract;
    int m_heightmap_selection_anchor_x;
    int m_heightmap_selection_anchor_y;
    int m_heightmap_selection_drag_anchor_x;
    int m_heightmap_selection_drag_anchor_y;
    int m_heightmap_last_draw_x;
    int m_heightmap_last_draw_y;
    int m_heightmap_line_start_x;
    int m_heightmap_line_start_y;
    int m_heightmap_line_end_x;
    int m_heightmap_line_end_y;
    int m_heightmap_selection_move_anchor_x;
    int m_heightmap_selection_move_anchor_y;
    int m_heightmap_selection_move_delta_x;
    int m_heightmap_selection_move_delta_y;
    std::set<std::pair<int, int>> m_heightmap_selected_cells;
    std::set<std::pair<int, int>> m_heightmap_selection_drag_base;
    std::set<std::pair<int, int>> m_layer_selected_cells;
    std::set<std::pair<int, int>> m_layer_selection_drag_base;
    std::map<std::pair<int, int>, uint16_t> m_layer_selection_move_values;
    std::vector<std::pair<int, int>> m_layer_line_preview_cells;
    std::vector<std::pair<int, int>> m_heightmap_line_preview_cells;
    std::map<std::pair<int, int>, uint16_t> m_heightmap_selection_move_values;
    std::vector<std::shared_ptr<Landstalker::Tilemap3D>> m_map_undo_stack;
    std::vector<std::shared_ptr<Landstalker::Tilemap3D>> m_map_redo_stack;
    std::vector<ObjectUndoState> m_object_undo_stack;
    std::vector<ObjectUndoState> m_object_redo_stack;
    bool m_restoring_history;
    PendingObjectAddType m_pending_add_type;
    PendingTileSwapPart m_pending_tileswap_part;
    Landstalker::TileSwap m_pending_add_swap;
    int m_pending_add_hover_x;
    int m_pending_add_hover_y;
    int m_pending_add_swap_index;
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
