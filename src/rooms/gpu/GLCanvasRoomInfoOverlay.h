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
    // Returns the linked destination room under the mouse point (navigation links only), or a
    // negative value when none is hit.
    int HitTest(const wxPoint& point) const;
    // True when the point is over the "Room Actions" row (which opens the room-actions editor for
    // the current room rather than navigating).
    bool HitTestActionLink(const wxPoint& point) const;
    // True when the point is over the "Shop" row (which opens the shop editor for the current room).
    bool HitTestShopLink(const wxPoint& point) const;

private:
    // The kinds of clickable row in the overlay.
    enum class LinkKind { Navigate, RoomActions, Shop };
    // Screen rectangle mapped to a destination room number (or the current room, for editor rows).
    struct Link {
        wxRect rect;         // Clickable bounds in canvas coordinates.
        uint16_t room;       // Room to open when a navigation link is clicked.
        LinkKind kind = LinkKind::Navigate;
    };

    // Canvas supplying current room, room data, and navigation helpers.
    GLCanvas& m_canvas;
    // Link hit boxes populated during the latest render pass.
    std::vector<Link> m_links;
};

#endif  // GL_CANVAS_ROOM_INFO_OVERLAY_H
