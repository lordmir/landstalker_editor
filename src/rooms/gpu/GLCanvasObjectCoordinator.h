#ifndef GL_CANVAS_OBJECT_COORDINATOR_H
#define GL_CANVAS_OBJECT_COORDINATOR_H

#include "GLCanvas.h"

// Coordinates operations that can target any selectable room object type.
class GLCanvasObjectCoordinator {
public:
    // Creates a coordinator over canvas-owned object state.
    explicit GLCanvasObjectCoordinator(MyGLCanvas& canvas);

    // Deletes whichever object type is currently selected.
    void DeleteSelectedObject();
    // Moves the selected object earlier/later in its list.
    void ReorderSelectedObject(int delta);
    // Selects the next object across entities, warps, tile swaps, and doors.
    void SelectNextObject(int direction);
    // Selects the next region belonging to tile swaps.
    void SelectNextTileSwapRegion(int direction);
    // Nudges the selected object by room-grid/Z deltas.
    void NudgeSelectedObject(float dx, float dy, float dz);

private:
    // Canvas whose shared editor state is being manipulated.
    MyGLCanvas& m_canvas;
};

#endif  // GL_CANVAS_OBJECT_COORDINATOR_H
