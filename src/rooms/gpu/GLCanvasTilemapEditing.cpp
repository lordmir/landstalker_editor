#include "GLCanvas.h"
#include "RoomProjection.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <map>
#include <set>
#include <utility>
#include <vector>

using namespace Landstalker;
using PickPoint = RoomProjection::PickPoint;

static void PostLayerBlockSelection(wxWindow* target, MyGLCanvas* canvas, int block_id) {
    if (!target) {
        return;
    }
    wxCommandEvent evt(EVT_GPU_LAYER_BLOCK_SELECT);
    evt.SetInt(block_id);
    evt.SetClientData(canvas);
    wxPostEvent(target, evt);
}

void MyGLCanvas::ClearEditSelection() {
    m_background_has_selection = false;
    m_background_selected_x = 0;
    m_background_selected_y = 0;
    m_heightmap_selection_anchor_x = 0;
    m_heightmap_selection_anchor_y = 0;
    m_heightmap_selection_drag_anchor_x = 0;
    m_heightmap_selection_drag_anchor_y = 0;
    m_layer_selection_anchor_x = 0;
    m_layer_selection_anchor_y = 0;
    m_layer_selection_drag_anchor_x = 0;
    m_layer_selection_drag_anchor_y = 0;
    ResetLayerEditState();
    ResetHeightmapEditState();
    m_heightmap_selected_cells.clear();
    m_layer_selected_cells.clear();
    if (HasCapture()) {
        ReleaseMouse();
    }
    NotifyHeightmapTargetChanged();
}

void MyGLCanvas::BeginLayerSelectionDrag(int x, int y, bool add_to_selection, bool subtract_from_selection, bool parallelogram_selection) {
    m_layer_dragging_select = true;
    m_layer_selection_add = add_to_selection && !subtract_from_selection;
    m_layer_selection_subtract = subtract_from_selection;
    m_layer_selection_parallelogram = parallelogram_selection;
    m_layer_selection_drag_base = m_layer_selected_cells;
    m_layer_selection_drag_anchor_x = x;
    m_layer_selection_drag_anchor_y = y;

    if (!m_layer_selection_add && !m_layer_selection_subtract) {
        m_layer_selection_drag_base.clear();
    }

    if (!m_layer_selection_subtract) {
        m_layer_selection_anchor_x = x;
        m_layer_selection_anchor_y = y;
    }
    m_background_selected_x = x;
    m_background_selected_y = y;
    UpdateLayerSelectionDrag(x, y);
    NotifyLayerBlockSelected();
}

void MyGLCanvas::UpdateLayerSelectionDrag(int x, int y) {
    auto map = CurrentRoomMap();
    if (!map) {
        return;
    }

    const int width = m_mapRenderer.GetRoomWidth();
    const int height = m_mapRenderer.GetRoomHeight();
    if (width <= 0 || height <= 0) {
        return;
    }
    m_background_selected_x = std::clamp(x, 0, width - 1);
    m_background_selected_y = std::clamp(y, 0, height - 1);
    m_background_has_selection = true;

    m_layer_selected_cells = m_layer_selection_drag_base;
    auto apply_cell = [this, width, height](int cell_x, int cell_y) {
        if (cell_x < 0 || cell_y < 0 || cell_x >= width || cell_y >= height) {
            return;
        }
        if (m_layer_selection_subtract) {
            m_layer_selected_cells.erase({cell_x, cell_y});
        } else {
            m_layer_selected_cells.insert({cell_x, cell_y});
        }
    };

    if (m_layer_selection_parallelogram) {
        int anchor_u = m_layer_selection_drag_anchor_x - m_layer_selection_drag_anchor_y;
        int target_u = x - y;
        int min_u = std::min(anchor_u, target_u);
        int max_u = std::max(anchor_u, target_u);
        bool track_x_axis = target_u < anchor_u ||
            (target_u == anchor_u && x < m_layer_selection_drag_anchor_x);
        int anchor_axis = track_x_axis ? m_layer_selection_drag_anchor_x : m_layer_selection_drag_anchor_y;
        int target_axis = track_x_axis ? x : y;
        int min_axis = std::min(anchor_axis, target_axis);
        int max_axis = std::max(anchor_axis, target_axis);
        for (int cell_y = 0; cell_y < height; ++cell_y) {
            for (int cell_x = 0; cell_x < width; ++cell_x) {
                int u = cell_x - cell_y;
                int axis = track_x_axis ? cell_x : cell_y;
                if (u >= min_u && u <= max_u && axis >= min_axis && axis <= max_axis) {
                    apply_cell(cell_x, cell_y);
                }
            }
        }
    } else {
        int min_x = std::min(m_layer_selection_drag_anchor_x, x);
        int max_x = std::max(m_layer_selection_drag_anchor_x, x);
        int min_y = std::min(m_layer_selection_drag_anchor_y, y);
        int max_y = std::max(m_layer_selection_drag_anchor_y, y);
        for (int cell_y = std::max(min_y, 0); cell_y <= std::min(max_y, height - 1); ++cell_y) {
            for (int cell_x = std::max(min_x, 0); cell_x <= std::min(max_x, width - 1); ++cell_x) {
                apply_cell(cell_x, cell_y);
            }
        }
    }

    if (m_layer_selected_cells.empty()) {
        ClearEditSelection();
        return;
    }

    if (m_layer_selected_cells.find({m_layer_selection_anchor_x, m_layer_selection_anchor_y}) == m_layer_selected_cells.end()) {
        const auto& primary = *m_layer_selected_cells.begin();
        m_layer_selection_anchor_x = primary.first;
        m_layer_selection_anchor_y = primary.second;
    }
    m_background_selected_x = m_layer_selection_anchor_x;
    m_background_selected_y = m_layer_selection_anchor_y;
}

void MyGLCanvas::FinishLayerSelectionDrag() {
    m_layer_dragging_select = false;
    m_layer_selection_add = false;
    m_layer_selection_subtract = false;
    m_layer_selection_parallelogram = false;
    m_layer_selection_drag_base.clear();
    NotifyLayerBlockSelected();
}

bool MyGLCanvas::IsLayerCellSelected(int x, int y) const {
    return m_layer_selected_cells.find({x, y}) != m_layer_selected_cells.end();
}

void MyGLCanvas::BeginLayerSelectionMoveDrag(int x, int y) {
    auto map = CurrentRoomMap();
    if (!map || !IsLayerCellSelected(x, y)) {
        return;
    }

    m_layer_dragging_selection_move = true;
    m_layer_selection_move_anchor_x = x;
    m_layer_selection_move_anchor_y = y;
    m_layer_selection_move_delta_x = 0;
    m_layer_selection_move_delta_y = 0;
    m_layer_selection_move_values.clear();

    Tilemap3D::Layer layer = CurrentEditLayer();
    const int width = m_mapRenderer.GetRoomWidth();
    const int height = m_mapRenderer.GetRoomHeight();
    for (const auto& cell : m_layer_selected_cells) {
        int cell_x = cell.first;
        int cell_y = cell.second;
        if (cell_x < 0 || cell_y < 0 || cell_x >= width || cell_y >= height) {
            continue;
        }
        int block_index = cell_y * width + cell_x;
        if (block_index < 0 || block_index >= map->GetWidth() * map->GetHeight()) {
            continue;
        }
        m_layer_selection_move_values[cell] = map->GetBlock(static_cast<uint16_t>(block_index), layer).value;
    }
}

void MyGLCanvas::UpdateLayerSelectionMoveDrag(int x, int y) {
    auto map = CurrentRoomMap();
    if (!map || !m_layer_dragging_selection_move) {
        return;
    }

    int dx = x - m_layer_selection_move_anchor_x;
    int dy = y - m_layer_selection_move_anchor_y;
    int min_dx = 0;
    int max_dx = 0;
    int min_dy = 0;
    int max_dy = 0;
    bool first = true;
    const int width = m_mapRenderer.GetRoomWidth();
    const int height = m_mapRenderer.GetRoomHeight();
    for (const auto& cell : m_layer_selected_cells) {
        int cell_min_dx = -cell.first;
        int cell_max_dx = width - 1 - cell.first;
        int cell_min_dy = -cell.second;
        int cell_max_dy = height - 1 - cell.second;
        if (first) {
            min_dx = cell_min_dx;
            max_dx = cell_max_dx;
            min_dy = cell_min_dy;
            max_dy = cell_max_dy;
            first = false;
        } else {
            min_dx = std::max(min_dx, cell_min_dx);
            max_dx = std::min(max_dx, cell_max_dx);
            min_dy = std::max(min_dy, cell_min_dy);
            max_dy = std::min(max_dy, cell_max_dy);
        }
    }

    m_layer_selection_move_delta_x = std::clamp(dx, min_dx, max_dx);
    m_layer_selection_move_delta_y = std::clamp(dy, min_dy, max_dy);
}

void MyGLCanvas::CommitLayerSelectionMoveDrag() {
    auto map = CurrentRoomMap();
    if (!map || !m_layer_dragging_selection_move) {
        CancelLayerSelectionMoveDrag();
        return;
    }

    int dx = m_layer_selection_move_delta_x;
    int dy = m_layer_selection_move_delta_y;
    if (dx == 0 && dy == 0) {
        CancelLayerSelectionMoveDrag();
        return;
    }

    CaptureUndoState();

    Tilemap3D::Layer layer = CurrentEditLayer();
    const int width = m_mapRenderer.GetRoomWidth();
    for (const auto& source : m_layer_selection_move_values) {
        int x = source.first.first;
        int y = source.first.second;
        int block_index = y * width + x;
        map->SetBlock(0, static_cast<uint16_t>(block_index), layer);
        if (m_tileswap_preview_map) {
            m_tileswap_preview_map->SetBlock(0, static_cast<uint16_t>(block_index), layer);
        }
    }

    std::set<std::pair<int, int>> moved_selection;
    for (const auto& source : m_layer_selection_move_values) {
        int x = source.first.first + dx;
        int y = source.first.second + dy;
        int block_index = y * width + x;
        map->SetBlock(source.second, static_cast<uint16_t>(block_index), layer);
        if (m_tileswap_preview_map) {
            m_tileswap_preview_map->SetBlock(source.second, static_cast<uint16_t>(block_index), layer);
        }
        moved_selection.insert({x, y});
    }

    m_layer_selected_cells = std::move(moved_selection);
    m_layer_selection_anchor_x += dx;
    m_layer_selection_anchor_y += dy;
    m_background_selected_x = m_layer_selection_anchor_x;
    m_background_selected_y = m_layer_selection_anchor_y;
    m_background_has_selection = !m_layer_selected_cells.empty();
    m_layer_dragging_selection_move = false;
    m_layer_selection_move_values.clear();
    m_layer_selection_move_anchor_x = -1;
    m_layer_selection_move_anchor_y = -1;
    m_layer_selection_move_delta_x = 0;
    m_layer_selection_move_delta_y = 0;
    ReloadCurrentRoomMapView();
    NotifyLayerBlockSelected();
}

