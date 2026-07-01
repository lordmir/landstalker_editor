#include "GLCanvasWarpEditor.h"
#include "GLLoader.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

#include "GLCanvasObjectSupport.h"
#include "PixelFont.h"

namespace {

struct PickPoint {
	float x;
	float y;
};

struct PickRect {
	float min_x;
	float min_y;
	float max_x;
	float max_y;
};

PickPoint ProjectWarpGridPoint(const WarpInstance& warp, float x, float y, float z)
{
	float grid_x = x - warp.room_left;
	float grid_y = y - warp.room_top;
	return {
		32.0f * grid_x - 32.0f * grid_y + 512.0f,
		16.0f * grid_x + 16.0f * grid_y + 100.0f - warp.z_extent * z
	};
}

std::array<PickPoint, 4> WarpQuad(const WarpInstance& warp, float z_offset = 0.0f)
{
	float x0 = warp.x;
	float y0 = warp.y;
	float x1 = warp.x + warp.width;
	float y1 = warp.y + warp.height;
	float z = warp.floor_z + z_offset;
	return {
		ProjectWarpGridPoint(warp, x0, y0, z),
		ProjectWarpGridPoint(warp, x1, y0, z),
		ProjectWarpGridPoint(warp, x1, y1, z),
		ProjectWarpGridPoint(warp, x0, y1, z)
	};
}

PickRect RectAroundPoint(const PickPoint& point, float half_size)
{
	return {
		point.x - half_size,
		point.y - half_size,
		point.x + half_size,
		point.y + half_size
	};
}

PickRect WarpResizeControlRect(const WarpInstance& warp, int axis)
{
	float z = warp.floor_z;
	PickPoint point = axis == 1
		? ProjectWarpGridPoint(warp, warp.x + warp.width, warp.y + warp.height * 0.5f, z)
		: ProjectWarpGridPoint(warp, warp.x + warp.width * 0.5f, warp.y + warp.height, z);
	return RectAroundPoint(point, 6.0f);
}

bool WarpResizeAxisUsable(const WarpInstance& warp, int axis)
{
	if (axis == 1) {
		return std::round(warp.height) <= 1.0f;
	}
	if (axis == 2) {
		return std::round(warp.width) <= 1.0f;
	}
	return false;
}

std::string HexWord(uint16_t value)
{
	constexpr char digits[] = "0123456789ABCDEF";
	std::string out;
	out.push_back(digits[(value >> 12) & 0x0F]);
	out.push_back(digits[(value >> 8) & 0x0F]);
	out.push_back(digits[(value >> 4) & 0x0F]);
	out.push_back(digits[value & 0x0F]);
	return out;
}

void DrawOverlayGlyph(char c, float x, float y, float scale)
{
	const auto* glyph = PixelFont::Glyph(c);
	if (!glyph) {
		return;
	}

	glBegin(GL_QUADS);
	for (int row = 0; row < PixelFont::kGlyphHeight; ++row) {
		uint8_t bits = (*glyph)[row];
		for (int col = 0; col < PixelFont::kGlyphWidth; ++col) {
			uint8_t mask = static_cast<uint8_t>(1u << (PixelFont::kGlyphWidth - 1 - col));
			if ((bits & mask) == 0) {
				continue;
			}

			float px = x + float(col) * scale;
			float py = y + float(row) * scale;
			glVertex2f(px, py);
			glVertex2f(px + scale, py);
			glVertex2f(px + scale, py + scale);
			glVertex2f(px, py + scale);
		}
	}
	glEnd();
}

void DrawOverlayText(const std::string& text, float x, float y, float scale)
{
	constexpr float glyph_advance = 6.0f;
	x = std::round(x);
	y = std::round(y);
	for (std::size_t i = 0; i < text.size(); ++i) {
		DrawOverlayGlyph(text[i], x + float(i) * glyph_advance * scale, y, scale);
	}
}

}  // namespace

