# GPU Room Editor Architecture

This directory contains the GPU-backed room editor used by `RoomViewerFrame`.
The frame still owns the panes, menus, toolbars, property grid, and list
controls. The central editing surface is `MyGLCanvas`.

For user-facing controls and shortcuts, see [USER_GUIDE.md](USER_GUIDE.md).

## High-Level Ownership

```text
RoomViewerFrame
  owns panes, toolbars, menus, property grid
  owns MyGLCanvas
      owns renderers
      owns room/editor/selection/camera state
      delegates mode behavior and object behavior
      posts wx events back to RoomViewerFrame
```

`RoomViewerFrame` owns the surrounding UI:

- `LayerControlFrame`
- `EntityControlFrame`
- `WarpControlFrame`
- `TileSwapControlFrame`
- `BlocksetEditorCtrl`
- `MyGLCanvas`

The frame talks to `MyGLCanvas` through public methods such as `SetRoomNum`,
`SetEditorMode`, `SetDrawingTool`, `SelectEntityByIndex`,
`SetBackgroundOpacity`, `SetSelectedBlockId`, and the row/column/object editing
commands.

## Main GPU Control

`MyGLCanvas` is the coordination hub. It owns:

- Current room and edit mode.
- Camera, zoom, visibility, opacity, and highlight state.
- Object selections for entities, warps, tile swaps, and doors.
- Layer and heightmap selection/clipboard state.
- Pending-add, drag, undo/redo, and preview state.
- Runtime sprite/warp geometry caches.
- The main renderers:
  - `MapRenderer`
  - `HeightmapRenderer`
  - `SpriteRenderer`
- A persistent `GLCanvasRoomInfoOverlay`.

Input is received by `MyGLCanvas` and delegated by `EditorMode`:

- `EditorMode::Room` -> `GLCanvasRoomMode`
- `EditorMode::BackgroundLayer` / `ForegroundLayer` -> `GLCanvasLayerEditMode`
- `EditorMode::Heightmap` -> `GLCanvasHeightmapMode`

Most helpers are still adapters over canvas-owned state. This keeps frame/pane
integration simple while letting behavior live outside the main canvas file.

## Current File Layout

- `GLCanvas.cpp`
  - Canvas construction, frame integration events, room loading/navigation,
    mode switching, camera/zoom, status updates, animation timing, top-level
    mouse/key dispatch, and `OnPaint`.
- `GLCanvas.h`
  - Public editor API, shared private state, undo-state structs, and friend
    declarations for helper classes.
- `GLCanvasHistory.cpp`
  - Undo/redo state capture and restore for maps, layers, heightmaps, objects,
    and pending object-add state.
- `GLCanvasObjectCommands.cpp`
  - Thin canvas methods for object drag forwarding, pending object-add,
    object command wrappers, hit-test delegates, and render delegates.
- `GLCanvasTilemapEditing.cpp`
  - Layer and heightmap selection, clipboard, stamping, flood fill, shape
    drawing, row/column edits, and selected-cell mutation.
- `GLCanvasEditorOverlays.cpp`
  - Background-layer and heightmap editor overlays, selection outlines, brush
    previews, and overlay text helpers.
- `GLCanvasHeightmapHitTest.cpp`
  - Heightmap floor/collision/occlusion hit tests used by object placement and
    sprite rendering.
- `RoomProjection.h`
  - Shared room, entity, warp, and heightmap projection helpers.

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
- Render individual layers for edit modes.
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
- Render editor overlays and preview maps.
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
- Mouse gestures for entities, warps, doors, tile-swap regions, room links, and
  panning.
- Room-mode shortcuts.
- Room-mode render ordering for map, heightmap, tile swaps, doors, warps,
  pending additions, sprites, controls, tooltips, and room-info links.

### `GLCanvasLayerEditMode`

Handles background/foreground map editing:

- Cell hover and selection.
- Selection drag/move behavior.
- Drawing tool mouse handling.
- Keyboard movement of selected cells.
- Copy/paste of block IDs.
- Clipboard ghost preview.
- Priority-bit and heightmap overlay rendering.

### `GLCanvasHeightmapMode`

Handles heightmap editing:

