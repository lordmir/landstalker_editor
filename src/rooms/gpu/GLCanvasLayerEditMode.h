#ifndef GL_CANVAS_LAYER_EDIT_MODE_H
#define GL_CANVAS_LAYER_EDIT_MODE_H

#include "GLCanvas.h"

// Handles foreground/background tile painting and tile selection workflows.
class GLCanvasLayerEditMode {
public:
    // Binds the mode facade to the shared canvas state it edits.
    explicit GLCanvasLayerEditMode(MyGLCanvas& canvas);

    // Applies layer-edit keyboard shortcuts such as tool and tile changes.
    bool HandleKeyDown(wxKeyEvent& evt);
    // Tracks hovered map cells and continuous paint/eyedropper actions.
    void HandleMouseMove(const wxMouseEvent& evt);
    // Starts a paint, erase, fill, or pick action on the active layer.
    void HandleLeftDown(const wxMouseEvent& evt);
    // Handles alternate layer-edit actions such as sampling or clearing.
    void HandleRightDown(const wxMouseEvent& evt);
    // Draws tile cursor, brush, and layer-edit overlays.
    void Render(int width, int height);

private:
    // Shared canvas owning room data, tilemap preview state, and renderers.
    MyGLCanvas& m_canvas;
};

#endif  // GL_CANVAS_LAYER_EDIT_MODE_H
