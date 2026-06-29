#ifndef GL_CANVAS_OBJECT_COORDINATOR_H
#define GL_CANVAS_OBJECT_COORDINATOR_H

#include "GLCanvas.h"

class GLCanvasObjectCoordinator {
public:
    explicit GLCanvasObjectCoordinator(MyGLCanvas& canvas);

    void DeleteSelectedObject();
    void ReorderSelectedObject(int delta);
    void SelectNextObject(int direction);
    void SelectNextTileSwapRegion(int direction);
    void NudgeSelectedObject(float dx, float dy, float dz);

private:
    MyGLCanvas& m_canvas;
};

#endif  // GL_CANVAS_OBJECT_COORDINATOR_H