- Heightmap hover and selection.
- Selection drag/move behavior.
- Drawing tool mouse handling.
- Cell copy/paste.
- Height/type/restriction changes.
- Heightmap view-mode rendering.

## Object Editing Helpers

These helpers group object-specific behavior while operating on `MyGLCanvas`
state.

### `GLCanvasEntityEditor`

Handles entity-specific operations:

- Begin/add/copy/paste entity.
- Entity drag and Z drag.
- Cycle entity ID and palette.
- Set orientation.
- Place selected entity on the floor.
- Entity hit tests, controls, and tooltips.

### `GLCanvasWarpEditor`

Handles warp-specific operations:

- Begin/add warp half.
- Find nearest free warp cell.
- Warp drag and resize drag.
- Resize and rotate selected warp.
- Cycle warp type.
- Warp hit tests, rendering, pending ghost, and tooltip.

### `GLCanvasTileDoorEditor`

The public helper class for both door and tile-swap commands. Its
implementation is split by domain:

- `GLCanvasTileDoorEditor.cpp`
  - Door add/drag/resize-size cycling.
  - Door preview toggling.
  - Door hit tests, rendering, pending ghost, and tooltip.
  - Shared preview reset.
- `GLCanvasTileSwapEditor.cpp`
  - Tile-swap add/commit flow.
  - Tile-swap drag/resize.
  - Tile-swap shape and ID cycling.
  - Tile-swap preview, outlines, and tooltip.
- `GLCanvasTileDoorEditorSupport.h`
  - Private shared geometry, hit-test, preview, and overlay drawing helpers.

### `GLCanvasObjectCoordinator`

Handles cross-object operations:

- Delete selected object.
- Reorder selected object.
- Select next object.
- Select next tile-swap region.
- Nudge selected object.

### `GLCanvasObjectSupport`

Contains shared object geometry and conversion helpers:

- Tile-swap region geometry construction and metrics.
- Door geometry construction (with optional preview-map override).
- Shared `PickRect`/bounds helpers for projected geometry.
- Warp instance construction and valid-size clamping.
- Geometric entity sorting.

## Shared Support Types

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

Provides the bitmap font used for GPU overlay text, labels, and tooltips,
plus the shared `DrawOverlayGlyph`/`DrawOverlayText` helpers used by all
overlay/tooltip renderers.

## Frame Integration Events

`MyGLCanvas` posts wx events back to `RoomViewerFrame` so the existing panes and
properties stay in sync.

- `EVT_GPU_EDITOR_MODE_CHANGE`
  - GPU edit mode or drawing tool changed.
  - The frame updates toolbar checks, menu checks, and control availability.
- `EVT_GPU_LAYER_OPACITY_CHANGE`
  - Keyboard opacity shortcuts changed GPU layer opacity.
  - The frame updates the sliders in the Layers pane.
- `EVT_GPU_LAYER_BLOCK_SELECT`
  - A background/foreground layer cell was selected in the GPU editor.
  - The frame selects the corresponding block in `BlocksetEditorCtrl`.
- `EVT_GPU_HEIGHTMAP_TARGET_CHANGE`
  - Heightmap selection or target cell changed.
  - The frame updates heightmap editing controls.
- Existing selection events:
  - `EVT_ENTITY_SELECT`
  - `EVT_WARP_SELECT`
  - `EVT_TILESWAP_SELECT`
  - `EVT_DOOR_SELECT`
- Existing update events:
  - `EVT_ENTITY_UPDATE`
  - `EVT_WARP_UPDATE`
  - `EVT_TILESWAP_UPDATE`
  - `EVT_DOOR_UPDATE`
- Navigation/properties:
  - Room navigation posts `EVT_GO_TO_NAV_ITEM` and property refresh events so
    the navigation pane and property grid follow GPU-driven room changes.

## Design Notes

The current design deliberately keeps `MyGLCanvas` as the central coordination
object. Mode and object helpers are organizational adapters over shared canvas
state rather than independent controllers. This made the GPU editor port
practical and keeps the existing frame/pane contracts stable.

The main tradeoff is that `MyGLCanvas` remains state-heavy and friend-heavy.
Future refactors could move pending-add state, drag state, layer edit state, and
heightmap edit state into explicit state/model objects so helper classes can
depend on smaller contracts.