void MyGLCanvas::CancelLayerSelectionMoveDrag() {
    m_layer_dragging_selection_move = false;
    m_layer_selection_move_anchor_x = -1;
    m_layer_selection_move_anchor_y = -1;
    m_layer_selection_move_delta_x = 0;
    m_layer_selection_move_delta_y = 0;
    m_layer_selection_move_values.clear();
}

std::pair<int, int> MyGLCanvas::SnapLayerLineEnd(int start_x, int start_y, int end_x, int end_y) const {
    int dx = end_x - start_x;
    int dy = end_y - start_y;
    if (dx == 0 && dy == 0) {
        return {end_x, end_y};
    }

    int k = static_cast<int>(std::round(static_cast<double>(dx + dy) * 0.5));

    std::array<std::pair<int, int>, 3> candidates{{
        {end_x, start_y},
        {start_x, end_y},
        {start_x + k, start_y + k}
    }};

    auto distance_sq = [end_x, end_y](const std::pair<int, int>& point) {
        int ddx = point.first - end_x;
        int ddy = point.second - end_y;
        return ddx * ddx + ddy * ddy;
    };
    return *std::min_element(candidates.begin(), candidates.end(), [&](const auto& a, const auto& b) {
        return distance_sq(a) < distance_sq(b);
    });
}

std::vector<std::pair<int, int>> MyGLCanvas::BuildLayerScreenLineCells(int start_x, int start_y, int end_x, int end_y) const {
    std::set<std::pair<int, int>> unique_cells;
    const int width = m_mapRenderer.GetRoomWidth();
    const int height = m_mapRenderer.GetRoomHeight();
    if (width <= 0 || height <= 0) {
        return {};
    }

    auto to_screen = [](int x, int y) {
        return PickPoint{
            32.0f * static_cast<float>(x - y),
            16.0f * static_cast<float>(x + y)
        };
    };
    auto add_screen_point = [&](float sx, float sy) {
        float map_x = ((sy / 16.0f) + (sx / 32.0f)) * 0.5f;
        float map_y = ((sy / 16.0f) - (sx / 32.0f)) * 0.5f;
        int cell_x = static_cast<int>(std::round(map_x));
        int cell_y = static_cast<int>(std::round(map_y));
        if (cell_x >= 0 && cell_y >= 0 && cell_x < width && cell_y < height) {
            unique_cells.insert({cell_x, cell_y});
        }
    };

    PickPoint start = to_screen(start_x, start_y);
    PickPoint end = to_screen(end_x, end_y);
    float dx = end.x - start.x;
    float dy = end.y - start.y;
    float distance = std::sqrt(dx * dx + dy * dy);
    int steps = std::max(1, static_cast<int>(std::ceil(distance / 8.0f)));
    for (int i = 0; i <= steps; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(steps);
        add_screen_point(start.x + dx * t, start.y + dy * t);
    }

    return {unique_cells.begin(), unique_cells.end()};
}

std::vector<std::pair<int, int>> MyGLCanvas::BuildLayerScreenCircleCells(
    int start_x,
    int start_y,
    int end_x,
    int end_y,
    bool filled) const {
    std::vector<std::pair<int, int>> cells;
    const int width = m_mapRenderer.GetRoomWidth();
    const int height = m_mapRenderer.GetRoomHeight();

    auto to_screen = [](int x, int y) {
        return PickPoint{
            32.0f * static_cast<float>(x - y),
            16.0f * static_cast<float>(x + y)
        };
    };

    PickPoint start = to_screen(start_x, start_y);
    PickPoint end = to_screen(end_x, end_y);
    float min_x = std::min(start.x, end.x);
    float max_x = std::max(start.x, end.x);
    float min_y = std::min(start.y, end.y);
    float max_y = std::max(start.y, end.y);
    float radius_x = std::max(16.0f, (max_x - min_x + 32.0f) * 0.5f);
    float radius_y = std::max(8.0f, (max_y - min_y + 16.0f) * 0.5f);
    float center_x = (min_x + max_x) * 0.5f;
    float center_y = (min_y + max_y) * 0.5f;

    auto inside = [&](int x, int y) {
        PickPoint point = to_screen(x, y);
        float dx = (point.x - center_x) / radius_x;
        float dy = (point.y - center_y) / radius_y;
        return dx * dx + dy * dy <= 1.0f;
    };

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (!inside(x, y)) {
                continue;
            }
            bool outline = !inside(x - 1, y) || !inside(x + 1, y) ||
                           !inside(x, y - 1) || !inside(x, y + 1);
            if (filled || outline) {
                cells.push_back({x, y});
            }
        }
    }

    return cells;
}

std::vector<std::pair<int, int>> MyGLCanvas::BuildLayerSkewRectCells(
    int start_x,
    int start_y,
    int end_x,
    int end_y,
    bool filled,
    bool lock_equal) const {
    std::vector<std::pair<int, int>> cells;
    const int width = m_mapRenderer.GetRoomWidth();
    const int height = m_mapRenderer.GetRoomHeight();
    int anchor_u = start_x - start_y;
    int target_u = end_x - end_y;
    bool track_x_axis = target_u < anchor_u || (target_u == anchor_u && end_x < start_x);
    int anchor_axis = track_x_axis ? start_x : start_y;
    int target_axis = track_x_axis ? end_x : end_y;

    if (lock_equal) {
        int du = target_u - anchor_u;
        int da = target_axis - anchor_axis;
        int size = std::max(std::abs(du), std::abs(da));
        int step_u = du < 0 ? -1 : 1;
        int step_axis = da < 0 ? -1 : 1;
        target_u = anchor_u + step_u * size;
        target_axis = anchor_axis + step_axis * size;
    }

    int min_u = std::min(anchor_u, target_u);
    int max_u = std::max(anchor_u, target_u);
    int min_axis = std::min(anchor_axis, target_axis);
    int max_axis = std::max(anchor_axis, target_axis);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int u = x - y;
            int axis = track_x_axis ? x : y;
            if (u < min_u || u > max_u || axis < min_axis || axis > max_axis) {
                continue;
            }
            if (filled || u == min_u || u == max_u || axis == min_axis || axis == max_axis) {
                cells.push_back({x, y});
            }
        }
    }

    return cells;
}

std::vector<std::pair<int, int>> MyGLCanvas::BuildLayerSkewCircleCells(
    int start_x,
    int start_y,
    int end_x,
    int end_y,
    bool filled,
    bool lock_equal) const {
    std::vector<std::pair<int, int>> cells;
    const int width = m_mapRenderer.GetRoomWidth();
    const int height = m_mapRenderer.GetRoomHeight();
    int anchor_u = start_x - start_y;
    int target_u = end_x - end_y;
    bool track_x_axis = target_u < anchor_u || (target_u == anchor_u && end_x < start_x);
    int anchor_axis = track_x_axis ? start_x : start_y;
    int target_axis = track_x_axis ? end_x : end_y;

    if (lock_equal) {
        int du = target_u - anchor_u;
        int da = target_axis - anchor_axis;
        int size = std::max(std::abs(du), std::abs(da));
        int step_u = du < 0 ? -1 : 1;
        int step_axis = da < 0 ? -1 : 1;
        target_u = anchor_u + step_u * size;
        target_axis = anchor_axis + step_axis * size;
    }

    int min_u = std::min(anchor_u, target_u);
    int max_u = std::max(anchor_u, target_u);
    int min_axis = std::min(anchor_axis, target_axis);
    int max_axis = std::max(anchor_axis, target_axis);
    float radius_u = std::max(0.5f, (static_cast<float>(max_u - min_u) + 1.0f) * 0.5f);
    float radius_axis = std::max(0.5f, (static_cast<float>(max_axis - min_axis) + 1.0f) * 0.5f);
    float center_u = (static_cast<float>(min_u) + static_cast<float>(max_u) + 1.0f) * 0.5f;
    float center_axis = (static_cast<float>(min_axis) + static_cast<float>(max_axis) + 1.0f) * 0.5f;

    auto inside = [&](int u, int axis) {
        float du = (static_cast<float>(u) + 0.5f - center_u) / radius_u;
        float da = (static_cast<float>(axis) + 0.5f - center_axis) / radius_axis;
        return du * du + da * da <= 1.0f;
    };

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int u = x - y;
            int axis = track_x_axis ? x : y;
            if (!inside(u, axis)) {
                continue;
            }
            bool outline = !inside(u - 1, axis) || !inside(u + 1, axis) ||
                           !inside(u, axis - 1) || !inside(u, axis + 1);
            if (filled || outline) {
                cells.push_back({x, y});
            }
        }
    }

    return cells;
}

void MyGLCanvas::BeginLayerLineDrag(int x, int y, bool shift_down, bool alt_down) {
    m_layer_dragging_line = true;
    m_layer_line_start_x = x;
    m_layer_line_start_y = y;
    UpdateLayerLineDrag(x, y, shift_down, alt_down);
}

