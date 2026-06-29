#ifndef GL_CANVAS_HEIGHTMAP_MODE_H
#define GL_CANVAS_HEIGHTMAP_MODE_H

#include "GLCanvas.h"
#include <cstdint>

class GLCanvasHeightmapMode {
public:
    explicit GLCanvasHeightmapMode(MyGLCanvas& canvas);

    bool HandleKeyDown(wxKeyEvent& evt);
    void HandleMouseMove(const wxMouseEvent& evt);
    void HandleLeftDown(const wxMouseEvent& evt);
    void HandleRightDown(const wxMouseEvent& evt);
    void Render(int width, int height);

private:
    uint8_t SelectedRestriction() const;
    void SetSelectedHeightmapCell(uint16_t value, bool refresh_object_placements);
    void AdjustSelectedHeightmapType(int delta);
    void AdjustSelectedHeightmapRestriction(int delta);
    void CycleSelectedHeightmapRestriction(int direction);
    void ClearSelectedHeightmapCell();

    MyGLCanvas& m_canvas;
};

#endif  // GL_CANVAS_HEIGHTMAP_MODE_H
