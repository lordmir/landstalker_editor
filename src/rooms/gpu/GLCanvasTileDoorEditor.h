#ifndef GL_CANVAS_TILE_DOOR_EDITOR_H
#define GL_CANVAS_TILE_DOOR_EDITOR_H

#include "GLCanvas.h"

class GLCanvasTileDoorEditor {
public:
    explicit GLCanvasTileDoorEditor(MyGLCanvas& canvas);

    void ClearTileSwapPreview();
    void CycleSelectedDoorSize(int delta);
    void AddDoor();
    void AddTileSwap();
    void CycleSelectedTileSwapShape(int delta);
    void CycleSelectedTileSwapId(int delta);
    void ResizeSelectedTileSwapRegion(float requested_width, float requested_height);
    void ToggleSelectedTileSwapPreview();
    void ToggleSelectedDoorPreview();
    void RenderTileSwapOutlines();
    void RenderSelectedDoorTooltip();
    void RenderSelectedTileSwapRegionTooltip();

private:
    MyGLCanvas& m_canvas;
};

#endif  // GL_CANVAS_TILE_DOOR_EDITOR_H