void MyGLCanvas::UpdateLayerLineDrag(int x, int y, bool shift_down, bool alt_down) {
    auto map = CurrentRoomMap();
    if (!map || !m_layer_dragging_line) {
        return;
    }

    std::pair<int, int> end{x, y};
    if (m_drawing_tool == DrawingTool::Line) {
        end = shift_down ? end : SnapLayerLineEnd(m_layer_line_start_x, m_layer_line_start_y, x, y);
    } else if (shift_down && !alt_down) {
        int dx = x - m_layer_line_start_x;
        int dy = y - m_layer_line_start_y;
        int size = std::max(std::abs(dx), std::abs(dy));
        int step_x = dx < 0 ? -1 : 1;
        int step_y = dy < 0 ? -1 : 1;
        end = {
            m_layer_line_start_x + step_x * size,
            m_layer_line_start_y + step_y * size
        };
    }
    int end_x = end.first;
    int end_y = end.second;

    m_layer_line_end_x = end_x;
    m_layer_line_end_y = end_y;
    switch (m_drawing_tool) {
        case DrawingTool::FilledRect:
            m_layer_line_preview_cells = alt_down ?
                BuildLayerSkewRectCells(m_layer_line_start_x, m_layer_line_start_y, end_x, end_y, true, shift_down) :
                BuildHeightmapRectCells(m_layer_line_start_x, m_layer_line_start_y, end_x, end_y, true);
            break;
        case DrawingTool::OutlineRect:
            m_layer_line_preview_cells = alt_down ?
                BuildLayerSkewRectCells(m_layer_line_start_x, m_layer_line_start_y, end_x, end_y, false, shift_down) :
                BuildHeightmapRectCells(m_layer_line_start_x, m_layer_line_start_y, end_x, end_y, false);
            break;
        case DrawingTool::FilledCircle:
            m_layer_line_preview_cells = alt_down ?
                BuildLayerSkewCircleCells(m_layer_line_start_x, m_layer_line_start_y, end_x, end_y, true, shift_down) :
                BuildLayerScreenCircleCells(m_layer_line_start_x, m_layer_line_start_y, end_x, end_y, true);
            break;
        case DrawingTool::OutlineCircle:
            m_layer_line_preview_cells = alt_down ?
                BuildLayerSkewCircleCells(m_layer_line_start_x, m_layer_line_start_y, end_x, end_y, false, shift_down) :
                BuildLayerScreenCircleCells(m_layer_line_start_x, m_layer_line_start_y, end_x, end_y, false);
            break;
        default:
            m_layer_line_preview_cells = shift_down ?
                BuildLayerScreenLineCells(m_layer_line_start_x, m_layer_line_start_y, end_x, end_y) :
                BuildHeightmapLineCells(m_layer_line_start_x, m_layer_line_start_y, end_x, end_y);
            break;
    }
}

void MyGLCanvas::CommitLayerLineDrag() {
    if (!m_layer_dragging_line || !m_background_clipboard_valid) {
        CancelLayerLineDrag();
        return;
    }

    bool changed = false;
    for (const auto& cell : m_layer_line_preview_cells) {
        changed = PasteBackgroundBlockAt(cell.first, cell.second, true) || changed;
    }
    m_layer_dragging_line = false;
    m_layer_line_preview_cells.clear();
    m_layer_line_start_x = -1;
    m_layer_line_start_y = -1;
    m_layer_line_end_x = -1;
    m_layer_line_end_y = -1;
    if (changed) {
        CommitLayerDrawStroke();
    } else {
        m_layer_draw_dirty = false;
    }
}

void MyGLCanvas::CancelLayerLineDrag() {
    m_layer_dragging_line = false;
    m_layer_line_preview_cells.clear();
    m_layer_line_start_x = -1;
    m_layer_line_start_y = -1;
    m_layer_line_end_x = -1;
    m_layer_line_end_y = -1;
    m_layer_draw_dirty = false;
}

void MyGLCanvas::BeginHeightmapSelectionDrag(int x, int y, bool add_to_selection, bool subtract_from_selection) {
    m_heightmap_dragging_select = true;
    m_heightmap_selection_add = add_to_selection && !subtract_from_selection;
    m_heightmap_selection_subtract = subtract_from_selection;
    m_heightmap_selection_drag_base = m_heightmap_selected_cells;
    m_heightmap_selection_drag_anchor_x = x;
    m_heightmap_selection_drag_anchor_y = y;

    if (!m_heightmap_selection_add && !m_heightmap_selection_subtract) {
        m_heightmap_selection_drag_base.clear();
    }

    if (!m_heightmap_selection_subtract) {
        m_heightmap_selection_anchor_x = x;
        m_heightmap_selection_anchor_y = y;
    }
    m_background_selected_x = x;
    m_background_selected_y = y;
    UpdateHeightmapSelectionDrag(x, y);
    NotifyHeightmapTargetChanged();
}

void MyGLCanvas::UpdateHeightmapSelectionDrag(int x, int y) {
    auto map = CurrentRoomMap();
    if (!map) {
        return;
    }

    const int width = map->GetHeightmapWidth();
    const int height = map->GetHeightmapHeight();
    if (width <= 0 || height <= 0) {
        return;
    }

    m_background_selected_x = std::clamp(x, 0, width - 1);
    m_background_selected_y = std::clamp(y, 0, height - 1);
    m_background_has_selection = true;

    int min_x = std::min(m_heightmap_selection_drag_anchor_x, x);
    int max_x = std::max(m_heightmap_selection_drag_anchor_x, x);
    int min_y = std::min(m_heightmap_selection_drag_anchor_y, y);
    int max_y = std::max(m_heightmap_selection_drag_anchor_y, y);

    m_heightmap_selected_cells = m_heightmap_selection_drag_base;
    for (int cell_y = std::max(min_y, 0); cell_y <= std::min(max_y, height - 1); ++cell_y) {
        for (int cell_x = std::max(min_x, 0); cell_x <= std::min(max_x, width - 1); ++cell_x) {
            if (m_heightmap_selection_subtract) {
                m_heightmap_selected_cells.erase({cell_x, cell_y});
            } else {
                m_heightmap_selected_cells.insert({cell_x, cell_y});
            }
        }
    }

    if (m_heightmap_selected_cells.empty()) {
        ClearEditSelection();
        return;
    }

    if (m_heightmap_selected_cells.find({m_heightmap_selection_anchor_x, m_heightmap_selection_anchor_y}) == m_heightmap_selected_cells.end()) {
        const auto& primary = *m_heightmap_selected_cells.begin();
        m_heightmap_selection_anchor_x = primary.first;
        m_heightmap_selection_anchor_y = primary.second;
    }
}

void MyGLCanvas::FinishHeightmapSelectionDrag() {
    m_heightmap_dragging_select = false;
    m_heightmap_selection_add = false;
    m_heightmap_selection_subtract = false;
    m_heightmap_selection_drag_base.clear();
    NotifyHeightmapTargetChanged();
}

bool MyGLCanvas::IsHeightmapCellSelected(int x, int y) const {
    return m_heightmap_selected_cells.find({x, y}) != m_heightmap_selected_cells.end();
}

void MyGLCanvas::BeginHeightmapSelectionMoveDrag(int x, int y) {
    auto map = CurrentRoomMap();
    if (!map || !IsHeightmapCellSelected(x, y)) {
        return;
    }

    m_heightmap_dragging_selection_move = true;
    m_heightmap_selection_move_anchor_x = x;
    m_heightmap_selection_move_anchor_y = y;
    m_heightmap_selection_move_delta_x = 0;
    m_heightmap_selection_move_delta_y = 0;
    m_heightmap_selection_move_values.clear();
    m_heightmap_line_preview_cells.clear();

    for (const auto& cell : m_heightmap_selected_cells) {
        int cell_x = cell.first;
        int cell_y = cell.second;
        if (cell_x < 0 || cell_y < 0 || cell_x >= map->GetHeightmapWidth() || cell_y >= map->GetHeightmapHeight()) {
            continue;
        }
        m_heightmap_selection_move_values[cell] = map->GetHeightmapCell({cell_x, cell_y});
        m_heightmap_line_preview_cells.push_back(cell);
    }
}

void MyGLCanvas::UpdateHeightmapSelectionMoveDrag(int x, int y) {
    auto map = CurrentRoomMap();
    if (!map || !m_heightmap_dragging_selection_move) {
        return;
    }

    int dx = x - m_heightmap_selection_move_anchor_x;
    int dy = y - m_heightmap_selection_move_anchor_y;
    int min_dx = 0;
    int max_dx = 0;
    int min_dy = 0;
    int max_dy = 0;
    bool first = true;
    for (const auto& cell : m_heightmap_selected_cells) {
        int cell_x = cell.first;
        int cell_y = cell.second;
        int cell_min_dx = -cell_x;
        int cell_max_dx = map->GetHeightmapWidth() - 1 - cell_x;
        int cell_min_dy = -cell_y;
        int cell_max_dy = map->GetHeightmapHeight() - 1 - cell_y;
        if (first) {
            min_dx = cell_min_dx;
            max_dx = cell_max_dx;
            min_dy = cell_min_dy;
            max_dy = cell_max_dy;
            first = false;
        } else {
            min_dx = std::max(min_dx, cell_min_dx);
            max_dx = std::min(max_dx, cell_max_dx);
            min_dy = std::max(min_dy, cell_min_dy);
            max_dy = std::min(max_dy, cell_max_dy);
        }
    }

    m_heightmap_selection_move_delta_x = std::clamp(dx, min_dx, max_dx);
    m_heightmap_selection_move_delta_y = std::clamp(dy, min_dy, max_dy);
    m_heightmap_line_preview_cells.clear();
    for (const auto& cell : m_heightmap_selected_cells) {
        m_heightmap_line_preview_cells.push_back({
            cell.first + m_heightmap_selection_move_delta_x,
            cell.second + m_heightmap_selection_move_delta_y
        });
    }
}

