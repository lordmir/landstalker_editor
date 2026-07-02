# GPU Room Editor User Guide

This guide covers the GPU room editor canvas controls. Shortcuts apply while
the GPU view has focus.

## Modes

| Key | Mode |
| --- | --- |
| `1` | Room/object mode |
| `2` | Heightmap mode |
| `3` | Background layer mode |
| `4` | Foreground layer mode |

## Global Controls

| Input | Action |
| --- | --- |
| `Ctrl+Z` | Undo |
| `Ctrl+Y` | Redo |
| `Ctrl+Mouse Wheel` | Zoom around the mouse cursor |
| `Ctrl++` / `Ctrl+=` | Zoom in around the window center |
| `Ctrl+-` | Zoom out around the window center |
| Mouse wheel | Pan vertically |
| Horizontal mouse wheel | Pan horizontally |
| Arrow keys | Pan the camera |
| `Esc` | Cancel current drag/tool operation, return to Select, or clear selection depending on mode |

## Drawing Tools

The drawing tools are available from the Drawing Tools toolbar in layer and
heightmap modes.

| Tool | Layer mode behavior | Heightmap mode behavior |
| --- | --- | --- |
| Select | Select cells, move selected cells | Select cells, move selected cells |
| Draw | Paint the copied block/cell while dragging | Paint the copied heightmap cell while dragging |
| Line | Draw a line on mouse release | Draw a line on mouse release |
| Filled Rect | Draw a filled rectangle | Draw a filled rectangle |
| Outline Rect | Draw an outline rectangle | Draw an outline rectangle |
| Filled Circle | Draw a filled circle | Draw a filled circle |
| Outline Circle | Draw an outline circle | Draw an outline circle |
| Flood Fill | Flood fill from clicked cell | Flood fill from clicked cell |
| Stamp | Stamp the current selection pattern | Stamp the current selection pattern |
| Clear | Clear selected layer cells, or selected heightmap cells |

For shape tools, hold `Shift` while dragging to constrain the shape. In layer
mode, `Alt` changes line/shape snapping for skewed room-grid geometry.

## Room Mode

Room mode edits objects: entities, warps, tile swaps, and doors.

### Mouse

| Input | Action |
| --- | --- |
| Left click object | Select and start dragging it |
| Left click selected warp resize handle | Resize selected warp |
| Left click selected tile-swap resize handle | Resize selected tile-swap region |
| Left click empty space and drag | Pan camera |
| Left double-click object | Open selected object properties |
| Right click warp | Navigate to the warp destination room |
| Right click entity | Drag entity on Z axis |
| Right click tile-swap region | Toggle tile-swap preview |
| Right click door | Toggle door preview |
| Click room-info destination link | Navigate to linked room |
| Pending add + left click | Commit/place pending object |
| Pending add + right click or `Esc` | Cancel pending object |

Hold `Ctrl` while dragging an entity to drag it on the Z axis.

### Object Shortcuts

| Key | Action |
| --- | --- |
| `Enter` | Open selected object properties |
| `Tab` | Select next object |
| `Ctrl+Tab` | Select previous object |
| `Insert` | Begin adding an entity |
| `Shift+Insert` | Begin adding a warp half |
| `Ctrl+Insert` | Begin adding a tile swap |
| `Alt+Insert` | Begin adding a door |
| `Delete` | Delete selected object |
| `Ctrl+C` | Copy selected entity |
| `Ctrl+V` | Paste copied entity |
| `Ctrl+X` | Cut selected entity |
| `[` / `{` | Reorder selected object earlier, or decrement selected tile-swap ID |
| `]` / `}` | Reorder selected object later, or increment selected tile-swap ID |
| `,` / `<` | Previous entity ID, warp type, door size, or tile-swap shape |
| `.` / `>` | Next entity ID, warp type, door size, or tile-swap shape |
| `P` | Cycle selected or pending entity palette |
| `Ctrl+F` | Put selected entity on the floor |

### Movement and Object Adjustment

| Key | Action |
| --- | --- |
| `W` / `A` / `S` / `D` | Nudge selected object north-west/south-west/south-east/north-east on the room grid |
| `R` | Raise selected object by 0.5 |
| `F` | Lower selected object by 0.5 |
| `Ctrl+W` / `Ctrl+A` / `Ctrl+S` / `Ctrl+D` on entity | Set entity orientation to NW/SW/SE/NE |
| `Shift+W` / `Shift+A` / `Shift+S` / `Shift+D` on warp | Resize selected or pending warp |
| `Ctrl+W` / `Ctrl+A` / `Ctrl+S` / `Ctrl+D` on warp | Rotate selected warp direction/shape |
| `Shift+W` / `Shift+A` / `Shift+S` / `Shift+D` on tile swap | Resize selected tile-swap region |

### Room View Toggles

