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
#include "GLCanvasRoomInfoOverlay.h"
#include "GLCanvasInputTypes.h"

wxDECLARE_EVENT(EVT_GPU_EDITOR_MODE_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(EVT_GPU_LAYER_OPACITY_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(EVT_GPU_LAYER_BLOCK_SELECT, wxCommandEvent);
wxDECLARE_EVENT(EVT_GPU_HEIGHTMAP_TARGET_CHANGE, wxCommandEvent);

class GLCanvasKeyboardInput;
class GLCanvasMouseInput;

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

class GLCanvas : public wxGLCanvas {
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

    GLCanvas(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd);
    virtual ~GLCanvas();

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
    void SetDirectionInputMode(GLCanvasDirectionInputMode mode);
    GLCanvasDirectionInputMode GetDirectionInputMode() const;
    void CycleDirectionInputMode();
    bool HandleKeyDown(wxKeyEvent& evt);
    bool HandleKeyUp(wxKeyEvent& evt);
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
    void AddEntity();
    void DeleteSelectedObject();
    void ReorderSelectedObject(int delta);
    void AddWarpHalf();
    void AddDoor();
    void AddTileSwap();

private:

    // Heightmap cell value written to "cleared" cells: restriction 4, height 0.
    static constexpr uint16_t kClearedHeightmapCell = 0x4000;

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

    struct LayerUndoState {
        Landstalker::Tilemap3D::Layer layer;
        std::vector<uint16_t> blocks;
        // Set for structural edits (row/column insert/delete, clear): those
        // change map dimensions or both layers, so a blocks-only snapshot of
        // one layer cannot restore them.
        std::shared_ptr<Landstalker::Tilemap3D> full_map;
    };

    friend class GLCanvasEntityEditor;
    friend class GLCanvasWarpEditor;
    friend class GLCanvasDoorEditor;
    friend class GLCanvasTileSwapEditor;
    friend class GLCanvasObjectCoordinator;
    friend class GLCanvasHeightmapHitTest;
    friend class GLCanvasHeightmapMode;
    friend class GLCanvasLayerEditMode;
    friend class GLCanvasRoomInfoOverlay;
    friend class GLCanvasRoomMode;
    friend class GLCanvasKeyboardInput;
    friend class GLCanvasMouseInput;

    void OnIdle(wxIdleEvent& evt);
    void OnKeyDown(wxKeyEvent& evt);
    void OnKeyUp(wxKeyEvent& evt);
    void OnKillFocus(wxFocusEvent& evt);
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
    void ResetLayerEditState();
    void ResetHeightmapEditState();
    void CaptureUndoState(bool structural = false);
    void RestoreUndoState(const std::shared_ptr<Landstalker::Tilemap3D>& state);
    bool IsObjectHistoryMode() const;
    bool IsBackgroundLayerHistoryMode() const;
    bool IsForegroundLayerHistoryMode() const;
    LayerUndoState BuildLayerUndoState(Landstalker::Tilemap3D::Layer layer) const;
    LayerUndoState BuildFullMapLayerUndoState(Landstalker::Tilemap3D::Layer layer) const;
    void InvalidateCrossLayerHistory();
    void RestoreLayerUndoState(const LayerUndoState& state);
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
    void EnsureWorldRectVisible(float min_x, float min_y, float max_x, float max_y);
    void PersistCurrentRoomEdits();
    void RefreshEntityMetadata(SpriteInstance& inst);
    void CopySelectedEntity();
    void PasteEntity();
    void CutSelectedEntity();
    void SelectNextObject(int direction);
    void CycleSelectedEntityId(int delta);
    void CycleSelectedEntityPalette();
    void SetSelectedEntityOrientation(Landstalker::Orientation orientation);
    void SetSelectedEntityToFloor();
    bool HasPendingObjectAdd() const;
    void UpdatePendingObjectAddHover();
    void CommitPendingObjectAdd();
    void CancelPendingObjectAdd();
    bool BuildPendingEntityPreviewInstance(SpriteInstance& inst);
    bool BuildPendingWarpPreviewInstance(WarpInstance& inst);
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
    void ApplyHeightmapEditZExtent();
    bool BackgroundCellAt(const wxPoint& point, int& cell_x, int& cell_y) const;
    bool HeightmapCellAt(const wxPoint& point, int& cell_x, int& cell_y);
    bool BackgroundVirtualCellAt(const wxPoint& point, int& cell_x, int& cell_y) const;
    bool HeightmapVirtualCellAt(const wxPoint& point, int& cell_x, int& cell_y) const;
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
    std::vector<std::pair<int, int>> BuildLayerScreenLineCells(int start_x, int start_y, int end_x, int end_y) const;
    std::vector<std::pair<int, int>> BuildLayerScreenCircleCells(int start_x, int start_y, int end_x, int end_y, bool filled) const;
    std::vector<std::pair<int, int>> BuildLayerSkewRectCells(int start_x, int start_y, int end_x, int end_y, bool filled, bool lock_equal) const;
    std::vector<std::pair<int, int>> BuildLayerSkewCircleCells(int start_x, int start_y, int end_x, int end_y, bool filled, bool lock_equal) const;
    void BeginLayerLineDrag(int x, int y, bool shift_down, bool alt_down);
    void UpdateLayerLineDrag(int x, int y, bool shift_down, bool alt_down);
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
    // Collapses the layer/heightmap selection to a single cell, clamped to the
    // current map dimensions, keeping every selection representation in sync
    // (primary cell, anchors, and the selected-cell set for the active mode).
    void SetSelectedCell(int x, int y);
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
    std::vector<std::pair<int, int>> BuildLayerFloodFillCells(int x, int y) const;
    void ApplyLayerFloodFillAt(int x, int y);
    std::map<std::pair<int, int>, uint16_t> BuildLayerStampCells(int x, int y) const;
    void ApplyLayerStampAt(int x, int y);
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
    void UpdateStatusBar();
    void RecordRenderedFrame();

    std::shared_ptr<Landstalker::GameData> m_gd;
    wxGLContext* m_context = nullptr;
    std::unique_ptr<GLCanvasKeyboardInput> m_keyboard_input;
    std::unique_ptr<GLCanvasMouseInput> m_mouse_input;

    MapRenderer m_mapRenderer;
    HeightmapRenderer m_heightmapRenderer;
    SpriteRenderer m_spriteRenderer;
    GLCanvasRoomInfoOverlay m_room_info_overlay;

    std::vector<SpriteInstance> m_instances;
    std::vector<WarpInstance> m_warps;
    std::vector<Landstalker::Entity> m_room_entities;
    bool m_pending_warp_half = false;
    uint16_t m_pending_warp_room = 0xFFFF;
    uint32_t m_pending_warp_instance_id = 0;
    Landstalker::WarpList::Warp m_pending_warp;
    bool m_entity_clipboard_valid = false;
    Landstalker::Entity m_entity_clipboard;
    wxStopWatch m_fps_stopwatch;
    wxStopWatch m_anim_stopwatch;
    long m_last_anim_ms = 0;
    long m_last_frame_ms = 0;
    long m_frame_count = 0;
    long m_animation_update_count = 0;
    float m_fps = 0.0f;
    bool m_alpha = false;
    bool m_show_heightmap = false;
    bool m_show_entities = true;
    bool m_show_warps = true;
    bool m_show_tile_swaps = true;
    int m_hovered_entity_idx = -1;
    int m_selected_entity_idx = -1;
    int m_hovered_warp_idx = -1;
    int m_selected_warp_idx = -1;
    int m_hovered_tileswap_region_idx = -1;
    int m_selected_tileswap_region_idx = -1;
    int m_hovered_door_idx = -1;
    int m_selected_door_idx = -1;
    bool m_dragging_entity = false;
    bool m_drag_z_axis_only = false;
    uint32_t m_drag_instance_id = 0;
    wxPoint m_drag_start_mouse = wxDefaultPosition;
    float m_drag_start_x = 0.0f;
    float m_drag_start_y = 0.0f;
    float m_drag_start_z = 0.0f;
    float m_drag_plane_z = 0.0f;
    float m_drag_cursor_offset_x = 0.0f;
    float m_drag_cursor_offset_y = 0.0f;
    bool m_drag_floor_snap = false;
    bool m_dragging_warp = false;
    uint32_t m_drag_warp_instance_id = 0;
    int m_drag_warp_resize_axis = 0;
    float m_drag_warp_start_x = 0.0f;
    float m_drag_warp_start_y = 0.0f;
    float m_drag_warp_start_width = 0.0f;
    float m_drag_warp_start_height = 0.0f;
    float m_drag_warp_start_floor_z = 0.0f;
    bool m_dragging_door = false;
    int m_drag_door_idx = -1;
    int m_drag_door_start_x = 0;
    int m_drag_door_start_y = 0;
    bool m_dragging_tileswap_region = false;
    int m_drag_tileswap_region_idx = -1;
    int m_drag_tileswap_resize_axis = 0;
    int m_drag_tileswap_start_x = 0;
    int m_drag_tileswap_start_y = 0;
    int m_drag_tileswap_start_width = 1;
    int m_drag_tileswap_start_height = 1;
    bool m_dragging_pan = false;
    wxPoint m_drag_pan_start_mouse = wxDefaultPosition;
    float m_drag_pan_start_cam_x = 0.0f;
    float m_drag_pan_start_cam_y = 0.0f;
    // Source of truth for layer/sprite opacity. Both the keyboard step cycle
    // and the continuous slider write here, so the status bar, the layer
    // control (GetXxxOpacityByte), and the pending-ghost render all agree.
    float m_bg_opacity = 1.0f;
    float m_fg_opacity = 1.0f;
    float m_sprite_opacity = 1.0f;
    int m_entity_occlusion_idx = 1;
    bool m_show_hitboxes = true;
    EditorMode m_editor_mode = EditorMode::Room;
    DrawingTool m_drawing_tool = DrawingTool::Select;
    float m_non_heightmap_z_extent = 32.0f;
    float m_heightmap_z_scale = 0.0f;
    bool m_heightmap_tilemap_underlay = false;
    bool m_layer_heightmap_overlay = false;
    bool m_foreground_show_background_underlay = true;
    bool m_background_show_block_ids = false;
    bool m_layer_priority_highlight = true;
    bool m_background_has_selection = false;
    int m_background_selected_x = 0;
    int m_background_selected_y = 0;
    bool m_background_has_hover = false;
    int m_background_hover_x = 0;
    int m_background_hover_y = 0;
    bool m_background_clipboard_valid = false;
    uint16_t m_background_clipboard_block_id = 0;
    bool m_layer_dragging_select = false;
    bool m_layer_selection_add = false;
    bool m_layer_selection_subtract = false;
    bool m_layer_selection_parallelogram = false;
    int m_layer_selection_anchor_x = 0;
    int m_layer_selection_anchor_y = 0;
    int m_layer_selection_drag_anchor_x = 0;
    int m_layer_selection_drag_anchor_y = 0;
    bool m_layer_dragging_selection_move = false;
    int m_layer_selection_move_anchor_x = -1;
    int m_layer_selection_move_anchor_y = -1;
    int m_layer_selection_move_delta_x = 0;
    int m_layer_selection_move_delta_y = 0;
    bool m_layer_dragging_draw = false;
    bool m_layer_dragging_line = false;
    bool m_layer_draw_dirty = false;
    int m_layer_last_draw_x = -1;
    int m_layer_last_draw_y = -1;
    int m_layer_line_start_x = -1;
    int m_layer_line_start_y = -1;
    int m_layer_line_end_x = -1;
    int m_layer_line_end_y = -1;
    bool m_heightmap_clipboard_valid = false;
    uint16_t m_heightmap_clipboard_cell = 0;
    bool m_heightmap_dragging_select = false;
    bool m_heightmap_dragging_draw = false;
    bool m_heightmap_dragging_line = false;
    bool m_heightmap_dragging_selection_move = false;
    bool m_heightmap_draw_dirty = false;
    bool m_heightmap_selection_add = false;
    bool m_heightmap_selection_subtract = false;
    int m_heightmap_selection_anchor_x = 0;
    int m_heightmap_selection_anchor_y = 0;
    int m_heightmap_selection_drag_anchor_x = 0;
    int m_heightmap_selection_drag_anchor_y = 0;
    int m_heightmap_last_draw_x = -1;
    int m_heightmap_last_draw_y = -1;
    int m_heightmap_line_start_x = -1;
    int m_heightmap_line_start_y = -1;
    int m_heightmap_line_end_x = -1;
    int m_heightmap_line_end_y = -1;
    int m_heightmap_selection_move_anchor_x = -1;
    int m_heightmap_selection_move_anchor_y = -1;
    int m_heightmap_selection_move_delta_x = 0;
    int m_heightmap_selection_move_delta_y = 0;
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
    std::vector<LayerUndoState> m_bg_layer_undo_stack;
    std::vector<LayerUndoState> m_bg_layer_redo_stack;
    std::vector<LayerUndoState> m_fg_layer_undo_stack;
    std::vector<LayerUndoState> m_fg_layer_redo_stack;
    std::vector<ObjectUndoState> m_object_undo_stack;
    std::vector<ObjectUndoState> m_object_redo_stack;
    bool m_restoring_history = false;
    PendingObjectAddType m_pending_add_type = PendingObjectAddType::None;
    PendingTileSwapPart m_pending_tileswap_part = PendingTileSwapPart::MapSource;
    uint8_t m_pending_add_entity_id = Landstalker::Entity{}.GetType();
    uint8_t m_pending_add_entity_palette = Landstalker::Entity{}.GetPalette();
    Landstalker::Orientation m_pending_add_entity_orientation = Landstalker::Entity{}.GetOrientation();
    float m_pending_add_entity_cursor_offset_x = 0.0f;
    float m_pending_add_entity_cursor_offset_y = 0.0f;
    float m_pending_add_plane_z = 0.0f;
    bool m_pending_add_floor_snap = true;
    float m_pending_add_start_x = 0.0f;
    float m_pending_add_start_y = 0.0f;
    float m_pending_add_start_z = 0.0f;
    wxPoint m_pending_add_mouse_start = wxDefaultPosition;
    Landstalker::TileSwap m_pending_add_swap;
    int m_pending_add_hover_x = -1;
    int m_pending_add_hover_y = -1;
    float m_pending_add_warp_width = 1.0f;
    float m_pending_add_warp_height = 1.0f;
    Landstalker::WarpList::Warp::Type m_pending_add_warp_type = Landstalker::WarpList::Warp::Type::NORMAL;
    Landstalker::Door::Size m_pending_add_door_size = Landstalker::Door::Size::DOOR_1X4;
    bool m_tileswap_preview_active = false;
    int m_tileswap_preview_swap_index = -1;
    bool m_door_preview_active = false;
    int m_door_preview_idx = -1;
    std::shared_ptr<Landstalker::Tilemap3D> m_tileswap_preview_map;
    uint16_t m_current_room = 0;
    float m_cam_x = 0.0f;
    float m_cam_y = 0.0f;
    int m_zoom_step_idx = 1;
    bool m_gl_init_failed = false;
    bool m_initialized = false;
    wxPoint m_last_mouse_pos = wxDefaultPosition;
    wxDECLARE_EVENT_TABLE();
};

#endif // GL_CANVAS_H