void MyGLCanvas::CommitHeightmapSelectionMoveDrag() {
    auto map = CurrentRoomMap();
    if (!map || !m_heightmap_dragging_selection_move) {
        CancelHeightmapSelectionMoveDrag();
        return;
    }

    int dx = m_heightmap_selection_move_delta_x;
    int dy = m_heightmap_selection_move_delta_y;
    if (dx == 0 && dy == 0) {
        CancelHeightmapSelectionMoveDrag();
        return;
    }

    CaptureUndoState();

    static constexpr uint16_t kClearedHeightmapCell = 0x4000;
    for (const auto& source : m_heightmap_selection_move_values) {
        int x = source.first.first;
        int y = source.first.second;
        map->SetHeightmapCell({x, y}, kClearedHeightmapCell);
        if (m_tileswap_preview_map) {
            m_tileswap_preview_map->SetHeightmapCell({x, y}, kClearedHeightmapCell);
        }
    }

    std::set<std::pair<int, int>> moved_selection;
    for (const auto& source : m_heightmap_selection_move_values) {
        int x = source.first.first + dx;
        int y = source.first.second + dy;
        map->SetHeightmapCell({x, y}, source.second);
        if (m_tileswap_preview_map) {
            m_tileswap_preview_map->SetHeightmapCell({x, y}, source.second);
        }
        moved_selection.insert({x, y});
    }

    m_heightmap_selected_cells = std::move(moved_selection);
    if (!m_heightmap_selected_cells.empty()) {
        auto primary = m_heightmap_selected_cells.find({
            m_heightmap_selection_anchor_x + dx,
            m_heightmap_selection_anchor_y + dy
        });
        if (primary == m_heightmap_selected_cells.end()) {
            primary = m_heightmap_selected_cells.begin();
        }
        m_heightmap_selection_anchor_x = primary->first;
        m_heightmap_selection_anchor_y = primary->second;
        m_background_selected_x = primary->first;
        m_background_selected_y = primary->second;
        m_background_has_selection = true;
    }

    m_heightmap_dragging_selection_move = false;
    m_heightmap_selection_move_values.clear();
    m_heightmap_line_preview_cells.clear();
    m_heightmap_selection_move_delta_x = 0;
    m_heightmap_selection_move_delta_y = 0;
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    NotifyHeightmapTargetChanged();
}

void MyGLCanvas::CancelHeightmapSelectionMoveDrag() {
    m_heightmap_dragging_selection_move = false;
    m_heightmap_selection_move_anchor_x = -1;
    m_heightmap_selection_move_anchor_y = -1;
    m_heightmap_selection_move_delta_x = 0;
    m_heightmap_selection_move_delta_y = 0;
    m_heightmap_selection_move_values.clear();
    m_heightmap_line_preview_cells.clear();
}

bool MyGLCanvas::IsHeightmapBrushTool() const {
    return m_drawing_tool == DrawingTool::Draw ||
           m_drawing_tool == DrawingTool::FloodFill ||
           IsHeightmapShapeTool();
}

bool MyGLCanvas::IsHeightmapShapeTool() const {
    return m_drawing_tool == DrawingTool::Line ||
           m_drawing_tool == DrawingTool::FilledRect ||
           m_drawing_tool == DrawingTool::OutlineRect ||
           m_drawing_tool == DrawingTool::FilledCircle ||
           m_drawing_tool == DrawingTool::OutlineCircle;
}

bool MyGLCanvas::IsHeightmapPreviewTool() const {
    return IsHeightmapBrushTool() || m_drawing_tool == DrawingTool::Stamp;
}

std::pair<int, int> MyGLCanvas::SnapHeightmapLineEnd(int start_x, int start_y, int end_x, int end_y) const {
    int dx = end_x - start_x;
    int dy = end_y - start_y;
    int abs_dx = std::abs(dx);
    int abs_dy = std::abs(dy);

    if (abs_dx == 0 && abs_dy == 0) {
        return {end_x, end_y};
    }
    if (abs_dx * 2 < abs_dy) {
        return {start_x, end_y};
    }
    if (abs_dy * 2 < abs_dx) {
        return {end_x, start_y};
    }

    int distance = std::max(abs_dx, abs_dy);
    int snapped_x = start_x + (dx < 0 ? -distance : distance);
    int snapped_y = start_y + (dy < 0 ? -distance : distance);
    return {snapped_x, snapped_y};
}

std::vector<std::pair<int, int>> MyGLCanvas::BuildHeightmapLineCells(int start_x, int start_y, int end_x, int end_y) const {
    std::vector<std::pair<int, int>> cells;
    int dx = std::abs(end_x - start_x);
    int sx = start_x < end_x ? 1 : -1;
    int dy = -std::abs(end_y - start_y);
    int sy = start_y < end_y ? 1 : -1;
    int err = dx + dy;

    while (true) {
        cells.push_back({start_x, start_y});
        if (start_x == end_x && start_y == end_y) {
            break;
        }
        int twice_err = 2 * err;
        if (twice_err >= dy) {
            err += dy;
            start_x += sx;
        }
        if (twice_err <= dx) {
            err += dx;
            start_y += sy;
        }
    }

    return cells;
}

std::vector<std::pair<int, int>> MyGLCanvas::BuildHeightmapRectCells(int start_x, int start_y, int end_x, int end_y, bool filled) const {
    std::vector<std::pair<int, int>> cells;
    int min_x = std::min(start_x, end_x);
    int max_x = std::max(start_x, end_x);
    int min_y = std::min(start_y, end_y);
    int max_y = std::max(start_y, end_y);

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            if (filled || x == min_x || x == max_x || y == min_y || y == max_y) {
                cells.push_back({x, y});
            }
        }
    }
    return cells;
}

std::vector<std::pair<int, int>> MyGLCanvas::BuildHeightmapCircleCells(int start_x, int start_y, int end_x, int end_y, bool filled) const {
    std::vector<std::pair<int, int>> cells;
    int min_x = std::min(start_x, end_x);
    int max_x = std::max(start_x, end_x);
    int min_y = std::min(start_y, end_y);
    int max_y = std::max(start_y, end_y);

    float radius_x = std::max(0.5f, (static_cast<float>(max_x - min_x) + 1.0f) * 0.5f);
    float radius_y = std::max(0.5f, (static_cast<float>(max_y - min_y) + 1.0f) * 0.5f);
    float center_x = (static_cast<float>(min_x) + static_cast<float>(max_x) + 1.0f) * 0.5f;
    float center_y = (static_cast<float>(min_y) + static_cast<float>(max_y) + 1.0f) * 0.5f;

    auto inside = [&](int x, int y) {
        float dx = (static_cast<float>(x) + 0.5f - center_x) / radius_x;
        float dy = (static_cast<float>(y) + 0.5f - center_y) / radius_y;
        return dx * dx + dy * dy <= 1.0f;
    };

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            if (!inside(x, y)) {
                continue;
            }
            bool outline = !inside(x - 1, y) || !inside(x + 1, y) || !inside(x, y - 1) || !inside(x, y + 1);
            if (filled || outline) {
                cells.push_back({x, y});
            }
        }
    }
    return cells;
}

std::vector<std::pair<int, int>> MyGLCanvas::BuildHeightmapFloodFillCells(int x, int y) const {
    std::vector<std::pair<int, int>> cells;
    auto map = CurrentRoomMap();
    if (!map || x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) {
        return cells;
    }

    uint16_t target = map->GetHeightmapCell({x, y});
    std::set<std::pair<int, int>> visited;
    std::deque<std::pair<int, int>> queue;
    queue.push_back({x, y});
    visited.insert({x, y});

    while (!queue.empty()) {
        auto cell = queue.front();
        queue.pop_front();
        cells.push_back(cell);

        static constexpr std::array<std::pair<int, int>, 4> kNeighbors = {{
            {-1, 0}, {1, 0}, {0, -1}, {0, 1}
        }};
        for (const auto& delta : kNeighbors) {
            int nx = cell.first + delta.first;
            int ny = cell.second + delta.second;
            std::pair<int, int> next{nx, ny};
            if (nx < 0 || ny < 0 || nx >= map->GetHeightmapWidth() || ny >= map->GetHeightmapHeight() ||
                visited.find(next) != visited.end() ||
                map->GetHeightmapCell({nx, ny}) != target) {
                continue;
            }
            visited.insert(next);
            queue.push_back(next);
        }
    }

    return cells;
}

std::map<std::pair<int, int>, uint16_t> MyGLCanvas::BuildHeightmapStampCells(int x, int y) const {
    std::map<std::pair<int, int>, uint16_t> cells;
    auto map = CurrentRoomMap();
    int primary_x = PrimaryHeightmapCellX();
    int primary_y = PrimaryHeightmapCellY();
    if (!map || primary_x < 0 || primary_y < 0) {
        return cells;
    }

    int dx = x - primary_x;
    int dy = y - primary_y;
    for (const auto& source : m_heightmap_selected_cells) {
        int source_x = source.first;
        int source_y = source.second;
        int target_x = source_x + dx;
        int target_y = source_y + dy;
        if (source_x < 0 || source_y < 0 ||
            source_x >= map->GetHeightmapWidth() || source_y >= map->GetHeightmapHeight() ||
            target_x < 0 || target_y < 0 ||
            target_x >= map->GetHeightmapWidth() || target_y >= map->GetHeightmapHeight()) {
            continue;
        }
        cells[{target_x, target_y}] = map->GetHeightmapCell({source_x, source_y});
    }

    return cells;
}

void MyGLCanvas::ApplyHeightmapStampAt(int x, int y) {
    auto map = CurrentRoomMap();
    if (!map) {
        return;
    }

    bool changed = false;
    for (const auto& cell : BuildHeightmapStampCells(x, y)) {
        int target_x = cell.first.first;
        int target_y = cell.first.second;
        if (map->GetHeightmapCell({target_x, target_y}) == cell.second) {
            continue;
        }
        if (!changed) {
            CaptureUndoState();
        }
        map->SetHeightmapCell({target_x, target_y}, cell.second);
        if (m_tileswap_preview_map) {
            m_tileswap_preview_map->SetHeightmapCell({target_x, target_y}, cell.second);
        }
        changed = true;
    }

    if (!changed) {
        return;
    }
    m_heightmap_draw_dirty = true;
    CommitHeightmapDrawStroke();
}

void MyGLCanvas::ApplyHeightmapFloodFillAt(int x, int y) {
    if (!m_heightmap_clipboard_valid) {
        return;
    }

    bool changed = false;
    for (const auto& cell : BuildHeightmapFloodFillCells(x, y)) {
        changed = PasteHeightmapCellAt(cell.first, cell.second, true) || changed;
    }
    if (changed) {
        CommitHeightmapDrawStroke();
    }
}

void MyGLCanvas::BeginHeightmapLineDrag(int x, int y, bool shift_down) {
    m_heightmap_dragging_line = true;
    m_heightmap_line_start_x = x;
    m_heightmap_line_start_y = y;
    UpdateHeightmapLineDrag(x, y, shift_down);
}

