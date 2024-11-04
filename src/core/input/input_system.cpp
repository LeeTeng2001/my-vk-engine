#include <algorithm>
#include <cstring>
#include <SDL3/SDL.h>

#include "input_system.hpp"

namespace luna {

bool KeyboardState::getKeyValue(SDL_Scancode keyCode) const { return _curState[keyCode] == 1; }

ButtonState KeyboardState::getKeyState(SDL_Scancode keyCode) const {
    if (_prevState[keyCode] == 0) {
        // Prev is up
        return _curState[keyCode] == 0 ? ENone : EPressed;
    } else {
        // Prev is down
        return _curState[keyCode] == 0 ? EReleased : EHeld;
    }
}

bool MouseState::getButtonValue(int button) const { return (SDL_BUTTON(button) & _curButtons); }

ButtonState MouseState::getButtonState(int button) const {
    int mask = SDL_BUTTON(button);

    if ((mask & _prevButtons) == 0) {
        return (mask & _curButtons) == 0 ? ENone : EPressed;
    } else {
        return (mask & _curButtons) == 0 ? EReleased : EHeld;
    }
}

bool ControllerState::getButtonValue(SDL_GamepadButton button) const {
    return _curButtons[button] == 1;
}

ButtonState ControllerState::getButtonState(SDL_GamepadButton button) const {
    if (_prevButtons[button] == 0) {
        return _curButtons[button] == 0 ? ENone : EPressed;
    } else {
        return _curButtons[button] == 0 ? EReleased : EHeld;
    }
}

bool InputSystem::initialise() {
    // Keyboard
    // Assign current state pointer
    _inputState.Keyboard._curState = SDL_GetKeyboardState(nullptr);
    // Clear previous state memory
    memset(_inputState.Keyboard._prevState, 0, SDL_NUM_SCANCODES);

    // Mouse (just set everything to 0)
    _inputState.Mouse._curButtons = 0;
    _inputState.Mouse._prevButtons = 0;

    // Get the connected controller, if it exists (return non nullptr)
    // TODO: We're hardcoding the first controller
    _controller = SDL_OpenGamepad(0);
    // Initialize controller state
    _inputState.Controller._isConnected = (_controller != nullptr);
    memset(_inputState.Controller._curButtons, 0, SDL_GAMEPAD_BUTTON_MAX);
    memset(_inputState.Controller._prevButtons, 0, SDL_GAMEPAD_BUTTON_MAX);

    return true;
}

void InputSystem::shutdown() { SDL_CloseGamepad(_controller); }

void InputSystem::prepareForUpdate() {
    // Copy current state to previous (because SDL overwrite its original key buffer)
    // Keyboard + Mouse + controller
    memcpy(_inputState.Keyboard._prevState, _inputState.Keyboard._curState, SDL_NUM_SCANCODES);

    // mState.Mouse.mIsRelative = false;  // TODO: Bug reset?
    _inputState.Mouse._prevButtons = _inputState.Mouse._curButtons;
    _inputState.Mouse._scrollWheel = {0,
                                      0};  // only triggers on frames where the scroll wheel moves

    memcpy(_inputState.Controller._prevButtons, _inputState.Controller._curButtons,
           SDL_GAMEPAD_BUTTON_MAX);
}

void InputSystem::update() {
    // Mouse
    float x = 0, y = 0;
    _inputState.Mouse._curButtons = _inputState.Mouse._isRelative
                                        ? SDL_GetRelativeMouseState(&x, &y)
                                        : SDL_GetMouseState(&x, &y);
    _inputState.Mouse._mouseOffsetPos.x =
        _inputState.Mouse._isRelative ? x : x - _inputState.Mouse._mousePos.x;
    _inputState.Mouse._mouseOffsetPos.y =
        _inputState.Mouse._isRelative ? y : y - _inputState.Mouse._mousePos.y;
    _inputState.Mouse._mousePos.x = x;
    _inputState.Mouse._mousePos.y = y;

    // Controller --------------------------------------------------------
    // Buttons
    for (int i = 0; i < SDL_GAMEPAD_BUTTON_MAX; i++) {
        _inputState.Controller._curButtons[i] =
            SDL_GetGamepadButton(_controller, SDL_GamepadButton(i));
    }

    // Triggers
    _inputState.Controller._leftTrigger =
        filter1D(SDL_GetGamepadAxis(_controller, SDL_GAMEPAD_AXIS_LEFT_TRIGGER));
    _inputState.Controller._rightTrigger =
        filter1D(SDL_GetGamepadAxis(_controller, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER));

    // Sticks, negates y because SDL report y axis in down positive
    x = SDL_GetGamepadAxis(_controller, SDL_GAMEPAD_AXIS_LEFTX);
    y = -SDL_GetGamepadAxis(_controller, SDL_GAMEPAD_AXIS_LEFTY);
    _inputState.Controller._leftStick = filter2D(x, y);

    x = SDL_GetGamepadAxis(_controller, SDL_GAMEPAD_AXIS_RIGHTX);
    y = -SDL_GetGamepadAxis(_controller, SDL_GAMEPAD_AXIS_RIGHTY);
    _inputState.Controller._rightStick = filter2D(x, y);
}

void InputSystem::processEvent(union SDL_Event &event) {
    switch (event.type) {
        case SDL_EVENT_MOUSE_WHEEL:
            _inputState.Mouse._scrollWheel = {event.wheel.x, event.wheel.y};
            break;
        default:
            break;
    }
}

void InputSystem::setRelativeMouseMode(bool value) {
    SDL_SetRelativeMouseMode(value);
    if (value) SDL_GetRelativeMouseState(nullptr, nullptr);  // clear out for relative
    _inputState.Mouse._isRelative = value;
}

float InputSystem::filter1D(float input) {
    // Could make this parameter user configurable
    // A value < dead zone is interpreted as 0%
    // A value > max value is interpreted as 100%
    constexpr float deadZone = 250;
    constexpr float maxValue = 30000;

    float retVal = 0.0f;

    // Take absolute value of input
    float absValue = glm::abs(input);
    // Ignore input within dead zone
    if (absValue > deadZone) {
        retVal = static_cast<float>(absValue - deadZone) / (maxValue - deadZone);
        retVal = input > 0 ? retVal : -1.0f * retVal;  // make sure sign
        retVal = std::clamp(retVal, -1.0f, 1.0f);      // clamp to [-1, 1]
    }

    return retVal;
}

glm::vec2 InputSystem::filter2D(float inputX, float inputY) {
    const float deadZone = 8000.0f;
    const float maxValue = 30000.0f;

    // Make into 2D vector
    glm::vec2 dir{inputX, inputY};
    float length = glm::length(dir);

    // If length < deadZone, should be no input
    if (length < deadZone) {
        dir = {0, 0};
    } else {
        // Calculate fractional value between
        // dead zone and max value circles
        float f = (length - deadZone) / (maxValue - deadZone);
        // Clamp f between 0.0f and 1.0f
        f = std::clamp(f, 0.0f, 1.0f);
        // Normalize the vector, and then scale it to the
        // fractional value
        dir *= f / length;
    }

    return dir;
}

}  // namespace luna