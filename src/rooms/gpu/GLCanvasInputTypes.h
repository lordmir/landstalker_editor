#ifndef GL_CANVAS_INPUT_TYPES_H
#define GL_CANVAS_INPUT_TYPES_H

// Selects how WASD maps onto the isometric room axes. Arrow keys always remain
// screen-space camera controls.
enum class GLCanvasDirectionInputMode {
    UpIsNorthEast,
    UpIsNorthWest,
    Landstalker,
    DiagonalChords
};

#endif  // GL_CANVAS_INPUT_TYPES_H
