#ifndef GL_CANVAS_ENTITY_EDITOR_H
#define GL_CANVAS_ENTITY_EDITOR_H

#include "GLCanvas.h"

// Entity-specific editing, hit-testing, dragging, and overlay rendering.
class GLCanvasEntityEditor {
public:
    // Creates an entity editor adapter over canvas-owned state.
    explicit GLCanvasEntityEditor(MyGLCanvas& canvas);

    // Enters pending-add mode for placing a new entity with the cursor.
    void BeginAddEntity();
    // Adds an entity from a prepared preview instance.
    void AddEntity(const SpriteInstance& preview_instance);
    // Returns the topmost entity hit by point, including shadow/body volume.
    int HitTestEntity(const wxPoint& point) const;
    // Returns the topmost entity body hit by point, excluding shadow-only hits.
    int HitTestEntityBody(const wxPoint& point) const;
    // Returns the selected entity if its vertical drag control is hit.
    int HitTestEntityZControl(const wxPoint& point) const;
    // Starts entity dragging, optionally constrained to the Z axis.
    void StartEntityDrag(int entity_idx, const wxMouseEvent& evt, bool z_axis_only, bool shadow_drag = false);
    // Applies the current mouse position to an active entity drag.
    void UpdateEntityDrag(const wxMouseEvent& evt);
    // Finishes an active entity drag and publishes room-data changes.
    void EndEntityDrag();
    // Reprojects a sprite instance after map or Z data changes.
    void UpdateEntityProjection(SpriteInstance& inst);
    // Reloads entity sprite, animation, and hitbox metadata.
    void RefreshEntityMetadata(SpriteInstance& inst);
    // Copies the currently selected entity into the canvas clipboard.
    void CopySelectedEntity();
    // Pastes the copied entity into the current room.
    void PasteEntity();
    // Adjusts the selected or pending entity type ID.
    void CycleSelectedEntityId(int delta);
    // Advances the selected or pending entity palette.
    void CycleSelectedEntityPalette();
    // Sets the selected or pending entity orientation.
    void SetSelectedEntityOrientation(Landstalker::Orientation orientation);
    // Snaps the selected entity to the floor under its hitbox.
    void SetSelectedEntityToFloor();
    // Renders entity selection controls such as the Z handle.
    void RenderEntityControls();
    // Renders the tooltip for the selected entity.
    void RenderSelectedEntityTooltip();
    // Renders a tooltip for an arbitrary entity instance, including previews.
    void RenderEntityTooltipForInstance(const SpriteInstance& inst);

private:
    // Shared drag math used for normal, shadow, and vertical entity drags.
    void ApplyEntityDragStep(
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
        bool drag_floor_snap) const;

    // Canvas whose shared editor state is being manipulated.
    MyGLCanvas& m_canvas;
};

#endif  // GL_CANVAS_ENTITY_EDITOR_H