void MyGLCanvas::UpdateHeightmapLineDrag(int x, int y, bool shift_down) {
    auto map = CurrentRoomMap();
    if (!map || m_heightmap_line_start_x < 0 || m_heightmap_line_start_y < 0) {
        m_heightmap_line_preview_cells.clear();
        return;
    }

    std::pair<int, int> end{x, y};
    if (m_drawing_tool == DrawingTool::Line) {
        end = shift_down ? end : SnapHeightmapLineEnd(m_heightmap_line_start_x, m_heightmap_line_start_y, x, y);
    } else if (shift_down) {
        int dx = x - m_heightmap_line_start_x;
        int dy = y - m_heightmap_line_start_y;
        int size = std::max(std::abs(dx), std::abs(dy));
        int step_x = dx < 0 ? -1 : 1;
        int step_y = dy < 0 ? -1 : 1;
        end = {
            m_heightmap_line_start_x + step_x * size,
            m_heightmap_line_start_y + step_y * size
        };
    }
    auto [end_x, end_y] = end;

    m_heightmap_line_end_x = end_x;
    m_heightmap_line_end_y = end_y;
    switch (m_drawing_tool) {
        case DrawingTool::FilledRect:
            m_heightmap_line_preview_cells = BuildHeightmapRectCells(m_heightmap_line_start_x, m_heightmap_line_start_y, end_x, end_y, true);
            break;
        case DrawingTool::OutlineRect:
            m_heightmap_line_preview_cells = BuildHeightmapRectCells(m_heightmap_line_start_x, m_heightmap_line_start_y, end_x, end_y, false);
            break;
        case DrawingTool::FilledCircle:
            m_heightmap_line_preview_cells = BuildHeightmapCircleCells(m_heightmap_line_start_x, m_heightmap_line_start_y, end_x, end_y, true);
            break;
        case DrawingTool::OutlineCircle:
            m_heightmap_line_preview_cells = BuildHeightmapCircleCells(m_heightmap_line_start_x, m_heightmap_line_start_y, end_x, end_y, false);
            break;
        default:
            m_heightmap_line_preview_cells = BuildHeightmapLineCells(m_heightmap_line_start_x, m_heightmap_line_start_y, end_x, end_y);
            break;
    }
}

void MyGLCanvas::CommitHeightmapLineDrag() {
    if (!m_heightmap_dragging_line || !m_heightmap_clipboard_valid) {
        CancelHeightmapLineDrag();
        return;
    }

    bool changed = false;
    for (const auto& cell : m_heightmap_line_preview_cells) {
        changed = PasteHeightmapCellAt(cell.first, cell.second, true) || changed;
    }
    m_heightmap_dragging_line = false;
    m_heightmap_line_preview_cells.clear();
    m_heightmap_line_start_x = -1;
    m_heightmap_line_start_y = -1;
    m_heightmap_line_end_x = -1;
    m_heightmap_line_end_y = -1;
    if (changed) {
        CommitHeightmapDrawStroke();
    } else {
        m_heightmap_draw_dirty = false;
    }
}

void MyGLCanvas::CancelHeightmapLineDrag() {
    m_heightmap_dragging_line = false;
    m_heightmap_line_preview_cells.clear();
    m_heightmap_line_start_x = -1;
    m_heightmap_line_start_y = -1;
    m_heightmap_line_end_x = -1;
    m_heightmap_line_end_y = -1;
    m_heightmap_draw_dirty = false;
}

void MyGLCanvas::ClampBackgroundSelection() {
    auto map = CurrentRoomMap();
    int width = IsHeightmapEditMode() && map ? map->GetHeightmapWidth() : m_mapRenderer.GetRoomWidth();
    int height = IsHeightmapEditMode() && map ? map->GetHeightmapHeight() : m_mapRenderer.GetRoomHeight();
    if (width <= 0 || height <= 0) {
        m_background_has_selection = false;
        m_background_selected_x = 0;
        m_background_selected_y = 0;
        return;
    }
    m_background_selected_x = std::clamp(m_background_selected_x, 0, width - 1);
    m_background_selected_y = std::clamp(m_background_selected_y, 0, height - 1);
    m_heightmap_selection_anchor_x = std::clamp(m_heightmap_selection_anchor_x, 0, width - 1);
    m_heightmap_selection_anchor_y = std::clamp(m_heightmap_selection_anchor_y, 0, height - 1);
    if (IsHeightmapEditMode() && map) {
        for (auto it = m_heightmap_selected_cells.begin(); it != m_heightmap_selected_cells.end(); ) {
            if (it->first < 0 || it->second < 0 || it->first >= width || it->second >= height) {
                it = m_heightmap_selected_cells.erase(it);
            } else {
                ++it;
            }
        }
        if (m_heightmap_selected_cells.empty()) {
            m_heightmap_selected_cells.insert({m_heightmap_selection_anchor_x, m_heightmap_selection_anchor_y});
        }
    }
    m_background_has_selection = true;
}

void MyGLCanvas::MoveBackgroundSelection(int dx, int dy) {
    if (!m_background_has_selection) {
        m_background_selected_x = 0;
        m_background_selected_y = 0;
        m_heightmap_selection_anchor_x = 0;
        m_heightmap_selection_anchor_y = 0;
        m_background_has_selection = true;
    }
    m_background_selected_x += dx;
    m_background_selected_y += dy;
    ClampBackgroundSelection();
    m_heightmap_selection_anchor_x = m_background_selected_x;
    m_heightmap_selection_anchor_y = m_background_selected_y;
    if (IsHeightmapEditMode()) {
        m_heightmap_selected_cells.clear();
        m_heightmap_selected_cells.insert({m_heightmap_selection_anchor_x, m_heightmap_selection_anchor_y});
        NotifyHeightmapTargetChanged();
    }
    NotifyLayerBlockSelected();
}

std::shared_ptr<Tilemap3D> MyGLCanvas::CurrentRoomMap() const {
    auto rd = m_gd ? m_gd->GetRoomData() : nullptr;
    if (!rd) {
        return nullptr;
    }
    auto map_entry = rd->GetMapForRoom(m_current_room);
    return map_entry ? map_entry->GetData() : nullptr;
}

int MyGLCanvas::SelectedBackgroundBlockIndex() const {
    if (!m_background_has_selection || m_background_selected_x < 0 || m_background_selected_y < 0) {
        return -1;
    }
    if (m_background_selected_x >= m_mapRenderer.GetRoomWidth() || m_background_selected_y >= m_mapRenderer.GetRoomHeight()) {
        return -1;
    }
    return m_background_selected_y * m_mapRenderer.GetRoomWidth() + m_background_selected_x;
}

int MyGLCanvas::SelectedHeightmapCellX() const {
    auto map = CurrentRoomMap();
    if (!map || !m_background_has_selection) {
        return -1;
    }
    if (m_background_selected_x < 0 || m_background_selected_x >= map->GetHeightmapWidth()) {
        return -1;
    }
    return m_background_selected_x;
}

int MyGLCanvas::SelectedHeightmapCellY() const {
    auto map = CurrentRoomMap();
    if (!map || !m_background_has_selection) {
        return -1;
    }
    if (m_background_selected_y < 0 || m_background_selected_y >= map->GetHeightmapHeight()) {
        return -1;
    }
    return m_background_selected_y;
}

int MyGLCanvas::PrimaryHeightmapCellX() const {
    auto map = CurrentRoomMap();
    if (!map || !m_background_has_selection) {
        return -1;
    }
    if (m_heightmap_selection_anchor_x < 0 || m_heightmap_selection_anchor_x >= map->GetHeightmapWidth()) {
        return -1;
    }
    return m_heightmap_selection_anchor_x;
}

int MyGLCanvas::PrimaryHeightmapCellY() const {
    auto map = CurrentRoomMap();
    if (!map || !m_background_has_selection) {
        return -1;
    }
    if (m_heightmap_selection_anchor_y < 0 || m_heightmap_selection_anchor_y >= map->GetHeightmapHeight()) {
        return -1;
    }
    return m_heightmap_selection_anchor_y;
}

namespace {
uint8_t HeightmapCellType(uint16_t value) {
    return static_cast<uint8_t>(value & 0x00FF);
}

uint8_t HeightmapCellHeight(uint16_t value) {
    return static_cast<uint8_t>((value >> 8) & 0x0F);
}

uint8_t HeightmapCellProps(uint16_t value) {
    return static_cast<uint8_t>((value >> 12) & 0x0F);
}

uint16_t WithHeightmapCellType(uint16_t value, uint8_t type) {
    return static_cast<uint16_t>((value & 0xFF00) | type);
}

uint16_t WithHeightmapCellHeight(uint16_t value, uint8_t height) {
    return static_cast<uint16_t>((value & 0xF0FF) | ((height & 0x0F) << 8));
}

uint16_t WithHeightmapCellProps(uint16_t value, uint8_t props) {
    return static_cast<uint16_t>((value & 0x0FFF) | ((props & 0x0F) << 12));
}
}

uint16_t MyGLCanvas::SelectedHeightmapCellValue() const {
    if (IsHeightmapBrushTool() && m_heightmap_clipboard_valid) {
        return m_heightmap_clipboard_cell;
    }
    auto map = CurrentRoomMap();
    int x = PrimaryHeightmapCellX();
    int y = PrimaryHeightmapCellY();
    if (map && x >= 0 && y >= 0) {
        return map->GetHeightmapCell({x, y});
    }
    return 0;
}

bool MyGLCanvas::HasSelectedLayerCell() const {
    return !m_layer_selected_cells.empty() && SelectedBackgroundBlockIndex() >= 0;
}

bool MyGLCanvas::HasSelectedHeightmapCell() const {
    return !m_heightmap_selected_cells.empty() && PrimaryHeightmapCellX() >= 0 && PrimaryHeightmapCellY() >= 0;
}

bool MyGLCanvas::HasHeightmapEditTarget() const {
    return HasSelectedHeightmapCell() || (IsHeightmapBrushTool() && m_heightmap_clipboard_valid);
}

bool MyGLCanvas::CanInsertSelectedHeightmapRow() const {
    auto map = CurrentRoomMap();
    return HasSelectedHeightmapCell() && map && map->GetHeightmapWidth() < 64;
}

bool MyGLCanvas::CanInsertSelectedHeightmapColumn() const {
    auto map = CurrentRoomMap();
    return HasSelectedHeightmapCell() && map && map->GetHeightmapHeight() < 64;
}

bool MyGLCanvas::CanDeleteSelectedHeightmapRow() const {
    auto map = CurrentRoomMap();
    return HasSelectedHeightmapCell() && map && map->GetHeightmapWidth() > 1;
}

