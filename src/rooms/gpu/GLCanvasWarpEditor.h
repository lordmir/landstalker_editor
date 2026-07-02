#ifndef GL_CANVAS_WARP_EDITOR_H
#define GL_CANVAS_WARP_EDITOR_H

#include <utility>
#include "GLCanvas.h"

// Warp-specific editing, hit-testing, dragging, and overlay rendering.
class GLCanvasWarpEditor {
public:
    // Creates a warp editor adapter over canvas-owned state.
    explicit GLCanvasWarpEditor(MyGLCanvas& canvas);

    // Enters pending-add mode for adding the other half of a warp pair.
    void BeginAddWarpHalf();
    // Commits the pending warp half into the room data.
    void AddWarpHalf();
    // Returns the topmost warp hit by the point.
    int HitTestWarp(const wxPoint& point) const;
    // Returns which resize handle was hit for the selected warp, or 0.
    int HitTestWarpResizeControl(const wxPoint& point) const;
    // Starts moving the selected warp.
    void StartWarpDrag(int warp_idx, const wxMouseEvent& evt);
    // Starts resizing the selected warp along the given axis.
    void StartWarpResizeDrag(int warp_idx, int axis, const wxMouseEvent& evt);
    // Applies the current mouse position to an active warp drag.
    void UpdateWarpDrag(const wxMouseEvent& evt);
    // Finishes an active warp drag and publishes room-data changes.
    void EndWarpDrag();
    // Finds an unused warp-sized grid cell near the requested location.
    std::pair<float, float> FindNearestFreeWarpCell(float preferred_x, float preferred_y) const;
    // Resizes the selected or pending warp by room-grid deltas.
    void ResizeSelectedWarp(float dx, float dy);
    // Rotates/changes the selected warp orientation using directional deltas.
    void RotateSelectedWarp(float dx, float dy);
    // Cycles selected or pending warp type.
    void CycleSelectedWarpType(int delta);
    // Renders warp outlines and handles.
    void RenderWarps();
    // Renders the selected warp tooltip.
    void RenderSelectedWarpTooltip();
    // Renders the pending warp placement ghost.
    void RenderPendingWarpGhost();

private:
    // Canvas whose shared editor state is being manipulated.
    MyGLCanvas& m_canvas;
};

#endif  // GL_CANVAS_WARP_EDITOR_H
