# GPU Room Editor Architecture

This directory contains the GPU-backed replacement for the old room viewer and
room editing controls. The existing `RoomViewerFrame` still owns the editor
window, panes, menus, toolbars, and property grid; the central editing surface is
now `MyGLCanvas`.

## High-Level Ownership

```text
RoomViewerFrame
  owns panes, toolbars, menus, property grid
  owns MyGLCanvas
      owns renderers
      owns room/editor/selection/camera state
      delegates input and rendering by editor mode
      posts wx events back to RoomViewerFrame
```

`RoomViewerFrame` owns:

- `LayerControlFrame`
- `EntityControlFrame`
- `WarpControlFrame`
- `TileSwapControlFrame`
- `BlocksetEditorCtrl`
- `MyGLCanvas`

The frame talks to `MyGLCanvas` through public methods such as `SetRoomNum`,
`SetEditorMode`, `SelectEntityByIndex`, `SetBackgroundOpacity`,
`SetSelectedBlockId`, and the heightmap/tilemap editing operations.

## Main GPU Control

`MyGLCanvas` is the coordination hub. It owns:

- Current room and edit mode.
- Camera, zoom, visibility, opacity, and highlight state.
- Object selections for entities, warps, tile swaps, and doors.
- Layer and heightmap cell selection/clipboard state.
- Runtime sprite/warp geometry caches.
- The main renderers:
  - `MapRenderer`
  - `HeightmapRenderer`
  - `SpriteRenderer`

Input is received by `MyGLCanvas` and delegated to a mode helper based on
`EditorMode`:

- `EditorMode::Room` -> `GLCanvasRoomMode`
- `EditorMode::BackgroundLayer` / `ForegroundLayer` -> `GLCanvasLayerEditMode`
- `EditorMode::Heightmap` -> `GLCanvasHeightmapMode`

The mode helpers are intentionally thin adapters over `MyGLCanvas`; they do not
own editor state independently.

## Renderers

### `MapRenderer`

Owns OpenGL resources for room tilemaps:

- Background/foreground map textures.
- Combined blockset texture.
- Tileset texture.
- Room palette texture.
- Animation metadata textures.
- Map shader program.

Responsibilities:

- Render full background/foreground layers.
- Render individual background/foreground layers for edit modes.
- Render priority-bit highlights.
- Render clipboard/preview block ghosts.
- Build the foreground priority stencil used by sprite occlusion.

### `HeightmapRenderer`

Renders room heightmap geometry and overlays. It can render either the real room
tilemap or a preview map.

Responsibilities:

- Render heightmap cells.
- Track heightmap hover.
- Build depth/occlusion stencil data.
- Render/debug entity occlusion.
- Provide configurable Z extent for the GPU view.

### `SpriteRenderer`

Owns sprite atlas/palette GPU resources and renders `SpriteInstance` data.

Responsibilities:

- Render entity sprites.
- Render selection and collision state.
- Render hitboxes/shadows.
- Apply entity occlusion modes.
- Manage sprite opacity.

## Mode Helpers

### `GLCanvasRoomMode`

Handles normal room editing:

- Object selection and hover.
- Dragging entities, warps, doors, and tile-swap regions.
- Tab selection.
- Object shortcuts.
- Layer opacity shortcuts.
- Room-mode rendering of map, heightmap, sprites, warps, doors, and swaps.

### `GLCanvasLayerEditMode`

Handles background/foreground map editing:

- Cell hover and selection.
- Keyboard movement of selected cell.
- Copy/paste of block IDs.
- Clipboard ghost preview.
- Priority-bit highlight rendering.
- Tilemap row/column editing.

### `GLCanvasHeightmapMode`

Handles heightmap editing:

- Heightmap cell hover and selection.
- Keyboard movement of selected cell.
- Cell copy/paste.
- Height/type/restriction changes.
- Heightmap row/column editing.
- Heightmap-mode rendering.

## Object Editing Helpers

These classes group object-specific behavior while operating directly on
`MyGLCanvas` state.

### `GLCanvasEntityEditor`

Handles entity-specific operations:

- Add/copy/paste entity.
- Cycle entity ID and palette.
- Set orientation.
- Place selected entity on the floor.
- Render selected entity tooltip.

### `GLCanvasWarpEditor`

Handles warp-specific operations:

- Add warp half.
- Find nearest free warp cell.
- Resize and rotate selected warp.
- Cycle warp type.
- Render warps and selected warp tooltip.

### `GLCanvasTileDoorEditor`

Handles tile swaps and doors:

- Add door.
- Add tile swap.
- Resize selected tile-swap region.
- Cycle tile-swap shape and ID.
- Cycle door size.
- Toggle tile-swap and door previews.
- Render tile-swap outlines and selected tooltips.

### `GLCanvasObjectCoordinator`

Handles cross-object operations:

- Delete selected object.
- Reorder selected object.
- Select next object.
- Select next tile-swap region.
- Nudge selected object.

## Shared Support Types

### `GLCanvasObjectSupport`

Contains shared geometry and conversion helpers:

- Tile-swap region geometry construction.
- Tile-swap region metrics.
- Warp instance construction and valid-size clamping.
- Geometric entity sorting.

### `SpriteInstance`

Runtime representation of an entity sprite in the GPU editor. It stores:

- Entity ID and palette.
- Screen and map position.
- Floor/Z data.
- Hitbox dimensions.
- Animation timer/speed.
- Orientation.

### `ShaderSources.h`

Embeds GLSL shader source strings directly into the executable. This avoids
runtime shader file loading.

### `PixelFont.h`

Provides the bitmap font used for GPU overlay text, labels, and tooltips.

## Frame Integration Events

`MyGLCanvas` posts wx events back to `RoomViewerFrame` so the existing panes and
properties stay in sync.

- `EVT_GPU_EDITOR_MODE_CHANGE`
  - GPU edit mode changed.
  - The frame updates mode state, toolbar checks, menu checks, and control
    availability.

- `EVT_GPU_LAYER_OPACITY_CHANGE`
  - Keyboard opacity shortcuts changed GPU layer opacity.
  - The frame updates the sliders in the Layers pane.

- `EVT_GPU_LAYER_BLOCK_SELECT`
  - A background/foreground layer cell was selected in the GPU editor.
  - The frame selects the corresponding block in `BlocksetEditorCtrl`.

- Existing selection events:
  - `EVT_ENTITY_SELECT`
  - `EVT_WARP_SELECT`
  - `EVT_TILESWAP_SELECT`
  - `EVT_DOOR_SELECT`
  - These update the list panes when GPU selection changes.

- Existing update events:
  - `EVT_ENTITY_UPDATE`
  - `EVT_WARP_UPDATE`
  - `EVT_TILESWAP_UPDATE`
  - `EVT_DOOR_UPDATE`
  - These refresh list panes and property views after GPU edits.

- Navigation/properties:
  - Room navigation posts `EVT_GO_TO_NAV_ITEM` and property refresh events so
    the navigation pane and property grid follow GPU-driven room changes.

## Design Notes

The current design deliberately keeps `MyGLCanvas` as the central coordination
object. The mode and object-helper classes are mostly organizational adapters
over that shared state. This made the port from the old controls practical and
keeps existing frame/pane integration straightforward.

The main tradeoff is that `MyGLCanvas` remains large and friend-heavy. If this
area is refactored later, the natural next step would be to move more state and
contracts into explicit model/controller objects so the helper classes can own
less implicit canvas state.