bool MyGLCanvas::CanDeleteSelectedHeightmapColumn() const {
    auto map = CurrentRoomMap();
    return HasSelectedHeightmapCell() && map && map->GetHeightmapHeight() > 1;
}

bool MyGLCanvas::CanIncreaseSelectedHeightmapHeight() const {
    if (IsHeightmapBrushTool()) {
        return m_heightmap_clipboard_valid && HeightmapCellHeight(m_heightmap_clipboard_cell) < 15;
    }
    auto map = CurrentRoomMap();
    int x = PrimaryHeightmapCellX();
    int y = PrimaryHeightmapCellY();
    if (map && x >= 0 && y >= 0) {
        return map->GetHeight({x, y}) < 15;
    }
    return false;
}

bool MyGLCanvas::CanDecreaseSelectedHeightmapHeight() const {
    if (IsHeightmapBrushTool()) {
        return m_heightmap_clipboard_valid && HeightmapCellHeight(m_heightmap_clipboard_cell) > 0;
    }
    auto map = CurrentRoomMap();
    int x = PrimaryHeightmapCellX();
    int y = PrimaryHeightmapCellY();
    if (map && x >= 0 && y >= 0) {
        return map->GetHeight({x, y}) > 0;
    }
    return false;
}

bool MyGLCanvas::CanDeleteSelectedTilemapRow() const {
    auto map = CurrentRoomMap();
    return HasSelectedLayerCell() && map && map->GetWidth() > 1;
}

bool MyGLCanvas::CanDeleteSelectedTilemapColumn() const {
    auto map = CurrentRoomMap();
    return HasSelectedLayerCell() && map && map->GetHeight() > 1;
}

uint8_t MyGLCanvas::GetSelectedHeightmapType() const {
    return HeightmapCellType(SelectedHeightmapCellValue());
}

bool MyGLCanvas::IsSelectedHeightmapPlayerPassable() const {
    return (HeightmapCellProps(SelectedHeightmapCellValue()) & 0x04) == 0;
}

bool MyGLCanvas::IsSelectedHeightmapNpcPassable() const {
    return (HeightmapCellProps(SelectedHeightmapCellValue()) & 0x02) == 0;
}

bool MyGLCanvas::IsSelectedHeightmapRaftTrack() const {
    return (HeightmapCellProps(SelectedHeightmapCellValue()) & 0x01) == 0;
}

void MyGLCanvas::SetSelectedHeightmapType(uint8_t type) {
    if (IsHeightmapBrushTool()) {
        if (m_heightmap_clipboard_valid) {
            m_heightmap_clipboard_cell = WithHeightmapCellType(m_heightmap_clipboard_cell, type);
            NotifyHeightmapTargetChanged();
            Refresh();
        }
        return;
    }
    auto map = CurrentRoomMap();
    int x = PrimaryHeightmapCellX();
    int y = PrimaryHeightmapCellY();
    if (!map || x < 0 || y < 0) return;
    CaptureUndoState();
    map->SetCellType({x, y}, type);
    ApplyPrimaryHeightmapTypeToSelection();
    UpdateHeightmapClipboardFromSelectedCell();
    ReloadCurrentRoomMapView();
    NotifyHeightmapChanged(false);
    NotifyHeightmapTargetChanged();
    Refresh();
}

void MyGLCanvas::ToggleSelectedHeightmapPlayerPassable() {
    if (IsHeightmapBrushTool()) {
        if (m_heightmap_clipboard_valid) {
            uint8_t props = HeightmapCellProps(m_heightmap_clipboard_cell);
            m_heightmap_clipboard_cell = WithHeightmapCellProps(m_heightmap_clipboard_cell, IsSelectedHeightmapPlayerPassable() ? (props | 0x04) : (props & ~0x04));
            NotifyHeightmapTargetChanged();
            Refresh();
        }
        return;
    }
    auto map = CurrentRoomMap();
    int x = PrimaryHeightmapCellX();
    int y = PrimaryHeightmapCellY();
    if (!map || x < 0 || y < 0) return;
    CaptureUndoState();
    uint8_t props = map->GetCellProps({x, y});
    map->SetCellProps({x, y}, IsSelectedHeightmapPlayerPassable() ? (props | 0x04) : (props & ~0x04));
    ApplyPrimaryHeightmapPropsToSelection();
    UpdateHeightmapClipboardFromSelectedCell();
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    NotifyHeightmapTargetChanged();
    Refresh();
}

void MyGLCanvas::ToggleSelectedHeightmapNpcPassable() {
    if (IsHeightmapBrushTool()) {
        if (m_heightmap_clipboard_valid) {
            uint8_t props = HeightmapCellProps(m_heightmap_clipboard_cell);
            m_heightmap_clipboard_cell = WithHeightmapCellProps(m_heightmap_clipboard_cell, IsSelectedHeightmapNpcPassable() ? (props | 0x02) : (props & ~0x02));
            NotifyHeightmapTargetChanged();
            Refresh();
        }
        return;
    }
    auto map = CurrentRoomMap();
    int x = PrimaryHeightmapCellX();
    int y = PrimaryHeightmapCellY();
    if (!map || x < 0 || y < 0) return;
    CaptureUndoState();
    uint8_t props = map->GetCellProps({x, y});
    map->SetCellProps({x, y}, IsSelectedHeightmapNpcPassable() ? (props | 0x02) : (props & ~0x02));
    ApplyPrimaryHeightmapPropsToSelection();
    UpdateHeightmapClipboardFromSelectedCell();
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    NotifyHeightmapTargetChanged();
    Refresh();
}

void MyGLCanvas::ToggleSelectedHeightmapRaftTrack() {
    if (IsHeightmapBrushTool()) {
        if (m_heightmap_clipboard_valid) {
            uint8_t props = HeightmapCellProps(m_heightmap_clipboard_cell);
            m_heightmap_clipboard_cell = WithHeightmapCellProps(m_heightmap_clipboard_cell, IsSelectedHeightmapRaftTrack() ? (props | 0x01) : (props & ~0x01));
            NotifyHeightmapTargetChanged();
            Refresh();
        }
        return;
    }
    auto map = CurrentRoomMap();
    int x = PrimaryHeightmapCellX();
    int y = PrimaryHeightmapCellY();
    if (!map || x < 0 || y < 0) return;
    CaptureUndoState();
    uint8_t props = map->GetCellProps({x, y});
    map->SetCellProps({x, y}, IsSelectedHeightmapRaftTrack() ? (props | 0x01) : (props & ~0x01));
    ApplyPrimaryHeightmapPropsToSelection();
    UpdateHeightmapClipboardFromSelectedCell();
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    NotifyHeightmapTargetChanged();
    Refresh();
}

void MyGLCanvas::AdjustSelectedHeightmapHeight(int delta) {
    if (IsHeightmapBrushTool()) {
        if (m_heightmap_clipboard_valid) {
            int height = std::clamp(static_cast<int>(HeightmapCellHeight(m_heightmap_clipboard_cell)) + delta, 0, 15);
            m_heightmap_clipboard_cell = WithHeightmapCellHeight(m_heightmap_clipboard_cell, static_cast<uint8_t>(height));
            NotifyHeightmapTargetChanged();
            Refresh();
        }
        return;
    }
    auto map = CurrentRoomMap();
    int x = PrimaryHeightmapCellX();
    int y = PrimaryHeightmapCellY();
    if (!map || x < 0 || y < 0) return;
    CaptureUndoState();
    int height = std::clamp(static_cast<int>(map->GetHeight({x, y})) + delta, 0, 15);
    map->SetHeight({x, y}, static_cast<uint8_t>(height));
    ApplyPrimaryHeightmapHeightToSelection();
    UpdateHeightmapClipboardFromSelectedCell();
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    NotifyHeightmapTargetChanged();
    Refresh();
}

void MyGLCanvas::ClearSelectedHeightmapCells() {
    if (!HasSelectedHeightmapCell()) {
        return;
    }

    auto map = CurrentRoomMap();
    if (!map) {
        return;
    }

    bool changed = false;
    static constexpr uint16_t kClearedHeightmapCell = 0x4000;
    for (const auto& cell : m_heightmap_selected_cells) {
        int x = cell.first;
        int y = cell.second;
        if (x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight() ||
            map->GetHeightmapCell({x, y}) == kClearedHeightmapCell) {
            continue;
        }
        if (!changed) {
            CaptureUndoState();
        }
        map->SetHeightmapCell({x, y}, kClearedHeightmapCell);
        if (m_tileswap_preview_map) {
            m_tileswap_preview_map->SetHeightmapCell({x, y}, kClearedHeightmapCell);
        }
        changed = true;
    }

    if (!changed) {
        return;
    }
    UpdateHeightmapClipboardFromSelectedCell();
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    NotifyHeightmapTargetChanged();
    Refresh();
}

void MyGLCanvas::ClearSelectedLayerCells() {
    auto map = CurrentRoomMap();
    if (!map || !HasSelectedLayerCell()) {
        return;
    }

    bool changed = false;
    Tilemap3D::Layer layer = CurrentEditLayer();
    const int width = m_mapRenderer.GetRoomWidth();
    const int height = m_mapRenderer.GetRoomHeight();
    for (const auto& cell : m_layer_selected_cells) {
        int x = cell.first;
        int y = cell.second;
        if (x < 0 || y < 0 || x >= width || y >= height) {
            continue;
        }
        int block_index = y * width + x;
        if (block_index < 0 || block_index >= map->GetWidth() * map->GetHeight()) {
            continue;
        }
        if (map->GetBlock(static_cast<uint16_t>(block_index), layer).value == 0) {
            continue;
        }
        if (!changed) {
            CaptureUndoState();
        }
        map->SetBlock(0, static_cast<uint16_t>(block_index), layer);
        if (m_tileswap_preview_map) {
            m_tileswap_preview_map->SetBlock(0, static_cast<uint16_t>(block_index), layer);
        }
        changed = true;
    }

    if (!changed) {
        return;
    }
    ReloadCurrentRoomMapView();
    NotifyLayerBlockSelected();
    Refresh();
}

bool MyGLCanvas::CanNudgeHeightmap(int left_delta, int top_delta) const {
    auto map = CurrentRoomMap();
    if (!map) {
        return false;
    }
    int new_left = static_cast<int>(map->GetLeft()) + left_delta;
    int new_top = static_cast<int>(map->GetTop()) + top_delta;
    return new_left >= 0 && new_left <= 63 && new_top >= 0 && new_top <= 63;
}

