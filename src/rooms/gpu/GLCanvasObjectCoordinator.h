#ifndef GL_CANVAS_OBJECT_COORDINATOR_H
#define GL_CANVAS_OBJECT_COORDINATOR_H

#include "GLCanvas.h"

// Coordinates operations that can target any selectable room object type.
class GLCanvasObjectCoordinator {
public:
    // Creates a coordinator over canvas-owned object state.
    explicit GLCanvasObjectCoordinator(GLCanvas& canvas);

    // Clears all room-object hover and selection state.
    void ClearSelection();
    // Clears transient object state before the canvas switches rooms.
    void PrepareForRoomLoad();
    // Rebuilds room-object editor instances after the room renderers load.
    void LoadRoomObjects(uint16_t roomnum);
    // Reprojects room objects after the heightmap floor changes.
    void RefreshPlacementsFromHeightmap();
    // Persists the current room's object edits to game data.
    void PersistCurrentRoomEdits();
    // Selects a room object from its corresponding control-list identifier.
    void SelectEntityByIndex(int selection);
    void SelectWarpByIndex(int selection);
    void SelectTileSwapByIndex(int selection);
    void SelectDoorByIndex(int selection);
    // Returns the selected object's identifier in its corresponding control list.
    static int SelectedEntityListIndex(const GLCanvas& canvas);
    static int SelectedWarpListIndex(const GLCanvas& canvas);
    static int SelectedTileSwapListIndex(const GLCanvas& canvas);
    static int SelectedDoorListIndex(const GLCanvas& canvas);
    // Selects the topmost room object under a canvas point.
    bool SelectAt(const wxPoint& point);
    // Opens the properties editor for the selected room object.
    bool OpenSelectedProperties();
    // Notifies room controls that the object selection or data has changed.
    void NotifySelectionChanged();
    void NotifyRoomDataChanged(bool entities, bool warps, bool swaps, bool doors);
    // Pans the camera when necessary to reveal the selected room object.
    void FocusCameraOnSelectedObjectIfNeeded();

    // Deletes whichever object type is currently selected.
    void DeleteSelectedObject();
    // Moves the selected object earlier/later in its list.
    void ReorderSelectedObject(int delta);
    // Selects the next object across entities, warps, tile swaps, and doors.
    void SelectNextObject(int direction);
    // Nudges the selected object by room-grid/Z deltas.
    void NudgeSelectedObject(float dx, float dy, float dz);

private:
    // Canvas whose shared editor state is being manipulated.
    GLCanvas& m_canvas;
};

#endif  // GL_CANVAS_OBJECT_COORDINATOR_H
