#ifndef GL_CANVAS_ENTITY_EDITOR_H
#define GL_CANVAS_ENTITY_EDITOR_H

#include "GLCanvas.h"

class GLCanvasEntityEditor {
public:
    explicit GLCanvasEntityEditor(MyGLCanvas& canvas);

    void AddEntity();
    void CopySelectedEntity();
    void PasteEntity();
    void CycleSelectedEntityId(int delta);
    void CycleSelectedEntityPalette();
    void SetSelectedEntityOrientation(Landstalker::Orientation orientation);
    void SetSelectedEntityToFloor();
    void RenderSelectedEntityTooltip();

private:
    MyGLCanvas& m_canvas;
};

#endif  // GL_CANVAS_ENTITY_EDITOR_H
