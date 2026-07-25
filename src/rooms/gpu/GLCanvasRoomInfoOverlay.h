#ifndef GL_CANVAS_ROOM_INFO_OVERLAY_H
#define GL_CANVAS_ROOM_INFO_OVERLAY_H

#include <cstdint>
#include <wx/gdicmn.h>
#include <vector>

class GLCanvas;

// Draws clickable room metadata and navigation links over the GL canvas.
class GLCanvasRoomInfoOverlay {
public:
    // Binds the overlay to the canvas whose room data it presents.
    explicit GLCanvasRoomInfoOverlay(GLCanvas& canvas);

    // Renders the current room information panel and records link hit boxes.
    void Render(int width, int height);
    // Returns the linked room under the mouse point, or a negative value when none is hit.
    int HitTest(const wxPoint& point) const;

private:
    // Screen rectangle mapped to a destination room number.
    struct Link {
        wxRect rect;   // Clickable bounds in canvas coordinates.
        uint16_t room; // Room to open when the link is clicked.
    };

    // Canvas supplying current room, room data, and navigation helpers.
    GLCanvas& m_canvas;
    // Link hit boxes populated during the latest render pass.
    std::vector<Link> m_links;
};

#endif  // GL_CANVAS_ROOM_INFO_OVERLAY_H
