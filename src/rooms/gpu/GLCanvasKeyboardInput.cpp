#include "GLCanvasKeyboardInput.h"

#include "GLCanvas.h"
#include "GLCanvasHeightmapMode.h"
#include "GLCanvasLayerEditMode.h"
#include "GLCanvasRoomMode.h"

namespace {
constexpr unsigned int kDirectionUp = 1U;
constexpr unsigned int kDirectionDown = 2U;
constexpr unsigned int kDirectionLeft = 4U;
constexpr unsigned int kDirectionRight = 8U;
}

GLCanvasKeyboardInput::GLCanvasKeyboardInput(GLCanvas& canvas)
    : m_canvas(canvas)
{
}

void GLCanvasKeyboardInput::SetDirectionInputMode(GLCanvasDirectionInputMode mode)
{
    ResetDirectionState();
    m_direction_input_mode = mode;
}

GLCanvasDirectionInputMode GLCanvasKeyboardInput::GetDirectionInputMode() const
{
    return m_direction_input_mode;
}

void GLCanvasKeyboardInput::CycleDirectionInputMode()
{
    ResetDirectionState();
    switch (m_direction_input_mode) {
    case GLCanvasDirectionInputMode::UpIsNorthEast:
        m_direction_input_mode = GLCanvasDirectionInputMode::UpIsNorthWest;
        break;
    case GLCanvasDirectionInputMode::UpIsNorthWest:
        m_direction_input_mode = GLCanvasDirectionInputMode::Landstalker;
        break;
    case GLCanvasDirectionInputMode::Landstalker:
        m_direction_input_mode = GLCanvasDirectionInputMode::DiagonalChords;
        break;
    case GLCanvasDirectionInputMode::DiagonalChords:
        m_direction_input_mode = GLCanvasDirectionInputMode::UpIsNorthEast;
        break;
    }
}

GLCanvasKeyboardInput::DirectionKey GLCanvasKeyboardInput::ClassifyDirectionKey(int key_code)
{
    switch (key_code) {
    case 'w': return {DirectionKeySet::Wasd, kDirectionUp, false};
    case 'W': return {DirectionKeySet::Wasd, kDirectionUp, true};
    case 's': return {DirectionKeySet::Wasd, kDirectionDown, false};
    case 'S': return {DirectionKeySet::Wasd, kDirectionDown, true};
    case 'a': return {DirectionKeySet::Wasd, kDirectionLeft, false};
    case 'A': return {DirectionKeySet::Wasd, kDirectionLeft, true};
    case 'd': return {DirectionKeySet::Wasd, kDirectionRight, false};
    case 'D': return {DirectionKeySet::Wasd, kDirectionRight, true};
    default: return {};
    }
}

bool GLCanvasKeyboardInput::ResolveDiagonalChord(unsigned int mask, IsometricDirection& direction)
{
    switch (mask) {
    case kDirectionUp | kDirectionRight:
        direction = IsometricDirection::NorthEast;
        return true;
    case kDirectionDown | kDirectionRight:
        direction = IsometricDirection::SouthEast;
        return true;
    case kDirectionDown | kDirectionLeft:
        direction = IsometricDirection::SouthWest;
        return true;
    case kDirectionUp | kDirectionLeft:
        direction = IsometricDirection::NorthWest;
        return true;
    default:
        return false;
    }
}

bool GLCanvasKeyboardInput::ResolveLandstalkerDirection(unsigned int mask, DirectionAxis last_axis,
                                                        IsometricDirection& direction)
{
    if (ResolveDiagonalChord(mask, direction)) {
        return true;
    }
    switch (mask) {
    case kDirectionUp:
        direction = last_axis == DirectionAxis::NorthEastSouthWest
            ? IsometricDirection::NorthEast : IsometricDirection::NorthWest;
        return true;
    case kDirectionRight:
        direction = last_axis == DirectionAxis::NorthEastSouthWest
            ? IsometricDirection::NorthEast : IsometricDirection::SouthEast;
        return true;
    case kDirectionDown:
        direction = last_axis == DirectionAxis::NorthEastSouthWest
            ? IsometricDirection::SouthWest : IsometricDirection::SouthEast;
        return true;
    case kDirectionLeft:
        direction = last_axis == DirectionAxis::NorthEastSouthWest
            ? IsometricDirection::SouthWest : IsometricDirection::NorthWest;
        return true;
    default:
        // Like HandleDirectionalControl, opposite pairs and inputs with more
        // than two directions do not produce movement.
        return false;
    }
}

int GLCanvasKeyboardInput::MappedKeyCode(IsometricDirection direction, const DirectionKey& key)
{
    int mapped = 0;
    switch (direction) {
    case IsometricDirection::NorthEast: mapped = 'w'; break;
    case IsometricDirection::SouthEast: mapped = 'd'; break;
    case IsometricDirection::SouthWest: mapped = 's'; break;
    case IsometricDirection::NorthWest: mapped = 'a'; break;
    }
    return key.uppercase ? mapped - 'a' + 'A' : mapped;
}

