#ifndef GL_CANVAS_TILE_DOOR_EDITOR_H
#define GL_CANVAS_TILE_DOOR_EDITOR_H

#include "GLCanvas.h"

// Door and tile-swap editing facade; implementation is split by domain.
class GLCanvasTileDoorEditor {
public:
    // Creates a door/tile-swap editor adapter over canvas-owned state.
    explicit GLCanvasTileDoorEditor(MyGLCanvas& canvas);

    // Clears any active tile-swap or door preview map.
    void ClearTileSwapPreview();
    // Returns the tile-swap region hit by the point, or -1.
    int HitTestTileSwapRegion(const wxPoint& point) const;
    // Returns the selected tile-swap resize handle hit by the point, or 0.
    int HitTestTileSwapRegionResizeControl(const wxPoint& point) const;
    // Returns the door hit by the point, or -1.
    int HitTestDoor(const wxPoint& point) const;
    // Starts moving a door.
    void StartDoorDrag(int door_idx, const wxMouseEvent& evt);
    // Applies the current mouse position to an active door drag.
    void UpdateDoorDrag(const wxMouseEvent& evt);
    // Finishes an active door drag and publishes room-data changes.
    void EndDoorDrag();
    // Starts moving or resizing a tile-swap region.
    void StartTileSwapRegionDrag(int region_idx, int resize_axis, const wxMouseEvent& evt);
    // Applies the current mouse position to an active tile-swap drag.
    void UpdateTileSwapRegionDrag(const wxMouseEvent& evt);
    // Finishes an active tile-swap drag and publishes room-data changes.
    void EndTileSwapRegionDrag();
    // Enters pending-add mode for a new door.
    void BeginAddDoor();
    // Enters pending-add mode for a new tile swap.
    void BeginAddTileSwap();
    // Commits the next step of the multi-click pending tile-swap flow.
    void CommitPendingTileSwapStep();
    // Cycles selected or pending door size.
    void CycleSelectedDoorSize(int delta);
    // Adds a door at the preferred free heightmap cell.
    void AddDoor();
    // Adds a tile swap at the preferred free cells.
    void AddTileSwap();
    // Cycles the selected tile swap's shape/mode.
    void CycleSelectedTileSwapShape(int delta);
    // Cycles or swaps the selected tile swap trigger ID.
    void CycleSelectedTileSwapId(int delta);
    // Resizes the selected tile-swap region by integer deltas.
    void ResizeSelectedTileSwapByDelta(int dw, int dh);
    // Resizes the selected tile-swap region to requested dimensions.
    void ResizeSelectedTileSwapRegion(float requested_width, float requested_height);
    // Toggles previewing the selected tile swap applied to the room map.
    void ToggleSelectedTileSwapPreview();
    // Toggles previewing the selected door drawn into the room map.
    void ToggleSelectedDoorPreview();
    // Renders tile-swap outlines, handles, and preview styling.
    void RenderTileSwapOutlines();
    // Renders door cell and affected-map outlines.
    void RenderDoors();
    // Renders the selected door tooltip.
    void RenderSelectedDoorTooltip();
    // Renders the selected tile-swap region tooltip.
    void RenderSelectedTileSwapRegionTooltip();
    // Renders the pending door placement ghost.
    void RenderPendingDoorGhost();

private:
    // Canvas whose shared editor state is being manipulated.
    MyGLCanvas& m_canvas;
};

#endif  // GL_CANVAS_TILE_DOOR_EDITOR_H
