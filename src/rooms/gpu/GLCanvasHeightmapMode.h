#ifndef GL_CANVAS_HEIGHTMAP_MODE_H
#define GL_CANVAS_HEIGHTMAP_MODE_H

#include "GLCanvas.h"
#include <cstdint>

// Handles heightmap cell editing, restriction editing, and heightmap overlays.
class GLCanvasHeightmapMode {
public:
    // Binds the mode facade to the shared canvas state it edits.
    explicit GLCanvasHeightmapMode(MyGLCanvas& canvas);

    // Applies heightmap keyboard commands such as value and restriction changes.
    bool HandleKeyDown(wxKeyEvent& evt);
    // Tracks hovered heightmap cells and continuous paint operations.
    void HandleMouseMove(const wxMouseEvent& evt);
    // Starts editing the selected or hovered heightmap cell.
    void HandleLeftDown(const wxMouseEvent& evt);
    // Completes active heightmap selection, drawing, and move gestures.
    void HandleLeftUp(const wxMouseEvent& evt);
    // Handles alternate heightmap actions such as clearing or sampling.
    void HandleRightDown(const wxMouseEvent& evt);
    // Clears the heightmap hover when the pointer leaves the canvas.
    void HandleMouseLeave(const wxMouseEvent& evt);
    // Draws heightmap cursors, selected-cell markers, and edit previews.
    void Render(int width, int height);

private:
    // Returns the currently selected movement restriction nibble.
    uint8_t SelectedRestriction() const;
    // Writes a heightmap value and optionally refreshes object placement previews.
    void SetSelectedHeightmapCell(uint16_t value, bool refresh_object_placements);
    // Adjusts the selected cell's height/type component.
    void AdjustSelectedHeightmapType(int delta);
    // Adjusts the selected cell's restriction component.
    void AdjustSelectedHeightmapRestriction(int delta);
    // Cycles through valid restriction values in the requested direction.
    void CycleSelectedHeightmapRestriction(int direction);
    // Clears the selected cell back to the empty/no-height value.
    void ClearSelectedHeightmapCell();

    // Shared canvas owning room data, heightmap selection, and renderers.
    MyGLCanvas& m_canvas;
};

#endif  // GL_CANVAS_HEIGHTMAP_MODE_H
