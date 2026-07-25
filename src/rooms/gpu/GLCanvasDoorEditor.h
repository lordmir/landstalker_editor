#ifndef GL_CANVAS_DOOR_EDITOR_H
#define GL_CANVAS_DOOR_EDITOR_H

#include "GLCanvas.h"

// Door editing facade over canvas-owned door state.
class GLCanvasDoorEditor {
public:
    // Creates a door editor adapter over canvas-owned state.
    explicit GLCanvasDoorEditor(GLCanvas& canvas);

    // Returns the door hit by the point, or -1.
    int HitTestDoor(const wxPoint& point) const;
    // Starts moving a door.
    void StartDoorDrag(int door_idx, const wxMouseEvent& evt);
    // Applies the current mouse position to an active door drag.
    void UpdateDoorDrag(const wxMouseEvent& evt);
    // Finishes an active door drag and publishes room-data changes.
    void EndDoorDrag();
    // Enters pending-add mode for a new door.
    void BeginAddDoor();
    // Adds a door at the preferred free heightmap cell.
    void AddDoor();
    // Cycles selected or pending door size.
    void CycleSelectedDoorSize(int delta);
    // Toggles previewing the selected door drawn into the room map.
    void ToggleSelectedDoorPreview();
    // Renders the pending door placement ghost.
    void RenderPendingDoorGhost();
    // Renders door cell and affected-map outlines.
    void RenderDoors();
    // Renders the selected door tooltip.
    void RenderSelectedDoorTooltip();

private:
    // Canvas whose shared editor state is being manipulated.
    GLCanvas& m_canvas;
};

#endif  // GL_CANVAS_DOOR_EDITOR_H
