#ifndef GL_CANVAS_KEYBOARD_INPUT_H
#define GL_CANVAS_KEYBOARD_INPUT_H

#include "GLCanvasInputTypes.h"

class GLCanvas;
class wxKeyEvent;

// Owns top-level keyboard policy and dispatches commands to the active editor mode.
// This persistent controller is the integration point for customizable key bindings.
class GLCanvasKeyboardInput {
public:
    explicit GLCanvasKeyboardInput(GLCanvas& canvas);

    // Handles global shortcuts before delegating mode-specific input.
    bool HandleKeyDown(wxKeyEvent& evt);
    bool HandleKeyUp(wxKeyEvent& evt);
    void ResetDirectionState();
    void SetDirectionInputMode(GLCanvasDirectionInputMode mode);
    GLCanvasDirectionInputMode GetDirectionInputMode() const;
    void CycleDirectionInputMode();

private:
    enum class DirectionKeySet {
        None,
        Wasd
    };

    enum class IsometricDirection {
        NorthEast,
        SouthEast,
        SouthWest,
        NorthWest
    };

    enum class DirectionAxis {
        NorthEastSouthWest,
        NorthWestSouthEast
    };

    struct DirectionKey {
        DirectionKeySet set = DirectionKeySet::None;
        unsigned int mask = 0;
        bool uppercase = false;
    };

    static DirectionKey ClassifyDirectionKey(int key_code);
    static bool ResolveDiagonalChord(unsigned int mask, IsometricDirection& direction);
    static bool ResolveLandstalkerDirection(unsigned int mask, DirectionAxis last_axis,
                                            IsometricDirection& direction);
    static int MappedKeyCode(IsometricDirection direction, const DirectionKey& key);
    bool MapDirectionalInput(const wxKeyEvent& evt, const DirectionKey& key, wxKeyEvent& mapped_evt);
    GLCanvas& m_canvas;
    GLCanvasDirectionInputMode m_direction_input_mode = GLCanvasDirectionInputMode::UpIsNorthEast;
    DirectionAxis m_last_direction_axis = DirectionAxis::NorthEastSouthWest;
    unsigned int m_pressed_wasd_directions = 0;
};

#endif  // GL_CANVAS_KEYBOARD_INPUT_H