GLCanvasWarpEditor::GLCanvasWarpEditor(MyGLCanvas& canvas)
	: m_canvas(canvas)
{
}

void GLCanvasWarpEditor::AddWarpHalf()
{
	// Warp creation is two-step: place one endpoint now, then complete on next insert.
	Landstalker::WarpList::Warp warp;
	// Use room-grid coords (m_pending_add_hover_x/y from ScreenToMapPoint) to match
	// ProjectWarpGridPoint, which has no heightmap offset. Centre on the cursor.
	float half_w = std::round(m_canvas.m_pending_add_warp_width) * 0.5f;
	float half_h = std::round(m_canvas.m_pending_add_warp_height) * 0.5f;
	float cursor_x, cursor_y;
	if (m_canvas.m_pending_add_hover_x >= 0 && m_canvas.m_pending_add_hover_y >= 0) {
		cursor_x = static_cast<float>(m_canvas.m_pending_add_hover_x) + 0.5f;
		cursor_y = static_cast<float>(m_canvas.m_pending_add_hover_y) + 0.5f;
	} else {
		auto [hx, hy] = m_canvas.MouseHeightmapCell();
		cursor_x = static_cast<float>(hx) + 12.5f;
		cursor_y = static_cast<float>(hy) + 12.5f;
	}
	auto [x, y] = FindNearestFreeWarpCell(cursor_x - half_w, cursor_y - half_h);
	bool update_pending_instance = false;
	if (!m_canvas.m_pending_warp_half) {
		warp.room1 = m_canvas.m_current_room;
		warp.x1 = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(x)), 0, 63));
		warp.y1 = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(y)), 0, 63));
		warp.room2 = 0xFFFF;
		warp.x_size = static_cast<uint8_t>(std::clamp(static_cast<int>(std::round(m_canvas.m_pending_add_warp_width)), 1, 3));
		warp.y_size = static_cast<uint8_t>(std::clamp(static_cast<int>(std::round(m_canvas.m_pending_add_warp_height)), 1, 3));
		warp.type = m_canvas.m_pending_add_warp_type;
		m_canvas.m_pending_warp = warp;
		m_canvas.m_pending_warp_half = true;
		m_canvas.m_pending_warp_room = m_canvas.m_current_room;
		m_canvas.m_pending_warp_instance_id = static_cast<uint32_t>(m_canvas.m_warps.size() + 1);
	} else {
		if (m_canvas.m_pending_warp_room == m_canvas.m_current_room && m_canvas.m_pending_warp_instance_id != 0) {
			int pending_idx = m_canvas.FindWarpIndex(m_canvas.m_pending_warp_instance_id);
			if (pending_idx >= 0) {
				const WarpInstance& pending = m_canvas.m_warps[static_cast<std::size_t>(pending_idx)];
				m_canvas.m_pending_warp.x1 = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(pending.x)), 0, 63));
				m_canvas.m_pending_warp.y1 = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(pending.y)), 0, 63));
				m_canvas.m_pending_warp.x_size = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(pending.width)), 1, 63));
				m_canvas.m_pending_warp.y_size = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(pending.height)), 1, 63));
				m_canvas.m_pending_warp.type = pending.warp.type;
			}
		}
		warp = m_canvas.m_pending_warp;
		warp.room2 = m_canvas.m_current_room;
		warp.x2 = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(x)), 0, 63));
		warp.y2 = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(y)), 0, 63));
		update_pending_instance = m_canvas.m_pending_warp_room == m_canvas.m_current_room;
		m_canvas.m_pending_warp_half = false;
		m_canvas.m_pending_warp_room = 0xFFFF;
	}

	if (update_pending_instance) {
		int pending_idx = m_canvas.FindWarpIndex(m_canvas.m_pending_warp_instance_id);
		if (pending_idx < 0) {
			for (std::size_t i = 0; i < m_canvas.m_warps.size(); ++i) {
				const WarpInstance& candidate = m_canvas.m_warps[i];
				if (candidate.current_room_is_room1 &&
					candidate.warp.room1 == warp.room1 &&
					candidate.warp.room2 == 0xFFFF &&
					candidate.warp.x1 == warp.x1 &&
					candidate.warp.y1 == warp.y1) {
					pending_idx = static_cast<int>(i);
					break;
				}
			}
		}
		uint32_t key = m_canvas.m_pending_warp_instance_id;
		if (pending_idx >= 0) {
			key = m_canvas.m_warps[static_cast<std::size_t>(pending_idx)].warp_key;
			WarpInstance first_inst = GLCanvasObjectSupport::MakeWarpInstance(
				warp,
				m_canvas.m_current_room,
				m_canvas.m_warps[static_cast<std::size_t>(pending_idx)].instance_id,
				float(m_canvas.m_mapRenderer.GetRoomLeft()),
				float(m_canvas.m_mapRenderer.GetRoomTop()),
				m_canvas.m_heightmapRenderer.GetZExtent(),
				key,
				1);
			m_canvas.UpdateWarpFloor(first_inst);
			m_canvas.m_warps[static_cast<std::size_t>(pending_idx)] = first_inst;
		} else {
			WarpInstance first_inst = GLCanvasObjectSupport::MakeWarpInstance(
				warp,
				m_canvas.m_current_room,
				static_cast<uint32_t>(m_canvas.m_warps.size() + 1),
				float(m_canvas.m_mapRenderer.GetRoomLeft()),
				float(m_canvas.m_mapRenderer.GetRoomTop()),
				m_canvas.m_heightmapRenderer.GetZExtent(),
				key,
				1);
			m_canvas.UpdateWarpFloor(first_inst);
			m_canvas.m_warps.push_back(first_inst);
		}
		WarpInstance second_inst = GLCanvasObjectSupport::MakeWarpInstance(
			warp,
			m_canvas.m_current_room,
			static_cast<uint32_t>(m_canvas.m_warps.size() + 1),
			float(m_canvas.m_mapRenderer.GetRoomLeft()),
			float(m_canvas.m_mapRenderer.GetRoomTop()),
			m_canvas.m_heightmapRenderer.GetZExtent(),
			key,
			2);
		m_canvas.UpdateWarpFloor(second_inst);
		m_canvas.m_warps.push_back(second_inst);
		m_canvas.m_selected_warp_idx = static_cast<int>(m_canvas.m_warps.size()) - 1;
	} else {
		WarpInstance inst = GLCanvasObjectSupport::MakeWarpInstance(
			warp,
			m_canvas.m_current_room,
			static_cast<uint32_t>(m_canvas.m_warps.size() + 1),
			float(m_canvas.m_mapRenderer.GetRoomLeft()),
			float(m_canvas.m_mapRenderer.GetRoomTop()),
			m_canvas.m_heightmapRenderer.GetZExtent());
		m_canvas.UpdateWarpFloor(inst);
		m_canvas.m_warps.push_back(inst);
		m_canvas.m_selected_warp_idx = static_cast<int>(m_canvas.m_warps.size()) - 1;
	}
	if (warp.IsValid()) {
		for (std::size_t i = 0; i < m_canvas.m_warps.size(); ) {
			const WarpInstance& candidate = m_canvas.m_warps[i];
			if (static_cast<int>(i) != m_canvas.m_selected_warp_idx &&
				candidate.warp.room1 == warp.room1 &&
				candidate.warp.room2 == 0xFFFF &&
				candidate.warp.x1 == warp.x1 &&
				candidate.warp.y1 == warp.y1) {
				m_canvas.m_warps.erase(m_canvas.m_warps.begin() + static_cast<std::ptrdiff_t>(i));
				if (m_canvas.m_selected_warp_idx > static_cast<int>(i)) {
					--m_canvas.m_selected_warp_idx;
				}
				continue;
			}
			++i;
		}
		for (std::size_t i = 0; i < m_canvas.m_warps.size(); ++i) {
			m_canvas.m_warps[i].instance_id = static_cast<uint32_t>(i + 1);
		}
	}
	if (!m_canvas.m_pending_warp_half) {
		m_canvas.m_pending_warp_instance_id = 0;
		if (warp.IsValid()) {
			m_canvas.PersistCurrentRoomEdits();
		}
	}
	m_canvas.m_selected_entity_idx = -1;
}

