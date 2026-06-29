#ifndef GL_CANVAS_LAYER_EDIT_MODE_H
#define GL_CANVAS_LAYER_EDIT_MODE_H

#include "GLCanvas.h"

class GLCanvasLayerEditMode {
public:
    explicit GLCanvasLayerEditMode(MyGLCanvas& canvas);

    bool HandleKeyDown(wxKeyEvent& evt);
    void HandleMouseMove(const wxMouseEvent& evt);
    void HandleLeftDown(const wxMouseEvent& evt);
    void HandleRightDown(const wxMouseEvent& evt);
    void Render(int width, int height);

private:
    MyGLCanvas& m_canvas;
};

#endif  // GL_CANVAS_LAYER_EDIT_MODE_H