void MyGLCanvas::NudgeHeightmap(int left_delta, int top_delta) {
    auto map = CurrentRoomMap();
    if (!map || !CanNudgeHeightmap(left_delta, top_delta)) {
        return;
    }
    CaptureUndoState();
    map->SetLeft(static_cast<uint8_t>(static_cast<int>(map->GetLeft()) + left_delta));
    map->SetTop(static_cast<uint8_t>(static_cast<int>(map->GetTop()) + top_delta));
    ReloadCurrentRoomMapView();
    m_heightmapRenderer.LoadRoom(m_current_room);
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(true);
    Refresh();
}

void MyGLCanvas::InsertSelectedHeightmapRowBefore() {
    auto map = CurrentRoomMap();
    int x = SelectedHeightmapCellX();
    if (!map || x < 0 || map->GetHeightmapWidth() >= 64) {
        return;
    }
    CaptureUndoState();
    map->InsertHeightmapRow(static_cast<uint8_t>(x));
    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    Refresh();
}

void MyGLCanvas::InsertSelectedHeightmapRowAfter() {
    auto map = CurrentRoomMap();
    int x = SelectedHeightmapCellX();
    if (!map || x < 0 || map->GetHeightmapWidth() >= 64) {
        return;
    }
    CaptureUndoState();
    map->InsertHeightmapRow(static_cast<uint8_t>(x));
    ++m_background_selected_x;
    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    Refresh();
}

void MyGLCanvas::DeleteSelectedHeightmapRow() {
    auto map = CurrentRoomMap();
    int x = SelectedHeightmapCellX();
    if (!map || x < 0 || map->GetHeightmapWidth() <= 1) {
        return;
    }
    CaptureUndoState();
    map->DeleteHeightmapRow(static_cast<uint8_t>(x));
    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    Refresh();
}

void MyGLCanvas::InsertSelectedHeightmapColumnBefore() {
    auto map = CurrentRoomMap();
    int y = SelectedHeightmapCellY();
    if (!map || y < 0 || map->GetHeightmapHeight() >= 64) {
        return;
    }
    CaptureUndoState();
    map->InsertHeightmapColumn(static_cast<uint8_t>(y));
    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    Refresh();
}

void MyGLCanvas::InsertSelectedHeightmapColumnAfter() {
    auto map = CurrentRoomMap();
    int y = SelectedHeightmapCellY();
    if (!map || y < 0 || map->GetHeightmapHeight() >= 64) {
        return;
    }
    CaptureUndoState();
    map->InsertHeightmapColumn(static_cast<uint8_t>(y));
    ++m_background_selected_y;
    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    Refresh();
}

void MyGLCanvas::DeleteSelectedHeightmapColumn() {
    auto map = CurrentRoomMap();
    int y = SelectedHeightmapCellY();
    if (!map || y < 0 || map->GetHeightmapHeight() <= 1) {
        return;
    }
    CaptureUndoState();
    map->DeleteHeightmapColumn(static_cast<uint8_t>(y));
    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    Refresh();
}

uint16_t MyGLCanvas::SelectedBackgroundBlockId() const {
    auto map = CurrentRoomMap();
    int block_index = SelectedBackgroundBlockIndex();
    if (!map || block_index < 0) {
        return 0;
    }
    if (block_index >= map->GetWidth() * map->GetHeight()) {
        return 0;
    }
    return static_cast<uint16_t>(map->GetBlock(static_cast<uint16_t>(block_index), CurrentEditLayer()).value & 0x03FF);
}

void MyGLCanvas::CopySelectedBackgroundBlock() {
    int block_index = SelectedBackgroundBlockIndex();
    auto map = CurrentRoomMap();
    if (!map || block_index < 0) {
        return;
    }
    if (block_index >= map->GetWidth() * map->GetHeight()) {
        return;
    }
    m_background_clipboard_block_id = static_cast<uint16_t>(map->GetBlock(static_cast<uint16_t>(block_index), CurrentEditLayer()).value & 0x03FF);
    m_background_clipboard_valid = true;
    NotifyLayerBlockSelected();
}

void MyGLCanvas::CopyBackgroundBlockAt(int x, int y) {
    auto map = CurrentRoomMap();
    if (!map || x < 0 || y < 0 || x >= m_mapRenderer.GetRoomWidth() || y >= m_mapRenderer.GetRoomHeight()) {
        return;
    }
    int block_index = y * m_mapRenderer.GetRoomWidth() + x;
    if (block_index < 0 || block_index >= map->GetWidth() * map->GetHeight()) {
        return;
    }
    m_background_clipboard_block_id = static_cast<uint16_t>(map->GetBlock(static_cast<uint16_t>(block_index), CurrentEditLayer()).value & 0x03FF);
    m_background_clipboard_valid = true;
    PostLayerBlockSelection(EventTarget(), this, static_cast<int>(m_background_clipboard_block_id));
}

void MyGLCanvas::SetSelectedBlockId(int block) {
    if (block < 0) {
        m_background_clipboard_valid = false;
        m_background_clipboard_block_id = 0;
        Refresh();
        return;
    }
    m_background_clipboard_block_id = static_cast<uint16_t>(block & 0x03FF);
    m_background_clipboard_valid = true;
    if (IsLayerEditMode()) {
        PostLayerBlockSelection(EventTarget(), this, static_cast<int>(m_background_clipboard_block_id));
    }
    Refresh();
}

void MyGLCanvas::AdjustSelectedBlockId(int delta) {
    int block = m_background_clipboard_valid ? static_cast<int>(m_background_clipboard_block_id) : static_cast<int>(SelectedBackgroundBlockId());
    SetSelectedBlockId((block + delta) & 0x03FF);
}

void MyGLCanvas::CopySelectedHeightmapCell() {
    uint16_t value = SelectedHeightmapCellValue();
    if (!m_background_has_selection) {
        return;
    }
    m_heightmap_clipboard_cell = value;
    m_heightmap_clipboard_valid = true;
    NotifyHeightmapTargetChanged();
}

void MyGLCanvas::SetSelectedHeightmapCell(uint16_t value, bool refresh_object_placements) {
    if (IsHeightmapBrushTool()) {
        m_heightmap_clipboard_cell = value;
        m_heightmap_clipboard_valid = true;
        NotifyHeightmapTargetChanged();
        Refresh();
        return;
    }
    if (!HasSelectedHeightmapCell()) {
        return;
    }

    auto map = CurrentRoomMap();
    if (!map) {
        return;
    }

    bool changed = false;
    for (const auto& cell : m_heightmap_selected_cells) {
        int x = cell.first;
        int y = cell.second;
        if (x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) {
            continue;
        }
        if (map->GetHeightmapCell({x, y}) == value) {
            continue;
        }
        if (!changed) {
            CaptureUndoState();
        }
        map->SetHeightmapCell({x, y}, value);
        if (m_tileswap_preview_map) {
            m_tileswap_preview_map->SetHeightmapCell({x, y}, value);
        }
        changed = true;
    }

    if (!changed) {
        return;
    }
    UpdateHeightmapClipboardFromSelectedCell();
    ReloadCurrentRoomMapView();
    if (refresh_object_placements) {
        RefreshObjectPlacementsFromHeightmap();
    }
    NotifyHeightmapChanged(false);
    NotifyHeightmapTargetChanged();
    Refresh();
}

void MyGLCanvas::ApplyPrimaryHeightmapTypeToSelection() {
    auto map = CurrentRoomMap();
    int primary_x = PrimaryHeightmapCellX();
    int primary_y = PrimaryHeightmapCellY();
    if (!map || primary_x < 0 || primary_y < 0) return;

    uint8_t type = map->GetCellType({primary_x, primary_y});
    for (const auto& cell : m_heightmap_selected_cells) {
        int x = cell.first;
        int y = cell.second;
        if (x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) continue;
        map->SetCellType({x, y}, type);
        if (m_tileswap_preview_map) {
            m_tileswap_preview_map->SetHeightmapCell({x, y}, map->GetHeightmapCell({x, y}));
        }
    }
}

void MyGLCanvas::ApplyPrimaryHeightmapPropsToSelection() {
    auto map = CurrentRoomMap();
    int primary_x = PrimaryHeightmapCellX();
    int primary_y = PrimaryHeightmapCellY();
    if (!map || primary_x < 0 || primary_y < 0) return;

    uint8_t props = map->GetCellProps({primary_x, primary_y});
    for (const auto& cell : m_heightmap_selected_cells) {
        int x = cell.first;
        int y = cell.second;
        if (x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) continue;
        map->SetCellProps({x, y}, props);
        if (m_tileswap_preview_map) {
            m_tileswap_preview_map->SetHeightmapCell({x, y}, map->GetHeightmapCell({x, y}));
        }
    }
}

void MyGLCanvas::ApplyPrimaryHeightmapHeightToSelection() {
    auto map = CurrentRoomMap();
    int primary_x = PrimaryHeightmapCellX();
    int primary_y = PrimaryHeightmapCellY();
    if (!map || primary_x < 0 || primary_y < 0) return;

    uint8_t height = map->GetHeight({primary_x, primary_y});
    for (const auto& cell : m_heightmap_selected_cells) {
        int x = cell.first;
        int y = cell.second;
        if (x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) continue;
        map->SetHeight({x, y}, height);
        if (m_tileswap_preview_map) {
            m_tileswap_preview_map->SetHeightmapCell({x, y}, map->GetHeightmapCell({x, y}));
        }
    }
}

void MyGLCanvas::CopyHeightmapCellAt(int x, int y) {
    auto map = CurrentRoomMap();
    if (!map || x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) {
        return;
    }
    m_heightmap_clipboard_cell = map->GetHeightmapCell({x, y});
    m_heightmap_clipboard_valid = true;
    NotifyHeightmapTargetChanged();
}

void MyGLCanvas::UpdateHeightmapClipboardFromSelectedCell() {
    if (!HasSelectedHeightmapCell()) {
        return;
    }
    m_heightmap_clipboard_cell = SelectedHeightmapCellValue();
    m_heightmap_clipboard_valid = true;
    NotifyHeightmapTargetChanged();
}