std::pair<float, float> GLCanvasWarpEditor::FindNearestFreeWarpCell(float preferred_x, float preferred_y) const
{
	int start_x = std::clamp(static_cast<int>(std::round(preferred_x)), 0, 63);
	int start_y = std::clamp(static_cast<int>(std::round(preferred_y)), 0, 63);

	auto cell_free = [this](int x, int y) {
		float fx = static_cast<float>(x);
		float fy = static_cast<float>(y);
		for (const auto& warp : m_canvas.m_warps) {
			if (fx < warp.x + warp.width &&
				fx + 1.0f > warp.x &&
				fy < warp.y + warp.height &&
				fy + 1.0f > warp.y) {
				return false;
			}
		}
		return true;
	};

	int best_x = start_x;
	int best_y = start_y;
	int best_dist = std::numeric_limits<int>::max();
	for (int y = 0; y <= 63; ++y) {
		for (int x = 0; x <= 63; ++x) {
			if (!cell_free(x, y)) {
				continue;
			}
			int dx = x - start_x;
			int dy = y - start_y;
			int dist = dx * dx + dy * dy;
			if (dist < best_dist) {
				best_dist = dist;
				best_x = x;
				best_y = y;
			}
		}
	}

	return {static_cast<float>(best_x), static_cast<float>(best_y)};
}

