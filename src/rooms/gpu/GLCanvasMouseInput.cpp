#include "GLCanvasMouseInput.h"

#include "GLCanvas.h"
#include "GLCanvasHeightmapMode.h"
#include "GLCanvasLayerEditMode.h"
#include "GLCanvasRoomMode.h"

#include <cmath>

namespace {

float WheelSteps(const wxMouseEvent& evt)
{
    int delta = evt.GetWheelDelta();
    if (delta == 0) {
        return evt.GetWheelRotation() > 0 ? 1.0f : -1.0f;
    }
    return static_cast<float>(evt.GetWheelRotation()) / static_cast<float>(delta);
}

}  // namespace

GLCanvasMouseInput::GLCanvasMouseInput(GLCanvas& canvas)
    : m_canvas(canvas)
{
}

void GLCanvasMouseInput::HandleMouseWheel(wxMouseEvent& evt)
{
    if (evt.ControlDown()) {
        float steps = WheelSteps(evt);
        int delta = steps > 0.0f ? 1 : (steps < 0.0f ? -1 : 0);
        if (delta != 0) {
            m_canvas.ChangeZoomStep(
                delta,
                static_cast<float>(evt.GetPosition().x),
                static_cast<float>(evt.GetPosition().y));
        }
        m_canvas.Refresh();
        return;
    }

    constexpr float wheel_pan_speed = 80.0f;
    float movement = WheelSteps(evt) * wheel_pan_speed;
    if (evt.GetWheelAxis() == wxMOUSE_WHEEL_HORIZONTAL) {
        m_canvas.m_cam_x += movement;
    } else {
        m_canvas.m_cam_y += movement;
    }
    m_canvas.m_cam_x = std::round(m_canvas.m_cam_x);
    m_canvas.m_cam_y = std::round(m_canvas.m_cam_y);
    m_canvas.Refresh();
}

void GLCanvasMouseInput::HandleMouseMove(wxMouseEvent& evt)
{
    m_canvas.m_last_mouse_pos = evt.GetPosition();
    if (m_canvas.m_dragging_pan) {
        m_canvas.m_cam_x = m_canvas.m_drag_pan_start_cam_x +
            static_cast<float>(evt.GetPosition().x - m_canvas.m_drag_pan_start_mouse.x);
        m_canvas.m_cam_y = m_canvas.m_drag_pan_start_cam_y +
            static_cast<float>(evt.GetPosition().y - m_canvas.m_drag_pan_start_mouse.y);
        m_canvas.m_cam_x = std::round(m_canvas.m_cam_x);
        m_canvas.m_cam_y = std::round(m_canvas.m_cam_y);
        m_canvas.UpdateStatusBar();
        m_canvas.Refresh();
        return;
    }
    if (m_canvas.IsHeightmapEditMode()) {
        GLCanvasHeightmapMode(m_canvas).HandleMouseMove(evt);
    } else if (m_canvas.IsLayerEditMode()) {
        GLCanvasLayerEditMode(m_canvas).HandleMouseMove(evt);
    } else {
        GLCanvasRoomMode(m_canvas).HandleMouseMove(evt);
    }
    m_canvas.UpdateStatusBar();
    evt.Skip();
}

void GLCanvasMouseInput::HandleLeftDown(wxMouseEvent& evt)
{
    m_canvas.SetFocus();
    m_canvas.m_last_mouse_pos = evt.GetPosition();
    if (m_canvas.IsHeightmapEditMode()) {
        GLCanvasHeightmapMode(m_canvas).HandleLeftDown(evt);
    } else if (m_canvas.IsLayerEditMode()) {
        GLCanvasLayerEditMode(m_canvas).HandleLeftDown(evt);
    } else {
        GLCanvasRoomMode(m_canvas).HandleLeftDown(evt);
    }
    m_canvas.UpdateStatusBar();
}

void GLCanvasMouseInput::HandleLeftDClick(wxMouseEvent& evt)
{
    m_canvas.SetFocus();
    m_canvas.m_last_mouse_pos = evt.GetPosition();
    if (m_canvas.IsAnyEditMode()) {
        evt.Skip();
        return;
    }
    evt.Skip(!GLCanvasRoomMode(m_canvas).HandleLeftDClick(evt));
}

