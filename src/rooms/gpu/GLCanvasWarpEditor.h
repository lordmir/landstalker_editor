#ifndef GL_CANVAS_WARP_EDITOR_H
#define GL_CANVAS_WARP_EDITOR_H

#include <utility>
#include "GLCanvas.h"

class GLCanvasWarpEditor {
public:
    explicit GLCanvasWarpEditor(MyGLCanvas& canvas);

    void AddWarpHalf();
    std::pair<float, float> FindNearestFreeWarpCell(float preferred_x, float preferred_y) const;
    void ResizeSelectedWarp(float dx, float dy);
    void RotateSelectedWarp(float dx, float dy);
    void CycleSelectedWarpType(int delta);
    void RenderWarps();
    void RenderSelectedWarpTooltip();

private:
    MyGLCanvas& m_canvas;
};

#endif  // GL_CANVAS_WARP_EDITOR_H