void GLCanvasWarpEditor::ResizeSelectedWarp(float dx, float dy)
{
	if (m_canvas.m_selected_warp_idx < 0 || m_canvas.m_selected_warp_idx >= static_cast<int>(m_canvas.m_warps.size())) {
		return;
	}
	WarpInstance& warp = m_canvas.m_warps[static_cast<std::size_t>(m_canvas.m_selected_warp_idx)];
	if (dx != 0.0f) {
		warp.height = GLCanvasObjectSupport::ValidWarpHeight(warp.height, warp.width);
		warp.width = GLCanvasObjectSupport::ValidWarpWidth(warp.width + dx, warp.height);
	}
	if (dy != 0.0f) {
		warp.width = GLCanvasObjectSupport::ValidWarpWidth(warp.width, warp.height);
		warp.height = GLCanvasObjectSupport::ValidWarpHeight(warp.height + dy, warp.width);
	}
	GLCanvasObjectSupport::ClampWarpToValidSize(warp);
	m_canvas.UpdateWarpFloor(warp);
	if (warp.warp_key != 0) {
		for (auto& other : m_canvas.m_warps) {
			if (other.instance_id != warp.instance_id && other.warp_key == warp.warp_key) {
				other.width = warp.width;
				other.height = warp.height;
				m_canvas.UpdateWarpFloor(other);
			}
		}
	}
}

void GLCanvasWarpEditor::RotateSelectedWarp(float dx, float dy)
{
	if (m_canvas.m_selected_warp_idx < 0 || m_canvas.m_selected_warp_idx >= static_cast<int>(m_canvas.m_warps.size())) {
		return;
	}
	WarpInstance& warp = m_canvas.m_warps[static_cast<std::size_t>(m_canvas.m_selected_warp_idx)];
	warp.x = std::clamp(warp.x + dx, 0.0f, 63.5f);
	warp.y = std::clamp(warp.y + dy, 0.0f, 63.5f);
	m_canvas.UpdateWarpFloor(warp);
}