void GLCanvasMouseInput::HandleLeftUp(wxMouseEvent& evt)
{
    if (m_canvas.IsHeightmapEditMode()) {
        GLCanvasHeightmapMode(m_canvas).HandleLeftUp(evt);
    } else if (m_canvas.IsLayerEditMode()) {
        GLCanvasLayerEditMode(m_canvas).HandleLeftUp(evt);
    } else {
        GLCanvasRoomMode(m_canvas).HandleLeftUp(evt);
    }
    evt.Skip();
}

void GLCanvasMouseInput::HandleMiddleDown(wxMouseEvent& evt)
{
    m_canvas.SetFocus();
    m_canvas.m_last_mouse_pos = evt.GetPosition();
    m_canvas.m_dragging_pan = true;
    m_canvas.m_drag_pan_start_mouse = evt.GetPosition();
    m_canvas.m_drag_pan_start_cam_x = m_canvas.m_cam_x;
    m_canvas.m_drag_pan_start_cam_y = m_canvas.m_cam_y;
    m_canvas.SetCursor(wxCursor(wxCURSOR_SIZING));
    if (!m_canvas.HasCapture()) {
        m_canvas.CaptureMouse();
    }
}

void GLCanvasMouseInput::HandleMiddleUp(wxMouseEvent& evt)
{
    if (m_canvas.m_dragging_pan) {
        m_canvas.m_dragging_pan = false;
        if (m_canvas.HasCapture()) {
            m_canvas.ReleaseMouse();
        }
        m_canvas.SetCursor(wxCursor(wxCURSOR_ARROW));
        m_canvas.Refresh();
    }
    evt.Skip();
}

void GLCanvasMouseInput::HandleRightDown(wxMouseEvent& evt)
{
    m_canvas.SetFocus();
    m_canvas.m_last_mouse_pos = evt.GetPosition();
    if (m_canvas.IsHeightmapEditMode()) {
        GLCanvasHeightmapMode(m_canvas).HandleRightDown(evt);
    } else if (m_canvas.IsLayerEditMode()) {
        GLCanvasLayerEditMode(m_canvas).HandleRightDown(evt);
    } else {
        GLCanvasRoomMode(m_canvas).HandleRightDown(evt);
    }
    m_canvas.UpdateStatusBar();
}

void GLCanvasMouseInput::HandleRightUp(wxMouseEvent& evt)
{
    if (!m_canvas.IsAnyEditMode()) {
        GLCanvasRoomMode(m_canvas).HandleRightUp(evt);
    }
    evt.Skip();
}

void GLCanvasMouseInput::HandleMouseLeave(wxMouseEvent& evt)
{
    if (m_canvas.IsHeightmapEditMode()) {
        GLCanvasHeightmapMode(m_canvas).HandleMouseLeave(evt);
    } else if (m_canvas.IsLayerEditMode()) {
        GLCanvasLayerEditMode(m_canvas).HandleMouseLeave(evt);
    } else {
        GLCanvasRoomMode(m_canvas).HandleMouseLeave(evt);
    }
    evt.Skip();
}

void GLCanvas::OnMouseWheel(wxMouseEvent& evt)
{
    m_mouse_input->HandleMouseWheel(evt);
}

void GLCanvas::OnMouseMove(wxMouseEvent& evt)
{
    m_mouse_input->HandleMouseMove(evt);
}

void GLCanvas::OnLeftDown(wxMouseEvent& evt)
{
    m_mouse_input->HandleLeftDown(evt);
}

void GLCanvas::OnLeftDClick(wxMouseEvent& evt)
{
    m_mouse_input->HandleLeftDClick(evt);
}

void GLCanvas::OnLeftUp(wxMouseEvent& evt)
{
    m_mouse_input->HandleLeftUp(evt);
}

void GLCanvas::OnMiddleDown(wxMouseEvent& evt)
{
    m_mouse_input->HandleMiddleDown(evt);
}

void GLCanvas::OnMiddleUp(wxMouseEvent& evt)
{
    m_mouse_input->HandleMiddleUp(evt);
}

void GLCanvas::OnRightDown(wxMouseEvent& evt)
{
    m_mouse_input->HandleRightDown(evt);
}

void GLCanvas::OnRightUp(wxMouseEvent& evt)
{
    m_mouse_input->HandleRightUp(evt);
}

void GLCanvas::OnMouseLeave(wxMouseEvent& evt)
{
    m_mouse_input->HandleMouseLeave(evt);
}
