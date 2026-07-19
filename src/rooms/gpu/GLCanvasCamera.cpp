#include "GLCanvas.h"
#include "RoomProjection.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace {

constexpr std::array<float, 5> kZoomSteps = {0.5f, 1.0f, 2.0f, 3.0f, 4.0f};
using PickPoint = RoomProjection::PickPoint;

}  // namespace

void MyGLCanvas::SetZoom(double zoom)
{
    int best_idx = 0;
    double best_distance = std::abs(static_cast<double>(kZoomSteps[0]) - zoom);
    for (std::size_t i = 1; i < kZoomSteps.size(); ++i) {
        const double distance = std::abs(static_cast<double>(kZoomSteps[i]) - zoom);
        if (distance < best_distance) {
            best_idx = static_cast<int>(i);
            best_distance = distance;
        }
    }

    int old_idx = std::clamp(m_zoom_step_idx, 0, static_cast<int>(kZoomSteps.size()) - 1);
    if (best_idx == old_idx) {
        return;
    }

    int width = 0;
    int height = 0;
    GetClientSize(&width, &height);
    ChangeZoomStep(best_idx - old_idx, static_cast<float>(width) * 0.5f, static_cast<float>(height) * 0.5f);
    Refresh();
}

void MyGLCanvas::PanCameraByStep(int dx, int dy, float speed)
{
    m_cam_x += speed * static_cast<float>(dx);
    m_cam_y += speed * static_cast<float>(dy);
    m_cam_x = std::round(m_cam_x);
    m_cam_y = std::round(m_cam_y);
}

void MyGLCanvas::ChangeZoomStep(int delta, float anchor_x, float anchor_y)
{
    int old_idx = std::clamp(m_zoom_step_idx, 0, static_cast<int>(kZoomSteps.size()) - 1);
    int new_idx = std::clamp(old_idx + delta, 0, static_cast<int>(kZoomSteps.size()) - 1);
    if (new_idx == old_idx) {
        return;
    }
    float old_zoom = kZoomSteps[static_cast<std::size_t>(old_idx)];
    float new_zoom = kZoomSteps[static_cast<std::size_t>(new_idx)];
    float world_x = (anchor_x - m_cam_x) / old_zoom;
    float world_y = (anchor_y - m_cam_y) / old_zoom;
    m_zoom_step_idx = new_idx;
    m_cam_x = anchor_x - world_x * new_zoom;
    m_cam_y = anchor_y - world_y * new_zoom;
    m_cam_x = std::round(m_cam_x);
    m_cam_y = std::round(m_cam_y);
}

float MyGLCanvas::ZoomFactor() const
{
    int idx = std::clamp(m_zoom_step_idx, 0, static_cast<int>(kZoomSteps.size()) - 1);
    return kZoomSteps[static_cast<std::size_t>(idx)];
}

float MyGLCanvas::ScreenToWorldX(int screen_x) const
{
    return (static_cast<float>(screen_x) - m_cam_x) / ZoomFactor();
}

float MyGLCanvas::ScreenToWorldY(int screen_y) const
{
    return (static_cast<float>(screen_y) - m_cam_y) / ZoomFactor();
}

void MyGLCanvas::CenterCameraOnRoom()
{
    int client_w = 0;
    int client_h = 0;
    GetClientSize(&client_w, &client_h);
    if (client_w <= 0 || client_h <= 0 || m_mapRenderer.GetRoomWidth() <= 0 || m_mapRenderer.GetRoomHeight() <= 0) {
        m_cam_x = 0.0f;
        m_cam_y = 0.0f;
        return;
    }

    auto project = [](float x, float y) {
        return PickPoint{
            32.0f * x - 32.0f * y + 512.0f,
            16.0f * x + 16.0f * y + 100.0f
        };
    };

    float room_w = float(m_mapRenderer.GetRoomWidth());
    float room_h = float(m_mapRenderer.GetRoomHeight());
    std::array<PickPoint, 4> corners = {
        project(0.0f, 0.0f),
        project(room_w, 0.0f),
        project(0.0f, room_h),
        project(room_w, room_h)
    };

    float min_x = corners.front().x;
    float max_x = corners.front().x;
    float min_y = corners.front().y;
    float max_y = corners.front().y;
    for (const auto& point : corners) {
        min_x = std::min(min_x, point.x);
        max_x = std::max(max_x, point.x);
        min_y = std::min(min_y, point.y);
        max_y = std::max(max_y, point.y);
    }

    float zoom = ZoomFactor();
    m_cam_x = float(client_w) * 0.5f - ((min_x + max_x) * 0.5f) * zoom;
    m_cam_y = float(client_h) * 0.5f - ((min_y + max_y) * 0.5f) * zoom;
    m_cam_x = std::round(m_cam_x);
    m_cam_y = std::round(m_cam_y);
}

void MyGLCanvas::EnsureWorldRectVisible(float min_x, float min_y, float max_x, float max_y)
{
    int client_w = 0;
    int client_h = 0;
    GetClientSize(&client_w, &client_h);
    if (client_w <= 0 || client_h <= 0) {
        return;
    }

    if (min_x > max_x) {
        std::swap(min_x, max_x);
    }
    if (min_y > max_y) {
        std::swap(min_y, max_y);
    }

    float zoom = std::max(ZoomFactor(), 0.0001f);
    float view_min_x = ScreenToWorldX(0);
    float view_min_y = ScreenToWorldY(0);
    float view_max_x = ScreenToWorldX(client_w);
    float view_max_y = ScreenToWorldY(client_h);
    if (view_min_x > view_max_x) {
        std::swap(view_min_x, view_max_x);
    }
    if (view_min_y > view_max_y) {
        std::swap(view_min_y, view_max_y);
    }

    constexpr float margin_px = 36.0f;
    float margin_world = margin_px / zoom;
    bool visible_x = min_x >= view_min_x + margin_world && max_x <= view_max_x - margin_world;
    bool visible_y = min_y >= view_min_y + margin_world && max_y <= view_max_y - margin_world;
    if (visible_x && visible_y) {
        return;
    }

    float center_x = (min_x + max_x) * 0.5f;
    float center_y = (min_y + max_y) * 0.5f;
    m_cam_x = float(client_w) * 0.5f - center_x * zoom;
    m_cam_y = float(client_h) * 0.5f - center_y * zoom;
    m_cam_x = std::round(m_cam_x);
    m_cam_y = std::round(m_cam_y);
}
