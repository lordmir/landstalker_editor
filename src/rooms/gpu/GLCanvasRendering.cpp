#include "GLCanvas.h"
#include "GLLoader.h"
#include "GLCanvasHeightmapMode.h"
#include "GLCanvasLayerEditMode.h"
#include "GLCanvasRoomMode.h"

#include <algorithm>
#include <cmath>
#include <wx/dcclient.h>
#include <wx/log.h>

namespace {

constexpr long kTargetFrameMs = 1000 / 60;

}  // namespace

void GLCanvas::OnIdle(wxIdleEvent& evt)
{
    if (!m_initialized || !m_gd || !IsShownOnScreen()) {
        return;
    }

    long now_ms = m_anim_stopwatch.Time();
    if (now_ms - m_last_frame_ms >= kTargetFrameMs) {
        float dt = std::clamp((now_ms - m_last_anim_ms) / 1000.0f, 0.0f, 0.1f);
        m_last_anim_ms = now_ms;

        if (!IsAnyEditMode()) {
            GLCanvasRoomMode(*this).UpdateAnimations(dt);
        }
        Refresh(false);
    }

    evt.RequestMore();
}

void GLCanvas::RecordRenderedFrame()
{
    m_last_frame_ms = m_anim_stopwatch.Time();
    m_frame_count++;
    if (m_fps_stopwatch.Time() >= 1000) {
        m_fps = (m_frame_count * 1000.0f) / m_fps_stopwatch.Time();
        m_frame_count = 0;
        m_fps_stopwatch.Start();
        UpdateStatusBar();
    }
}

void GLCanvas::OnPaint(wxPaintEvent&)
{
    // This is the entry point for drawing. wxPaintDC is a helper that ensures
    // the windowing system knows we are drawing.
    wxPaintDC dc(this);
    if (!m_gd) {
        return;
    }
    if (!m_context) {
        m_context = new wxGLContext(this);
    }

    // Set the current OpenGL context to this window. A failed SetCurrent is
    // usually transient (e.g. the window is not yet shown on screen), so return
    // without latching m_gl_init_failed and let the next paint retry. Only the
    // genuine initialization failures below are treated as permanent.
    if (!m_context || !SetCurrent(*m_context)) {
        return;
    }

    // Perform one-time initialization of shaders and textures.
    if (m_gl_init_failed) {
        return;
    }

    if (!m_initialized) {
        if (!InitGLLoader()) {
            m_gl_init_failed = true;
            wxLogError("OpenGL initialization failed. Check GLEW and graphics driver setup.");
            return;
        }
        m_mapRenderer.Init();
        m_spriteRenderer.Init();
        LoadRoom(m_current_room);
        m_initialized = true;
    }

    long now_ms = m_anim_stopwatch.Time();
    if (now_ms - m_last_frame_ms < kTargetFrameMs) {
        return;
    }

    // wx reports client size in logical pixels; OpenGL needs the backing framebuffer size.
    int w = 0;
    int h = 0;
    GetClientSize(&w, &h);
    const float content_scale = static_cast<float>(GetContentScaleFactor());
    const int fb_w = std::max(1, static_cast<int>(std::lround(static_cast<float>(w) * content_scale)));
    const int fb_h = std::max(1, static_cast<int>(std::lround(static_cast<float>(h) * content_scale)));
    glViewport(0, 0, fb_w, fb_h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, h, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(m_cam_x, m_cam_y, 0.0f);
    glScalef(ZoomFactor(), ZoomFactor(), 1.0f);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    if (IsHeightmapEditMode()) {
        GLCanvasHeightmapMode(*this).Render(w, h);
        return;
    }

    if (IsLayerEditMode()) {
        GLCanvasLayerEditMode(*this).Render(w, h);
        return;
    }

    GLCanvasRoomMode(*this).Render(w, h);
}