bool GLCanvasKeyboardInput::MapDirectionalInput(const wxKeyEvent& evt, const DirectionKey& key,
                                                wxKeyEvent& mapped_evt)
{
    IsometricDirection direction = IsometricDirection::NorthEast;
    if (m_direction_input_mode == GLCanvasDirectionInputMode::Landstalker ||
        m_direction_input_mode == GLCanvasDirectionInputMode::DiagonalChords) {
        unsigned int mask = key.mask;
        if (evt.GetEventType() == wxEVT_KEY_DOWN) {
            m_pressed_wasd_directions |= key.mask;
            mask = m_pressed_wasd_directions;
        }
        const bool resolved = m_direction_input_mode == GLCanvasDirectionInputMode::Landstalker
            ? ResolveLandstalkerDirection(mask, m_last_direction_axis, direction)
            : ResolveDiagonalChord(mask, direction);
        if (!resolved) {
            return false;
        }
    } else if (m_direction_input_mode == GLCanvasDirectionInputMode::UpIsNorthEast) {
        switch (key.mask) {
        case kDirectionUp: direction = IsometricDirection::NorthEast; break;
        case kDirectionRight: direction = IsometricDirection::SouthEast; break;
        case kDirectionDown: direction = IsometricDirection::SouthWest; break;
        case kDirectionLeft: direction = IsometricDirection::NorthWest; break;
        }
    } else {
        switch (key.mask) {
        case kDirectionUp: direction = IsometricDirection::NorthWest; break;
        case kDirectionRight: direction = IsometricDirection::NorthEast; break;
        case kDirectionDown: direction = IsometricDirection::SouthEast; break;
        case kDirectionLeft: direction = IsometricDirection::SouthWest; break;
        }
    }

    m_last_direction_axis = direction == IsometricDirection::NorthEast ||
                            direction == IsometricDirection::SouthWest
        ? DirectionAxis::NorthEastSouthWest : DirectionAxis::NorthWestSouthEast;

    const int mapped_key = MappedKeyCode(direction, key);
    mapped_evt.m_keyCode = mapped_key;
    mapped_evt.m_uniChar = mapped_key;
    return true;
}

void GLCanvasKeyboardInput::ResetDirectionState()
{
    m_pressed_wasd_directions = 0;
}

bool GLCanvasKeyboardInput::HandleKeyDown(wxKeyEvent& evt)
{
    if (evt.ControlDown() && !evt.AltDown()) {
        int key = evt.GetKeyCode();
        if (key == 'Z') {
            m_canvas.Undo();
            m_canvas.UpdateStatusBar();
            return true;
        }
        if (key == 'Y') {
            m_canvas.Redo();
            m_canvas.UpdateStatusBar();
            return true;
        }
    }

    wxKeyEvent mapped_evt(evt);
    const DirectionKey direction_key = ClassifyDirectionKey(evt.GetKeyCode());
    if (direction_key.set != DirectionKeySet::None &&
        !MapDirectionalInput(evt, direction_key, mapped_evt)) {
        m_canvas.UpdateStatusBar();
        return true;
    }
    bool handled = false;
    if (m_canvas.IsHeightmapEditMode()) {
        handled = GLCanvasHeightmapMode(m_canvas).HandleKeyDown(mapped_evt);
    } else if (m_canvas.IsLayerEditMode()) {
        handled = GLCanvasLayerEditMode(m_canvas).HandleKeyDown(mapped_evt);
    } else {
        handled = GLCanvasRoomMode(m_canvas).HandleKeyDown(mapped_evt);
    }
    m_canvas.UpdateStatusBar();
    return handled;
}

bool GLCanvasKeyboardInput::HandleKeyUp(wxKeyEvent& evt)
{
    const DirectionKey key = ClassifyDirectionKey(evt.GetKeyCode());
    if (key.set == DirectionKeySet::None) {
        return false;
    }
    m_pressed_wasd_directions &= ~key.mask;
    return true;
}

bool GLCanvas::HandleKeyDown(wxKeyEvent& evt)
{
    return m_keyboard_input->HandleKeyDown(evt);
}

bool GLCanvas::HandleKeyUp(wxKeyEvent& evt)
{
    return m_keyboard_input->HandleKeyUp(evt);
}

void GLCanvas::SetDirectionInputMode(GLCanvasDirectionInputMode mode)
{
    m_keyboard_input->SetDirectionInputMode(mode);
}

GLCanvasDirectionInputMode GLCanvas::GetDirectionInputMode() const
{
    return m_keyboard_input->GetDirectionInputMode();
}

void GLCanvas::CycleDirectionInputMode()
{
    m_keyboard_input->CycleDirectionInputMode();
}

void GLCanvas::OnKeyDown(wxKeyEvent& evt)
{
    evt.Skip(!m_keyboard_input->HandleKeyDown(evt));
}

void GLCanvas::OnKeyUp(wxKeyEvent& evt)
{
    evt.Skip(!m_keyboard_input->HandleKeyUp(evt));
}

void GLCanvas::OnKillFocus(wxFocusEvent& evt)
{
    m_keyboard_input->ResetDirectionState();
    evt.Skip();
}
