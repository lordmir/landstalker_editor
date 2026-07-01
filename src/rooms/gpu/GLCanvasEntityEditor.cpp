#include "GLCanvasEntityEditor.h"
#include "GLLoader.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <sstream>

#include "GLCanvasObjectSupport.h"
#include "PixelFont.h"

namespace {

std::string HexByte(uint8_t value)
{
	constexpr char digits[] = "0123456789ABCDEF";
	std::string out;
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

GLCanvasEntityEditor::GLCanvasEntityEditor(MyGLCanvas& canvas)
	: m_canvas(canvas)
{
}

void GLCanvasEntityEditor::AddEntity()
{
	// Keep room entity data and render instances in lockstep when inserting.
	if (m_canvas.m_room_entities.size() >= 15) {
		return;
	}

	Landstalker::Entity entity;
	auto [cell_x, cell_y] = m_canvas.MouseHeightmapCell();
	if (m_canvas.m_pending_add_type == MyGLCanvas::PendingObjectAddType::Entity &&
		m_canvas.m_pending_add_hover_x >= 0 &&
		m_canvas.m_pending_add_hover_y >= 0) {
		cell_x = m_canvas.m_pending_add_hover_x;
		cell_y = m_canvas.m_pending_add_hover_y;
	}
	float x = static_cast<float>(cell_x) + 0.5f;
	float y = static_cast<float>(cell_y) + 0.5f;
	entity.SetXDbl(std::clamp<double>(x, 0.5, 63.5));
	entity.SetYDbl(std::clamp<double>(y, 0.5, 63.5));
	entity.SetZDbl(m_canvas.FloorUnderPoint(float(entity.GetXDbl()), float(entity.GetYDbl())));
	m_canvas.m_room_entities.push_back(entity);

	SpriteInstance inst{};
	inst.instance_id = static_cast<uint32_t>(m_canvas.m_room_entities.size());
	inst.entity_id = entity.GetType();
	inst.palette = entity.GetPalette();
	inst.map_x = float(entity.GetXDbl());
	inst.map_y = float(entity.GetYDbl());
	inst.map_z = float(entity.GetZDbl());
	inst.z_extent = m_canvas.m_heightmapRenderer.GetZExtent();
	inst.room_left = float(m_canvas.m_mapRenderer.GetRoomLeft());
	inst.room_top = float(m_canvas.m_mapRenderer.GetRoomTop());
	inst.dx = 0.0f;
	inst.dy = 0.0f;
	inst.scale = 2.0f;
	inst.anim_timer = 0.0f;
	inst.anim_speed = 1.0f;
	inst.orientation = entity.GetOrientation();
	m_canvas.RefreshEntityMetadata(inst);
	m_canvas.m_instances.push_back(inst);
	GLCanvasObjectSupport::SortEntitiesGeometrically(m_canvas.m_instances);
	m_canvas.m_selected_entity_idx = m_canvas.FindInstanceIndex(inst.instance_id);
	m_canvas.m_selected_warp_idx = -1;
}

void GLCanvasEntityEditor::CopySelectedEntity()
{
	if (m_canvas.m_selected_entity_idx < 0 || m_canvas.m_selected_entity_idx >= static_cast<int>(m_canvas.m_instances.size())) {
		return;
	}

	const SpriteInstance& inst = m_canvas.m_instances[static_cast<std::size_t>(m_canvas.m_selected_entity_idx)];
	std::size_t slot = inst.instance_id > 0 ? std::size_t(inst.instance_id - 1) : m_canvas.m_room_entities.size();
	Landstalker::Entity entity = slot < m_canvas.m_room_entities.size() ? m_canvas.m_room_entities[slot] : Landstalker::Entity{};
	entity.SetType(inst.entity_id);
	entity.SetPalette(std::min<uint8_t>(inst.palette, 3));
	entity.SetOrientation(inst.orientation);
	entity.SetXDbl(inst.map_x);
	entity.SetYDbl(inst.map_y);
	entity.SetZDbl(inst.map_z);
	m_canvas.m_entity_clipboard = entity;
	m_canvas.m_entity_clipboard_valid = true;
}

void GLCanvasEntityEditor::PasteEntity()
{
	if (!m_canvas.m_entity_clipboard_valid || m_canvas.m_room_entities.size() >= 15) {
		return;
	}

	Landstalker::Entity entity = m_canvas.m_entity_clipboard;
	auto occupied = [this](double x, double y) {
		for (const auto& inst : m_canvas.m_instances) {
			if (std::abs(double(inst.map_x) - x) < 0.25 && std::abs(double(inst.map_y) - y) < 0.25) {
				return true;
			}
		}
		return false;
	};
	double x = std::clamp<double>(entity.GetXDbl() + 1.0, 0.5, 63.5);
	double y = std::clamp<double>(entity.GetYDbl(), 0.5, 63.5);
	for (int tries = 0; tries < 128 && occupied(x, y); ++tries) {
		x += 1.0;
		if (x > 63.5) {
			x = 0.5;
			y = std::clamp<double>(y + 1.0, 0.5, 63.5);
		}
	}
	entity.SetXDbl(x);
	entity.SetYDbl(y);
	m_canvas.m_room_entities.push_back(entity);

	SpriteInstance inst{};
	inst.instance_id = static_cast<uint32_t>(m_canvas.m_room_entities.size());
	inst.entity_id = entity.GetType();
	inst.palette = entity.GetPalette();
	inst.map_x = float(entity.GetXDbl());
	inst.map_y = float(entity.GetYDbl());
	inst.map_z = std::clamp(float(entity.GetZDbl()), 0.0f, 15.5f);
	inst.z_extent = m_canvas.m_heightmapRenderer.GetZExtent();
	inst.room_left = float(m_canvas.m_mapRenderer.GetRoomLeft());
	inst.room_top = float(m_canvas.m_mapRenderer.GetRoomTop());
	inst.dx = 0.0f;
	inst.dy = 0.0f;
	inst.scale = 2.0f;
	inst.anim_timer = 0.0f;
	inst.anim_speed = 1.0f;
	inst.orientation = entity.GetOrientation();
	m_canvas.RefreshEntityMetadata(inst);
	m_canvas.m_instances.push_back(inst);
	GLCanvasObjectSupport::SortEntitiesGeometrically(m_canvas.m_instances);
	m_canvas.m_selected_entity_idx = m_canvas.FindInstanceIndex(inst.instance_id);
	m_canvas.m_hovered_entity_idx = m_canvas.m_selected_entity_idx;
	m_canvas.m_selected_warp_idx = -1;
	m_canvas.m_hovered_warp_idx = -1;
}

void GLCanvasEntityEditor::CycleSelectedEntityId(int delta)
{
	if (m_canvas.m_selected_entity_idx < 0 || m_canvas.m_selected_entity_idx >= static_cast<int>(m_canvas.m_instances.size())) {
		return;
	}
	SpriteInstance& inst = m_canvas.m_instances[static_cast<std::size_t>(m_canvas.m_selected_entity_idx)];
	inst.entity_id = static_cast<uint8_t>((int(inst.entity_id) + delta + 256) & 0xFF);
	m_canvas.RefreshEntityMetadata(inst);
}

void GLCanvasEntityEditor::CycleSelectedEntityPalette()
{
	if (m_canvas.m_selected_entity_idx < 0 || m_canvas.m_selected_entity_idx >= static_cast<int>(m_canvas.m_instances.size())) {
		return;
	}
	SpriteInstance& inst = m_canvas.m_instances[static_cast<std::size_t>(m_canvas.m_selected_entity_idx)];
	inst.palette = static_cast<uint8_t>((inst.palette + 1) % 4);
}

void GLCanvasEntityEditor::SetSelectedEntityOrientation(Landstalker::Orientation orientation)
{
	if (m_canvas.m_selected_entity_idx < 0 || m_canvas.m_selected_entity_idx >= static_cast<int>(m_canvas.m_instances.size())) {
		return;
	}
	m_canvas.m_instances[static_cast<std::size_t>(m_canvas.m_selected_entity_idx)].orientation = orientation;
}

void GLCanvasEntityEditor::SetSelectedEntityToFloor()
{
	if (m_canvas.m_selected_entity_idx < 0 || m_canvas.m_selected_entity_idx >= static_cast<int>(m_canvas.m_instances.size())) {
		return;
	}
	SpriteInstance& inst = m_canvas.m_instances[static_cast<std::size_t>(m_canvas.m_selected_entity_idx)];
	inst.floor_z = m_canvas.FloorUnderHitbox(inst.map_x + inst.hitbox_offset, inst.map_y + inst.hitbox_offset, inst.hitbox_base * 0.5f);
	inst.map_z = std::clamp(inst.floor_z, 0.0f, 15.5f);
	m_canvas.UpdateEntityProjection(inst);
}

void GLCanvasEntityEditor::RenderSelectedEntityTooltip()
{
	if (m_canvas.m_selected_entity_idx < 0 || m_canvas.m_selected_entity_idx >= static_cast<int>(m_canvas.m_instances.size())) {
		return;
	}

	const SpriteInstance& inst = m_canvas.m_instances[static_cast<std::size_t>(m_canvas.m_selected_entity_idx)];
	float center_x = inst.map_x + inst.hitbox_offset;
	float center_y = inst.map_y + inst.hitbox_offset;
	float top_z = inst.map_z + std::max(inst.hitbox_height, 0.125f);
	float grid_x = center_x - inst.room_left;
	float grid_y = center_y - inst.room_top;
	float anchor_x = 32.0f * grid_x - 32.0f * grid_y + 512.0f;
	float anchor_y = 16.0f * grid_x + 16.0f * grid_y + 100.0f - inst.z_extent * top_z;

	auto fmt = [](float value) {
		std::ostringstream out;
		out << std::fixed << std::setprecision(1) << value;
		return out.str();
	};

	std::array<std::string, 5> lines = {
		std::string{"I:"} + std::to_string(std::clamp<int>(static_cast<int>(inst.instance_id), 1, 15)),
		std::string{"EID:"} + HexByte(inst.entity_id),
		std::string{"X:"} + fmt(inst.map_x),
		std::string{"Y:"} + fmt(inst.map_y),
		std::string{"Z:"} + fmt(inst.map_z)
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
	float x = anchor_x * zoom + m_canvas.m_cam_x + 14.0f;
	float y = anchor_y * zoom + m_canvas.m_cam_y - 44.0f;

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

	glLineWidth(1.0f);
	glColor4f(0.9f, 0.9f, 0.88f, 0.95f);
	for (std::size_t i = 0; i < lines.size(); ++i) {
		DrawOverlayText(lines[i], x, y + float(i) * line_height, scale);
	}
}