void GLCanvasWarpEditor::CycleSelectedWarpType(int delta)
{
	if (m_canvas.m_selected_warp_idx < 0 || m_canvas.m_selected_warp_idx >= static_cast<int>(m_canvas.m_warps.size())) {
		return;
	}
	WarpInstance& warp = m_canvas.m_warps[static_cast<std::size_t>(m_canvas.m_selected_warp_idx)];
	int type = static_cast<int>(warp.warp.type);
	type = (type + delta + 3) % 3;
	warp.warp.type = static_cast<Landstalker::WarpList::Warp::Type>(type);
	if (warp.warp_key != 0) {
		for (auto& other : m_canvas.m_warps) {
			if (other.instance_id != warp.instance_id && other.warp_key == warp.warp_key) {
				other.warp.type = warp.warp.type;
			}
		}
	}
}

void GLCanvasWarpEditor::RenderPendingWarpGhost()
{
	WarpInstance ghost{};
	if (!m_canvas.BuildPendingWarpPreviewInstance(ghost)) {
		return;
	}
	glUseProgram(0);
	for (int i = 0; i <= 5; ++i) {
		glActiveTexture(GL_TEXTURE0 + i);
		glDisable(GL_TEXTURE_2D);
	}
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	auto quad = WarpQuad(ghost);
	glColor4f(1.0f, 0.95f, 0.0f, 0.35f);
	glBegin(GL_QUADS);
	for (const auto& point : quad) {
		glVertex2f(point.x, point.y);
	}
	glEnd();
	glColor4f(1.0f, 0.95f, 0.0f, 0.9f);
	glLineWidth(2.5f);
	glBegin(GL_LINE_LOOP);
	for (const auto& point : quad) {
		glVertex2f(point.x, point.y);
	}
	glEnd();
	glLineWidth(1.0f);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void GLCanvasWarpEditor::RenderWarps()
{
	glUseProgram(0);
	for (int i = 0; i <= 5; ++i) {
		glActiveTexture(GL_TEXTURE0 + i);
		glDisable(GL_TEXTURE_2D);
	}
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	for (std::size_t i = 0; i < m_canvas.m_warps.size(); ++i) {
		const WarpInstance& warp = m_canvas.m_warps[i];
		auto quad = WarpQuad(warp);
		if (static_cast<int>(i) == m_canvas.m_selected_warp_idx) {
			auto selected_quad = WarpQuad(warp, -0.08f);
			glLineWidth(3.0f);
			glColor4f(1.0f, 0.0f, 0.0f, 0.95f);
			glBegin(GL_LINE_LOOP);
			for (const auto& point : selected_quad) {
				glVertex2f(point.x, point.y);
			}
			glEnd();
		}

		if (warp.DestinationRoom() == 0xFFFF) {
			glColor4f(1.0f, 0.05f, 0.05f, 0.95f);
		} else {
			switch (warp.warp.type) {
				case Landstalker::WarpList::Warp::Type::STAIR_SE:
					glColor4f(0.0f, 0.95f, 1.0f, 0.95f);
					break;
				case Landstalker::WarpList::Warp::Type::STAIR_SW:
					glColor4f(0.1f, 1.0f, 0.1f, 0.95f);
					break;
				case Landstalker::WarpList::Warp::Type::NORMAL:
				default:
					glColor4f(1.0f, 0.95f, 0.0f, 0.95f);
					break;
			}
		}

		glLineWidth(1.0f);
		glBegin(GL_LINE_LOOP);
		for (const auto& point : quad) {
			glVertex2f(point.x, point.y);
		}
		glEnd();

		if (static_cast<int>(i) == m_canvas.m_selected_warp_idx) {
			for (int axis = 1; axis <= 2; ++axis) {
				PickRect rect = WarpResizeControlRect(warp, axis);
				bool usable = WarpResizeAxisUsable(warp, axis);
				glColor4f(0.02f, 0.02f, 0.02f, 0.6f);
				glBegin(GL_QUADS);
				glVertex2f(rect.min_x, rect.min_y);
				glVertex2f(rect.max_x, rect.min_y);
				glVertex2f(rect.max_x, rect.max_y);
				glVertex2f(rect.min_x, rect.max_y);
				glEnd();

				if (!usable) {
					glColor4f(0.8f, 0.1f, 0.1f, 0.75f);
				} else if (axis == 1) {
					glColor4f(1.0f, 0.95f, 0.1f, 0.95f);
				} else {
					glColor4f(0.0f, 0.95f, 1.0f, 0.95f);
				}
				glLineWidth(2.0f);
				glBegin(GL_LINE_LOOP);
				glVertex2f(rect.min_x, rect.min_y);
				glVertex2f(rect.max_x, rect.min_y);
				glVertex2f(rect.max_x, rect.max_y);
				glVertex2f(rect.min_x, rect.max_y);
				glEnd();
				if (!usable) {
					glBegin(GL_LINES);
					glVertex2f(rect.min_x + 2.0f, rect.min_y + 2.0f);
					glVertex2f(rect.max_x - 2.0f, rect.max_y - 2.0f);
					glVertex2f(rect.max_x - 2.0f, rect.min_y + 2.0f);
					glVertex2f(rect.min_x + 2.0f, rect.max_y - 2.0f);
					glEnd();
				}
				glLineWidth(1.0f);
			}
		}
	}

	glLineWidth(1.0f);
}

void GLCanvasWarpEditor::RenderSelectedWarpTooltip()
{
	if (m_canvas.m_selected_warp_idx < 0 || m_canvas.m_selected_warp_idx >= static_cast<int>(m_canvas.m_warps.size())) {
		return;
	}

	const WarpInstance& warp = m_canvas.m_warps[static_cast<std::size_t>(m_canvas.m_selected_warp_idx)];
	PickPoint anchor = ProjectWarpGridPoint(warp, warp.x + warp.width, warp.y + warp.height, warp.floor_z);

	auto fmt = [](float value) {
		std::ostringstream out;
		out << std::fixed << std::setprecision(0) << value;
		return out.str();
	};

	std::array<std::string, 4> lines = {
		std::string{"ID:"} + std::to_string(warp.instance_id),
		std::string{"X:"} + fmt(warp.x),
		std::string{"Y:"} + fmt(warp.y),
		std::string{"D:"} + HexWord(warp.DestinationRoom())
	};

	float zoom = std::max(m_canvas.ZoomFactor(), 0.0001f);
	float scale = 1.0f;
	float line_height = 10.0f;
	float char_width = 6.0f * scale;
	float text_w = 0.0f;
	for (const auto& line : lines) {
		text_w = std::max(text_w, float(line.size()) * char_width);
	}
	float text_h = float(lines.size()) * line_height;
	float x = anchor.x * zoom + m_canvas.m_cam_x + 48.0f;
	float y = anchor.y * zoom + m_canvas.m_cam_y - 34.0f;

	int w = 0;
	int h = 0;
	m_canvas.GetClientSize(&w, &h);

	glUseProgram(0);
	for (int i = 0; i <= 5; ++i) {
		glActiveTexture(GL_TEXTURE0 + i);
		glDisable(GL_TEXTURE_2D);
	}
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, h, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glColor4f(0.02f, 0.02f, 0.025f, 0.72f);
	glBegin(GL_QUADS);
	glVertex2f(x - 5.0f, y - 5.0f);
	glVertex2f(x + text_w + 5.0f, y - 5.0f);
	glVertex2f(x + text_w + 5.0f, y + text_h + 2.0f);
	glVertex2f(x - 5.0f, y + text_h + 2.0f);
	glEnd();

	if (warp.DestinationRoom() == 0xFFFF) {
		glColor4f(1.0f, 0.4f, 0.4f, 0.95f);
	} else {
		glColor4f(0.9f, 0.9f, 0.88f, 0.95f);
	}
	glLineWidth(1.0f);
	for (std::size_t i = 0; i < lines.size(); ++i) {
		DrawOverlayText(lines[i], x, y + float(i) * line_height, scale);
	}
}
