#include "GLCanvasEntityEditor.h"
#include "GLLoader.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <sstream>

#include "GLCanvasObjectSupport.h"
#include "PixelFont.h"
#include "RoomProjection.h"

#include <landstalker/misc/Utils.h>

namespace {

using PickPoint = RoomProjection::PickPoint;
using GLCanvasObjectSupport::HitboxBaseToBlocks;
using GLCanvasObjectSupport::HitboxDrawOffset;
using GLCanvasObjectSupport::HitboxHeightToBlocks;
using RoomProjection::ProjectEntityGridPoint;
using RoomProjection::ScreenToMapPoint;

struct PickRect {
	float min_x;
	float min_y;
	float max_x;
	float max_y;
};

PickRect EntityZControlRect(const SpriteInstance& inst)
{
	float center_x = inst.map_x + inst.hitbox_offset;
	float center_y = inst.map_y + inst.hitbox_offset;
	float top_z = inst.map_z + std::max(inst.hitbox_height, 0.125f);
	PickPoint top_center = ProjectEntityGridPoint(inst, center_x, center_y, top_z);
	constexpr float half_size = 6.0f;
	constexpr float y_offset = 14.0f;
	return {
		top_center.x - half_size,
		top_center.y - y_offset - half_size,
		top_center.x + half_size,
		top_center.y - y_offset + half_size
	};
}

bool PointInRect(const PickPoint& point, const PickRect& rect)
{
	return point.x >= rect.min_x &&
	       point.x <= rect.max_x &&
	       point.y >= rect.min_y &&
	       point.y <= rect.max_y;
}

float Cross(const PickPoint& a, const PickPoint& b, const PickPoint& c)
{
	return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

bool PointInQuad(const PickPoint& point, const std::array<PickPoint, 4>& quad)
{
	bool has_positive = false;
	bool has_negative = false;
	for (std::size_t i = 0; i < quad.size(); ++i) {
		float cross = Cross(quad[i], quad[(i + 1) % quad.size()], point);
		has_positive = has_positive || cross > 0.0f;
		has_negative = has_negative || cross < 0.0f;
		if (has_positive && has_negative) {
			return false;
		}
	}
	return true;
}

bool PointInEntityHitbox(const SpriteInstance& inst, const PickPoint& point, bool include_shadow = true)
{
	if (inst.hitbox_base <= 0.0f) {
		return false;
	}

	float center_x = inst.map_x + inst.hitbox_offset;
	float center_y = inst.map_y + inst.hitbox_offset;
	float half_base = inst.hitbox_base * 0.5f;
	float min_x = center_x - half_base;
	float min_y = center_y - half_base;
	float max_x = center_x + half_base;
	float max_y = center_y + half_base;
	float bottom_z = inst.map_z;
	float height = std::max(inst.hitbox_height, 0.125f);
	std::array<PickPoint, 4> bottom = {
		ProjectEntityGridPoint(inst, min_x, min_y, bottom_z),
		ProjectEntityGridPoint(inst, max_x, min_y, bottom_z),
		ProjectEntityGridPoint(inst, max_x, max_y, bottom_z),
		ProjectEntityGridPoint(inst, min_x, max_y, bottom_z)
	};
	std::array<PickPoint, 4> top = {
		ProjectEntityGridPoint(inst, min_x, min_y, bottom_z + height),
		ProjectEntityGridPoint(inst, max_x, min_y, bottom_z + height),
		ProjectEntityGridPoint(inst, max_x, max_y, bottom_z + height),
		ProjectEntityGridPoint(inst, min_x, max_y, bottom_z + height)
	};
	std::array<PickPoint, 4> shadow = {
		ProjectEntityGridPoint(inst, min_x, min_y, inst.floor_z),
		ProjectEntityGridPoint(inst, max_x, min_y, inst.floor_z),
		ProjectEntityGridPoint(inst, max_x, max_y, inst.floor_z),
		ProjectEntityGridPoint(inst, min_x, max_y, inst.floor_z)
	};

	if (PointInQuad(point, bottom) || PointInQuad(point, top) || (include_shadow && PointInQuad(point, shadow))) {
		return true;
	}

	for (std::size_t i = 0; i < bottom.size(); ++i) {
		std::array<PickPoint, 4> side = {
			bottom[i],
			bottom[(i + 1) % bottom.size()],
			top[(i + 1) % top.size()],
			top[i]
		};
		if (PointInQuad(point, side)) {
			return true;
		}
	}
	return false;
}

using PixelFont::DrawOverlayText;

}  // namespace

void GLCanvas::UpdateEntityProjection(SpriteInstance& inst)
{
	GLCanvasEntityEditor(*this).UpdateEntityProjection(inst);
}

void GLCanvas::RefreshEntityMetadata(SpriteInstance& inst)
{
	GLCanvasEntityEditor(*this).RefreshEntityMetadata(inst);
}

GLCanvasEntityEditor::GLCanvasEntityEditor(GLCanvas& canvas)
	: m_canvas(canvas)
{
}

void GLCanvasEntityEditor::BeginAddEntity()
{
	if (m_canvas.m_room_entities.size() >= 15) {
		return;
	}
	m_canvas.SetFocus();
	m_canvas.m_pending_add_type = GLCanvas::PendingObjectAddType::Entity;

	float room_left = static_cast<float>(m_canvas.m_mapRenderer.GetRoomLeft());
	float room_top = static_cast<float>(m_canvas.m_mapRenderer.GetRoomTop());
	float z_extent = m_canvas.m_heightmapRenderer.GetZExtent();

	m_canvas.m_pending_add_floor_snap = true;

	float seed_center_x = 0.5f;
	float seed_center_y = 0.5f;
	if (m_canvas.m_last_mouse_pos.x >= 0 && m_canvas.m_last_mouse_pos.y >= 0) {
		float world_x = m_canvas.ScreenToWorldX(m_canvas.m_last_mouse_pos.x);
		float world_y = m_canvas.ScreenToWorldY(m_canvas.m_last_mouse_pos.y);
		PickPoint coarse = ScreenToMapPoint(world_x, world_y, 0.0f, room_left, room_top, z_extent);
		seed_center_x = std::clamp(coarse.x, 0.0f, 63.5f);
		seed_center_y = std::clamp(coarse.y, 0.0f, 63.5f);
	}

	m_canvas.m_pending_add_plane_z = std::clamp(m_canvas.FloorUnderPoint(seed_center_x, seed_center_y), 0.0f, 15.5f);
	m_canvas.m_pending_add_entity_cursor_offset_x = 0.0f;
	m_canvas.m_pending_add_entity_cursor_offset_y = 0.0f;

	m_canvas.UpdatePendingObjectAddHover();

	if (m_canvas.m_pending_add_hover_x >= 0 && m_canvas.m_pending_add_hover_y >= 0) {
		seed_center_x = static_cast<float>(m_canvas.m_pending_add_hover_x) + 0.5f;
		seed_center_y = static_cast<float>(m_canvas.m_pending_add_hover_y) + 0.5f;
	} else if (m_canvas.m_last_mouse_pos.x >= 0 && m_canvas.m_last_mouse_pos.y >= 0) {
		float world_x = m_canvas.ScreenToWorldX(m_canvas.m_last_mouse_pos.x);
		float world_y = m_canvas.ScreenToWorldY(m_canvas.m_last_mouse_pos.y);
		PickPoint precise = ScreenToMapPoint(world_x, world_y, m_canvas.m_pending_add_plane_z, room_left, room_top, z_extent);
		seed_center_x = std::clamp(precise.x, 0.0f, 63.5f);
		seed_center_y = std::clamp(precise.y, 0.0f, 63.5f);
	}

	m_canvas.m_pending_add_start_x = seed_center_x;
	m_canvas.m_pending_add_start_y = seed_center_y;
	m_canvas.m_pending_add_start_z = m_canvas.m_pending_add_plane_z;
	m_canvas.m_pending_add_mouse_start = m_canvas.m_last_mouse_pos;

	m_canvas.SetCursor(wxCursor(wxCURSOR_CROSS));
	m_canvas.Refresh();
}

int GLCanvasEntityEditor::HitTestEntity(const wxPoint& point) const
{
	PickPoint world_point{
		m_canvas.ScreenToWorldX(point.x),
		m_canvas.ScreenToWorldY(point.y)
	};

	for (int i = static_cast<int>(m_canvas.m_instances.size()) - 1; i >= 0; --i) {
		if (PointInEntityHitbox(m_canvas.m_instances[static_cast<std::size_t>(i)], world_point)) {
			return i;
		}
	}
	return -1;
}

int GLCanvasEntityEditor::HitTestEntityBody(const wxPoint& point) const
{
	PickPoint world_point{
		m_canvas.ScreenToWorldX(point.x),
		m_canvas.ScreenToWorldY(point.y)
	};

	for (int i = static_cast<int>(m_canvas.m_instances.size()) - 1; i >= 0; --i) {
		if (PointInEntityHitbox(m_canvas.m_instances[static_cast<std::size_t>(i)], world_point, false)) {
			return i;
		}
	}
	return -1;
}

int GLCanvasEntityEditor::HitTestEntityZControl(const wxPoint& point) const
{
	PickPoint world_point{
		m_canvas.ScreenToWorldX(point.x),
		m_canvas.ScreenToWorldY(point.y)
	};

	if (m_canvas.m_selected_entity_idx >= 0 &&
		m_canvas.m_selected_entity_idx < static_cast<int>(m_canvas.m_instances.size()) &&
		PointInRect(world_point, EntityZControlRect(m_canvas.m_instances[static_cast<std::size_t>(m_canvas.m_selected_entity_idx)]))) {
		return m_canvas.m_selected_entity_idx;
	}

	return -1;
}

void GLCanvasEntityEditor::StartEntityDrag(int entity_idx, const wxMouseEvent& evt, bool z_axis_only, bool shadow_drag)
{
	if (entity_idx < 0 || entity_idx >= static_cast<int>(m_canvas.m_instances.size())) {
		return;
	}

	m_canvas.CaptureObjectUndoState();
	SpriteInstance& inst = m_canvas.m_instances[static_cast<std::size_t>(entity_idx)];
	m_canvas.m_dragging_entity = true;
	m_canvas.m_drag_z_axis_only = z_axis_only;
	m_canvas.m_drag_instance_id = inst.instance_id;
	m_canvas.m_drag_start_mouse = evt.GetPosition();
	m_canvas.m_drag_start_x = inst.map_x;
	m_canvas.m_drag_start_y = inst.map_y;
	m_canvas.m_drag_start_z = inst.map_z;
	m_canvas.m_drag_plane_z = shadow_drag ? inst.floor_z : inst.map_z;
	PickPoint cursor_map = ScreenToMapPoint(
		m_canvas.ScreenToWorldX(evt.GetPosition().x),
		m_canvas.ScreenToWorldY(evt.GetPosition().y),
		m_canvas.m_drag_plane_z,
		inst.room_left,
		inst.room_top,
		inst.z_extent);
	m_canvas.m_drag_cursor_offset_x = inst.map_x + inst.hitbox_offset - cursor_map.x;
	m_canvas.m_drag_cursor_offset_y = inst.map_y + inst.hitbox_offset - cursor_map.y;
	m_canvas.m_drag_floor_snap = std::abs(inst.map_z - inst.floor_z) <= 0.01f;
	m_canvas.SetCursor(wxCursor(z_axis_only ? wxCURSOR_SIZENS : wxCURSOR_HAND));
	if (!m_canvas.HasCapture()) {
		m_canvas.CaptureMouse();
	}
}

void GLCanvasEntityEditor::ApplyEntityDragStep(
	SpriteInstance& inst,
	const wxPoint& mouse_pos,
	bool z_axis_only,
	const wxPoint& drag_start_mouse,
	float drag_start_x,
	float drag_start_y,
	float drag_start_z,
	float drag_plane_z,
	float drag_cursor_offset_x,
	float drag_cursor_offset_y,
	bool drag_floor_snap) const
{
	auto snap_half = [](float value) {
		return std::round(value * 2.0f) * 0.5f;
	};
	auto clamp_map_pos = [](float value) {
		return std::clamp(value, 0.0f, 63.5f);
	};

	if (z_axis_only) {
		float dy = static_cast<float>(mouse_pos.y - drag_start_mouse.y);
		inst.map_x = clamp_map_pos(drag_start_x);
		inst.map_y = clamp_map_pos(drag_start_y);
		inst.map_z = std::clamp(snap_half(drag_start_z - dy / 32.0f), 0.0f, 15.5f);
	} else {
		float world_x = m_canvas.ScreenToWorldX(mouse_pos.x);
		float world_y = m_canvas.ScreenToWorldY(mouse_pos.y);
		PickPoint cursor_map = ScreenToMapPoint(world_x, world_y, drag_plane_z, inst.room_left, inst.room_top, inst.z_extent);
		float hitbox_center_x = cursor_map.x + drag_cursor_offset_x;
		float hitbox_center_y = cursor_map.y + drag_cursor_offset_y;
		inst.map_x = clamp_map_pos(snap_half(hitbox_center_x - inst.hitbox_offset));
		inst.map_y = clamp_map_pos(snap_half(hitbox_center_y - inst.hitbox_offset));
	}

	inst.floor_z = m_canvas.FloorUnderHitbox(
		inst.map_x + inst.hitbox_offset,
		inst.map_y + inst.hitbox_offset,
		inst.hitbox_base * 0.5f);
	if (!z_axis_only && drag_floor_snap) {
		inst.map_z = std::clamp(inst.floor_z, 0.0f, 15.5f);
	} else if (!z_axis_only) {
		inst.map_z = std::clamp(drag_start_z, 0.0f, 15.5f);
	}
}

void GLCanvasEntityEditor::UpdateEntityDrag(const wxMouseEvent& evt)
{
	int entity_idx = m_canvas.FindInstanceIndex(m_canvas.m_drag_instance_id);
	if (entity_idx < 0) {
		EndEntityDrag();
		return;
	}

	SpriteInstance& inst = m_canvas.m_instances[static_cast<std::size_t>(entity_idx)];
	bool z_axis_only = m_canvas.m_drag_z_axis_only || evt.ControlDown() || evt.RightIsDown();
	m_canvas.SetCursor(wxCursor(z_axis_only ? wxCURSOR_SIZENS : wxCURSOR_HAND));
	ApplyEntityDragStep(
		inst,
		evt.GetPosition(),
		z_axis_only,
		m_canvas.m_drag_start_mouse,
		m_canvas.m_drag_start_x,
		m_canvas.m_drag_start_y,
		m_canvas.m_drag_start_z,
		m_canvas.m_drag_plane_z,
		m_canvas.m_drag_cursor_offset_x,
		m_canvas.m_drag_cursor_offset_y,
		m_canvas.m_drag_floor_snap);
	UpdateEntityProjection(inst);
	GLCanvasObjectSupport::SortEntitiesGeometrically(m_canvas.m_instances);
	entity_idx = m_canvas.FindInstanceIndex(m_canvas.m_drag_instance_id);
	m_canvas.m_selected_entity_idx = entity_idx;
	m_canvas.m_hovered_entity_idx = entity_idx;
	m_canvas.Refresh();
}

void GLCanvasEntityEditor::EndEntityDrag()
{
	if (!m_canvas.m_dragging_entity) {
		return;
	}

	uint32_t dragged_id = m_canvas.m_drag_instance_id;
	m_canvas.m_dragging_entity = false;
	if (m_canvas.HasCapture()) {
		m_canvas.ReleaseMouse();
	}
	GLCanvasObjectSupport::SortEntitiesGeometrically(m_canvas.m_instances);
	m_canvas.m_selected_entity_idx = m_canvas.FindInstanceIndex(dragged_id);
	m_canvas.m_hovered_entity_idx = m_canvas.m_selected_entity_idx;
	m_canvas.SetCursor(wxCursor(m_canvas.m_hovered_entity_idx >= 0 ? wxCURSOR_HAND : wxCURSOR_ARROW));
	m_canvas.NotifyRoomDataChanged(true, false, false, false);
	m_canvas.Refresh();
}

void GLCanvasEntityEditor::UpdateEntityProjection(SpriteInstance& inst)
{
	float ex_block = inst.map_x + inst.hitbox_offset - inst.room_left;
	float ey_block = inst.map_y + inst.hitbox_offset - inst.room_top;
	inst.x = 32.0f * ex_block - 32.0f * ey_block + 512.0f;
	inst.y = 16.0f * ex_block + 16.0f * ey_block + 100.0f - inst.map_z * inst.z_extent;
}

void GLCanvasEntityEditor::RefreshEntityMetadata(SpriteInstance& inst)
{
	auto sd = m_canvas.m_gd->GetSpriteData();
	if (sd->IsEntity(inst.entity_id)) {
		auto hitbox = sd->GetEntityHitbox(inst.entity_id);
		inst.hitbox_base = HitboxBaseToBlocks(hitbox.base);
		inst.hitbox_height = HitboxHeightToBlocks(hitbox.height);
	} else {
		inst.hitbox_base = 1.0f;
		inst.hitbox_height = 1.0f;
	}
	inst.hitbox_offset = HitboxDrawOffset(inst.hitbox_base);
	inst.floor_z = m_canvas.FloorUnderHitbox(
		inst.map_x + inst.hitbox_offset,
		inst.map_y + inst.hitbox_offset,
		inst.hitbox_base * 0.5f);
	UpdateEntityProjection(inst);
}

void GLCanvasEntityEditor::AddEntity(const SpriteInstance& preview_instance)
{
	// Keep room entity data and render instances in lockstep when inserting.
	if (m_canvas.m_room_entities.size() >= 15) {
		return;
	}

	// Create entity from ghost position
	Landstalker::Entity entity;
	entity.SetType(m_canvas.m_pending_add_entity_id);
	entity.SetPalette(std::min<uint8_t>(m_canvas.m_pending_add_entity_palette, 3));
	entity.SetOrientation(m_canvas.m_pending_add_entity_orientation);
	entity.SetXDbl(preview_instance.map_x);
	entity.SetYDbl(preview_instance.map_y);
	entity.SetZDbl(preview_instance.map_z);
	m_canvas.m_room_entities.push_back(entity);

	// Create sprite instance from ghost
	SpriteInstance inst = preview_instance;
	inst.instance_id = static_cast<uint32_t>(m_canvas.m_room_entities.size());
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
	if (m_canvas.m_pending_add_type == GLCanvas::PendingObjectAddType::Entity) {
		m_canvas.m_pending_add_entity_id = static_cast<uint8_t>((int(m_canvas.m_pending_add_entity_id) + delta + 256) & 0xFF);
		m_canvas.Refresh();
		return;
	}
	if (m_canvas.m_selected_entity_idx < 0 || m_canvas.m_selected_entity_idx >= static_cast<int>(m_canvas.m_instances.size())) {
		return;
	}
	SpriteInstance& inst = m_canvas.m_instances[static_cast<std::size_t>(m_canvas.m_selected_entity_idx)];
	inst.entity_id = static_cast<uint8_t>((int(inst.entity_id) + delta + 256) & 0xFF);
	m_canvas.RefreshEntityMetadata(inst);
}

void GLCanvasEntityEditor::CycleSelectedEntityPalette()
{
	if (m_canvas.m_pending_add_type == GLCanvas::PendingObjectAddType::Entity) {
		m_canvas.m_pending_add_entity_palette = static_cast<uint8_t>((m_canvas.m_pending_add_entity_palette + 1) % 4);
		m_canvas.Refresh();
		return;
	}
	if (m_canvas.m_selected_entity_idx < 0 || m_canvas.m_selected_entity_idx >= static_cast<int>(m_canvas.m_instances.size())) {
		return;
	}
	SpriteInstance& inst = m_canvas.m_instances[static_cast<std::size_t>(m_canvas.m_selected_entity_idx)];
	inst.palette = static_cast<uint8_t>((inst.palette + 1) % 4);
}

void GLCanvasEntityEditor::SetSelectedEntityOrientation(Landstalker::Orientation orientation)
{
	if (m_canvas.m_pending_add_type == GLCanvas::PendingObjectAddType::Entity) {
		m_canvas.m_pending_add_entity_orientation = orientation;
		m_canvas.Refresh();
		return;
	}
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

void GLCanvasEntityEditor::RenderEntityControls()
{
	auto draw_control = [this](int entity_idx, bool selected) {
		if (entity_idx < 0 || entity_idx >= static_cast<int>(m_canvas.m_instances.size())) {
			return;
		}

		const SpriteInstance& inst = m_canvas.m_instances[static_cast<std::size_t>(entity_idx)];
		PickRect rect = EntityZControlRect(inst);
		float center_x = inst.map_x + inst.hitbox_offset;
		float center_y = inst.map_y + inst.hitbox_offset;
		float top_z = inst.map_z + std::max(inst.hitbox_height, 0.125f);
		PickPoint top_center = ProjectEntityGridPoint(inst, center_x, center_y, top_z);
		PickPoint handle_center{
			(rect.min_x + rect.max_x) * 0.5f,
			(rect.min_y + rect.max_y) * 0.5f
		};

		glColor4f(selected ? 1.0f : 0.85f, selected ? 0.2f : 0.9f, selected ? 0.2f : 1.0f, 0.95f);
		glLineWidth(1.0f);
		glBegin(GL_LINES);
		glVertex2f(top_center.x, top_center.y);
		glVertex2f(handle_center.x, handle_center.y);
		glEnd();

		glColor4f(0.02f, 0.02f, 0.02f, 0.55f);
		glBegin(GL_QUADS);
		glVertex2f(rect.min_x, rect.min_y);
		glVertex2f(rect.max_x, rect.min_y);
		glVertex2f(rect.max_x, rect.max_y);
		glVertex2f(rect.min_x, rect.max_y);
		glEnd();

		glColor4f(selected ? 1.0f : 0.85f, selected ? 0.2f : 0.9f, selected ? 0.2f : 1.0f, 0.95f);
		glLineWidth(2.0f);
		glBegin(GL_LINE_LOOP);
		glVertex2f(rect.min_x, rect.min_y);
		glVertex2f(rect.max_x, rect.min_y);
		glVertex2f(rect.max_x, rect.max_y);
		glVertex2f(rect.min_x, rect.max_y);
		glEnd();
		glLineWidth(1.0f);
	};

	glUseProgram(0);
	for (int i = 0; i <= 5; ++i) {
		glActiveTexture(GL_TEXTURE0 + i);
		glDisable(GL_TEXTURE_2D);
	}
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	draw_control(m_canvas.m_selected_entity_idx, true);
}

void GLCanvasEntityEditor::RenderSelectedEntityTooltip()
{
	if (m_canvas.m_selected_entity_idx < 0 || m_canvas.m_selected_entity_idx >= static_cast<int>(m_canvas.m_instances.size())) {
		return;
	}

	const SpriteInstance& inst = m_canvas.m_instances[static_cast<std::size_t>(m_canvas.m_selected_entity_idx)];
	RenderEntityTooltipForInstance(inst);
}

void GLCanvasEntityEditor::RenderEntityTooltipForInstance(const SpriteInstance& inst)
{
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
		Landstalker::StrPrintf("EID:%02X", inst.entity_id),
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
