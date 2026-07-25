#ifndef GL_CANVAS_MOUSE_INPUT_H
#define GL_CANVAS_MOUSE_INPUT_H

class GLCanvas;
class wxMouseEvent;

// Owns top-level mouse gesture state and dispatches pointer input to the active mode.
class GLCanvasMouseInput {
public:
    explicit GLCanvasMouseInput(GLCanvas& canvas);

    void HandleMouseWheel(wxMouseEvent& evt);
    void HandleMouseMove(wxMouseEvent& evt);
    void HandleLeftDown(wxMouseEvent& evt);
    void HandleLeftDClick(wxMouseEvent& evt);
    void HandleLeftUp(wxMouseEvent& evt);
    void HandleMiddleDown(wxMouseEvent& evt);
    void HandleMiddleUp(wxMouseEvent& evt);
    void HandleRightDown(wxMouseEvent& evt);
    void HandleRightUp(wxMouseEvent& evt);
    void HandleMouseLeave(wxMouseEvent& evt);

private:
    GLCanvas& m_canvas;
};

#endif  // GL_CANVAS_MOUSE_INPUT_H