| Key | Action |
| --- | --- |
| `PageUp` | Next room |
| `PageDown` | Previous room |
| `Space` | Toggle selected tile-swap preview, or selected door preview |
| `B` | Cycle background opacity |
| `G` | Cycle foreground opacity |
| `E` | Cycle entity/sprite opacity |
| `X` | Toggle entity hitboxes |
| `H` | Toggle heightmap display in room mode |
| `Z` | Cycle entity occlusion mode |
| `+` / `=` | Increase room heightmap Z extent |
| `-` | Decrease room heightmap Z extent |

## Layer Modes

Background and foreground layer modes edit tilemap cells.

### Mouse

| Input | Action |
| --- | --- |
| Left drag with Select | Select a rectangular/parallelogram cell region |
| `Shift` + left drag | Add to selection |
| `Ctrl` + left drag | Subtract from selection |
| `Alt` + left drag | Parallelogram selection |
| Left drag selected cells | Move selected cells |
| Right click cell | Copy block at cell |
| Right click while drawing/dragging | Cancel or finish current operation |

### Keys

| Key | Action |
| --- | --- |
| `I` | Toggle block ID labels |
| `H` | Toggle heightmap overlay |
| `B` in foreground mode | Toggle background underlay |
| `C` | Copy selected block |
| `Space` | Paste copied block |
| `Delete` | Clear selected layer cells |
| `,` / `<` | Decrement selected block ID by 1 |
| `Shift+,` / `Shift+<` | Decrement selected block ID by 16 |
| `Ctrl+,` / `Ctrl+<` | Decrement selected block ID by 256 |
| `.` / `>` | Increment selected block ID by 1 |
| `Shift+.` / `Shift+>` | Increment selected block ID by 16 |
| `Ctrl+.` / `Ctrl+>` | Increment selected block ID by 256 |
| `W` / `A` / `S` / `D` | Move selected layer cell/selection |

### Toolbar/Menu Commands

| Command | Action |
| --- | --- |
| Clear Tilemap | Clear current tilemap |
| Insert Row Before / After | Insert tilemap row relative to selection |
| Delete Row | Delete selected tilemap row |
| Insert Column Before / After | Insert tilemap column relative to selection |
| Delete Column | Delete selected tilemap column |
| Toggle Priority Highlight | Toggle priority-bit highlight overlay |

## Heightmap Mode

Heightmap mode edits heightmap cells, heightmap types, heights, and movement
restriction bits.

### Mouse

| Input | Action |
| --- | --- |
| Left drag with Select | Select heightmap cells |
| `Shift` + left drag | Add to selection |
| `Ctrl` + left drag | Subtract from selection |
| Left drag selected cells | Move selected cells |
| Right click cell | Copy heightmap cell |
| Right click while drawing/dragging | Cancel current line/move operation |

### Keys

| Key | Action |
| --- | --- |
| `Shift+1` | Flat heightmap view |
| `Shift+2` | Raised heightmap view |
| `Shift+3` | Full heightmap view |
| `Shift+4` | Full heightmap plus tilemap view |
| `B` | Toggle tilemap underlay |
| `[` / `{` | Decrement selected heightmap type |
| `]` / `}` | Increment selected heightmap type |
| `,` / `<` | Decrement selected restriction nibble |
| `.` / `>` | Increment selected restriction nibble |
| `R` | Cycle selected restriction backward through common passability states |
| `F` | Cycle selected restriction forward through common passability states |
| `PageUp` | Increase selected height |
| `PageDown` | Decrease selected height |
| `Delete` | Clear selected heightmap cells |
| `C` | Copy selected heightmap cell |
| `Space` | Paste copied heightmap cell |
| `W` / `A` / `S` / `D` | Move selected heightmap cell/selection |
| `+` / `=` | Increase heightmap editor Z scale |
| `-` | Decrease heightmap editor Z scale |

### Toolbar/Menu Commands

| Command | Action |
| --- | --- |
| Insert Row Before / After | Insert heightmap row relative to selection |
| Delete Row | Delete selected heightmap row |
| Insert Column Before / After | Insert heightmap column relative to selection |
| Delete Column | Delete selected heightmap column |
| Toggle Player Passable | Toggle player passability on selected cells |
| Toggle NPC Passable | Toggle NPC passability on selected cells |
| Toggle Raft Track | Toggle raft-track flag on selected cells |
| Increase / Decrease Height | Adjust selected heightmap height |
| Nudge Heightmap NE/NW/SE/SW | Move the whole heightmap bounds |

## Object Pane Commands

The entity, warp, tile-swap, and door panes still expose their normal Add,
Delete, and property commands. In GPU view, those commands call the same canvas
operations as the shortcuts:

- Add Entity -> `Insert`
- Add Warp Half -> `Shift+Insert`
- Add Tile Swap -> `Ctrl+Insert`
- Add Door -> `Alt+Insert`
- Delete selected object -> `Delete`
- Open selected properties -> `Enter` or double-click an object

## Notes

- Many commands operate on the current selection. If nothing is selected, they
  may do nothing.
- Pending object additions use a crosshair cursor. Place with left click,
  cancel with right click or `Esc`.
- Layer and heightmap edit modes share the drawing toolbar, but the current
  clipboard source is mode-specific.
