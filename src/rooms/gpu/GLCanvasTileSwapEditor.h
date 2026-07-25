#ifndef GL_CANVAS_TILE_SWAP_EDITOR_H
#define GL_CANVAS_TILE_SWAP_EDITOR_H

#include "GLCanvas.h"

// Tile-swap editing facade over canvas-owned tile-swap state.
class GLCanvasTileSwapEditor {
public:
    // Creates a tile-swap editor adapter over canvas-owned state.
    explicit GLCanvasTileSwapEditor(GLCanvas& canvas);

    // Returns the tile-swap region hit by the point, or -1.
    int HitTestTileSwapRegion(const wxPoint& point) const;
    // Returns the selected tile-swap resize handle hit by the point, or 0.
    int HitTestTileSwapRegionResizeControl(const wxPoint& point) const;
    // Starts moving or resizing a tile-swap region.
    void StartTileSwapRegionDrag(int region_idx, int resize_axis, const wxMouseEvent& evt);
    // Applies the current mouse position to an active tile-swap drag.
    void UpdateTileSwapRegionDrag(const wxMouseEvent& evt);
    // Finishes an active tile-swap drag and publishes room-data changes.
    void EndTileSwapRegionDrag();
    // Enters pending-add mode for a new tile swap.
    void BeginAddTileSwap();
    // Commits the next step of the multi-click pending tile-swap flow.
    void CommitPendingTileSwapStep();
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
    // Renders tile-swap outlines, handles, and preview styling.
    void RenderTileSwapOutlines();
    // Renders the selected tile-swap region tooltip.
    void RenderSelectedTileSwapRegionTooltip();

private:
    // Canvas whose shared editor state is being manipulated.
    GLCanvas& m_canvas;
};

#endif  // GL_CANVAS_TILE_SWAP_EDITOR_H