void MyGLCanvas::ReloadCurrentRoomMapView() {
    if (m_tileswap_preview_map) {
        m_mapRenderer.LoadPreviewRoom(m_current_room, *m_tileswap_preview_map);
    } else {
        m_mapRenderer.LoadRoom(m_current_room);
    }
}

void MyGLCanvas::PasteSelectedBackgroundBlock() {
    PasteBackgroundBlockAt(m_background_selected_x, m_background_selected_y);
}

std::vector<std::pair<int, int>> MyGLCanvas::BuildLayerFloodFillCells(int x, int y) const {
    std::vector<std::pair<int, int>> cells;
    auto map = CurrentRoomMap();
    const int width = m_mapRenderer.GetRoomWidth();
    const int height = m_mapRenderer.GetRoomHeight();
    if (!map || x < 0 || y < 0 || x >= width || y >= height) {
        return cells;
    }

    int start_index = y * width + x;
    if (start_index < 0 || start_index >= map->GetWidth() * map->GetHeight()) {
        return cells;
    }

    Tilemap3D::Layer layer = CurrentEditLayer();
    uint16_t target = map->GetBlock(static_cast<uint16_t>(start_index), layer).value;
    std::set<std::pair<int, int>> visited;
    std::deque<std::pair<int, int>> queue;
    queue.push_back({x, y});
    visited.insert({x, y});

    while (!queue.empty()) {
        auto cell = queue.front();
        queue.pop_front();
        cells.push_back(cell);

        static constexpr std::array<std::pair<int, int>, 6> kNeighbors = {{
            {1, 0}, {0, 1}, {-1, 0}, {0, -1}, {1, 1}, {-1, -1}
        }};
        for (const auto& delta : kNeighbors) {
            int nx = cell.first + delta.first;
            int ny = cell.second + delta.second;
            std::pair<int, int> next{nx, ny};
            int block_index = ny * width + nx;
            if (nx < 0 || ny < 0 || nx >= width || ny >= height ||
                block_index < 0 || block_index >= map->GetWidth() * map->GetHeight() ||
                visited.find(next) != visited.end() ||
                map->GetBlock(static_cast<uint16_t>(block_index), layer).value != target) {
                continue;
            }
            visited.insert(next);
            queue.push_back(next);
        }
    }

    return cells;
}

void MyGLCanvas::ApplyLayerFloodFillAt(int x, int y) {
    if (!m_background_clipboard_valid) {
        return;
    }

    bool changed = false;
    for (const auto& cell : BuildLayerFloodFillCells(x, y)) {
        changed = PasteBackgroundBlockAt(cell.first, cell.second, true) || changed;
    }
    if (changed) {
        CommitLayerDrawStroke();
    }
}

std::map<std::pair<int, int>, uint16_t> MyGLCanvas::BuildLayerStampCells(int x, int y) const {
    std::map<std::pair<int, int>, uint16_t> cells;
    auto map = CurrentRoomMap();
    if (!map || m_layer_selected_cells.empty() || m_layer_selection_anchor_x < 0 || m_layer_selection_anchor_y < 0) {
        return cells;
    }

    const int width = m_mapRenderer.GetRoomWidth();
    const int height = m_mapRenderer.GetRoomHeight();
    const int dx = x - m_layer_selection_anchor_x;
    const int dy = y - m_layer_selection_anchor_y;
    Tilemap3D::Layer layer = CurrentEditLayer();
    for (const auto& source : m_layer_selected_cells) {
        int source_x = source.first;
        int source_y = source.second;
        int target_x = source_x + dx;
        int target_y = source_y + dy;
        if (source_x < 0 || source_y < 0 || source_x >= width || source_y >= height ||
            target_x < 0 || target_y < 0 || target_x >= width || target_y >= height) {
            continue;
        }

        int source_index = source_y * width + source_x;
        if (source_index < 0 || source_index >= map->GetWidth() * map->GetHeight()) {
            continue;
        }
        cells[{target_x, target_y}] = map->GetBlock(static_cast<uint16_t>(source_index), layer).value;
    }

    return cells;
}

void MyGLCanvas::ApplyLayerStampAt(int x, int y) {
    auto map = CurrentRoomMap();
    if (!map) {
        return;
    }

    bool changed = false;
    const int width = m_mapRenderer.GetRoomWidth();
    Tilemap3D::Layer layer = CurrentEditLayer();
    for (const auto& cell : BuildLayerStampCells(x, y)) {
        int target_x = cell.first.first;
        int target_y = cell.first.second;
        int target_index = target_y * width + target_x;
        if (target_index < 0 || target_index >= map->GetWidth() * map->GetHeight()) {
            continue;
        }
        if (map->GetBlock(static_cast<uint16_t>(target_index), layer).value == cell.second) {
            continue;
        }
        if (!m_layer_draw_dirty) {
            CaptureUndoState();
        }
        map->SetBlock(cell.second, static_cast<uint16_t>(target_index), layer);
        if (m_tileswap_preview_map) {
            m_tileswap_preview_map->SetBlock(cell.second, static_cast<uint16_t>(target_index), layer);
        }
        m_layer_draw_dirty = true;
        changed = true;
    }

    if (changed) {
        ReloadCurrentRoomMapView();
    }
}

bool MyGLCanvas::PasteBackgroundBlockAt(int x, int y, bool defer_updates) {
    auto map = CurrentRoomMap();
    if (!m_background_clipboard_valid || !map || x < 0 || y < 0 ||
        x >= m_mapRenderer.GetRoomWidth() || y >= m_mapRenderer.GetRoomHeight()) {
        return false;
    }

    int block_index = y * m_mapRenderer.GetRoomWidth() + x;
    if (block_index < 0 || block_index >= map->GetWidth() * map->GetHeight()) {
        return false;
    }

    Tilemap3D::Layer layer = CurrentEditLayer();
    if (map->GetBlock(static_cast<uint16_t>(block_index), layer).value == m_background_clipboard_block_id) {
        return false;
    }
    if (!defer_updates || !m_layer_draw_dirty) {
        CaptureUndoState();
    }
    map->SetBlock(m_background_clipboard_block_id, static_cast<uint16_t>(block_index), layer);
    if (m_tileswap_preview_map) {
        m_tileswap_preview_map->SetBlock(m_background_clipboard_block_id, static_cast<uint16_t>(block_index), layer);
    }
    if (defer_updates) {
        m_layer_draw_dirty = true;
        ReloadCurrentRoomMapView();
        return true;
    }
    ReloadCurrentRoomMapView();
    return true;
}

void MyGLCanvas::CommitLayerDrawStroke() {
    if (!m_layer_draw_dirty) {
        return;
    }
    m_layer_draw_dirty = false;
    ReloadCurrentRoomMapView();
}

void MyGLCanvas::PasteSelectedHeightmapCell() {
    auto map = CurrentRoomMap();
    int x = SelectedHeightmapCellX();
    int y = SelectedHeightmapCellY();
    if (!map || x < 0 || y < 0) {
        return;
    }
    PasteHeightmapCellAt(x, y);
}

bool MyGLCanvas::PasteHeightmapCellAt(int x, int y, bool defer_updates) {
    auto map = CurrentRoomMap();
    if (!m_heightmap_clipboard_valid || !map || x < 0 || y < 0 ||
        x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) {
        return false;
    }
    if (map->GetHeightmapCell({x, y}) == m_heightmap_clipboard_cell) {
        return false;
    }
    if (!defer_updates || !m_heightmap_draw_dirty) {
        CaptureUndoState();
    }
    map->SetHeightmapCell({x, y}, m_heightmap_clipboard_cell);
    if (m_tileswap_preview_map) {
        m_tileswap_preview_map->SetHeightmapCell({x, y}, m_heightmap_clipboard_cell);
    }
    if (defer_updates) {
        m_heightmap_draw_dirty = true;
        return true;
    }
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    NotifyHeightmapTargetChanged();
    return true;
}

void MyGLCanvas::CommitHeightmapDrawStroke() {
    if (!m_heightmap_draw_dirty) {
        return;
    }
    m_heightmap_draw_dirty = false;
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    NotifyHeightmapTargetChanged();
}

void MyGLCanvas::ClearCurrentTilemap() {
    auto map = CurrentRoomMap();
    if (!map) {
        return;
    }
    CaptureUndoState();
    map->ClearTilemap();
    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    Refresh();
}

void MyGLCanvas::InsertSelectedTilemapRowBefore() {
    auto map = CurrentRoomMap();
    if (!map || !HasSelectedLayerCell() || map->GetWidth() >= 64) {
        return;
    }
    CaptureUndoState();
    map->InsertTilemapRow(m_background_selected_x);
    ++m_background_selected_x;
    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    Refresh();
}

void MyGLCanvas::InsertSelectedTilemapRowAfter() {
    auto map = CurrentRoomMap();
    if (!map || !HasSelectedLayerCell() || map->GetWidth() >= 64) {
        return;
    }
    CaptureUndoState();
    map->InsertTilemapRow(m_background_selected_x + 1);
    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    Refresh();
}

void MyGLCanvas::DeleteSelectedTilemapRow() {
    auto map = CurrentRoomMap();
    if (!map || !CanDeleteSelectedTilemapRow()) {
        return;
    }
    CaptureUndoState();
    map->DeleteTilemapRow(m_background_selected_x);
    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    Refresh();
}

void MyGLCanvas::InsertSelectedTilemapColumnBefore() {
    auto map = CurrentRoomMap();
    if (!map || !HasSelectedLayerCell() || map->GetHeight() >= 64) {
        return;
    }
    CaptureUndoState();
    map->InsertTilemapColumn(m_background_selected_y);
    ++m_background_selected_y;
    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    Refresh();
}

void MyGLCanvas::InsertSelectedTilemapColumnAfter() {
    auto map = CurrentRoomMap();
    if (!map || !HasSelectedLayerCell() || map->GetHeight() >= 64) {
        return;
    }
    CaptureUndoState();
    map->InsertTilemapColumn(m_background_selected_y + 1);
    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    Refresh();
}

void MyGLCanvas::DeleteSelectedTilemapColumn() {
    auto map = CurrentRoomMap();
    if (!map || !CanDeleteSelectedTilemapColumn()) {
        return;
    }
    CaptureUndoState();
    map->DeleteTilemapColumn(m_background_selected_y);
    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    Refresh();
}
