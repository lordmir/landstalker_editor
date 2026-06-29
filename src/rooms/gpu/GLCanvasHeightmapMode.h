#ifndef GL_CANVAS_HEIGHTMAP_MODE_H
#define GL_CANVAS_HEIGHTMAP_MODE_H

#include "GLCanvas.h"

class GLCanvasHeightmapMode {
public:
    explicit GLCanvasHeightmapMode(MyGLCanvas& canvas);

    bool HandleKeyDown(wxKeyEvent& evt);
    void HandleMouseMove(const wxMouseEvent& evt);
    void HandleLeftDown(const wxMouseEvent& evt);
    void HandleRightDown(const wxMouseEvent& evt);
    void Render(int width, int height);

private:
    MyGLCanvas& m_canvas;
};

#endif  // GL_CANVAS_HEIGHTMAP_MODE_H
