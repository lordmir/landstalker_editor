#ifndef GL_CANVAS_ROOM_MODE_H
#define GL_CANVAS_ROOM_MODE_H

#include "GLCanvas.h"

// Handles the default room view, object selection, and room-level mouse shortcuts.
class GLCanvasRoomMode {
public:
    // Binds the mode facade to the shared canvas state it operates on.
    explicit GLCanvasRoomMode(MyGLCanvas& canvas);

    // Dispatches keyboard commands that are only active in normal room mode.
    bool HandleKeyDown(wxKeyEvent& evt);
    // Updates hover state, cursor previews, and drag affordances.
    void HandleMouseMove(const wxMouseEvent& evt);
    // Starts selections, drags, and object creation from a left click.
    void HandleLeftDown(const wxMouseEvent& evt);
    // Handles context actions and right-click navigation for the room view.
    void HandleRightDown(const wxMouseEvent& evt);
    // Draws room-mode overlays after the base room has rendered.
    void Render(int width, int height);

private:
    // Shared canvas owning room data, renderer state, and selections.
    MyGLCanvas& m_canvas;
};

#endif  // GL_CANVAS_ROOM_MODE_H
