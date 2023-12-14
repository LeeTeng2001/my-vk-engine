#include "input_system.hpp"
#include <SDL.h>
#include <algorithm>
#include <cstring>

bool KeyboardState::getKeyValue(SDL_Scancode keyCode) const {
	return _curState[keyCode] == 1;
}

ButtonState KeyboardState::getKeyState(SDL_Scancode keyCode) const {
    if (_prevState[keyCode] == 0) {
        // Prev is up
        return _curState[keyCode] == 0 ? ENone : EPressed;
    } else {
        // Prev is down
        return _curState[keyCode] == 0 ? EReleased : EHeld;
    }
}

bool MouseState::getButtonValue(int button) const {
    return (SDL_BUTTON(button) & _curButtons);
}

ButtonState MouseState::getButtonState(int button) const {
    int mask = SDL_BUTTON(button);

    if ((mask & _prevButtons) == 0) {
        return (mask & _curButtons) == 0 ? ENone : EPressed;
    } else {
        return (mask & _curButtons) == 0 ? EReleased : EHeld;
    }
}

bool ControllerState::GetButtonValue(SDL_GameControllerButton button) const {
    return mCurrButtons[button] == 1;
}

ButtonState ControllerState::GetButtonState(SDL_GameControllerButton button) const {
    if (mPrevButtons[button] == 0) {
        return mCurrButtons[button] == 0 ? ENone : EPressed;
    } else {
        return mCurrButtons[button] == 0 ? EReleased : EHeld;
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
    _inputState.Controller.mIsConnected = (_controller != nullptr);
    memset(_inputState.Controller.mCurrButtons, 0, SDL_CONTROLLER_BUTTON_MAX);
    memset(_inputState.Controller.mPrevButtons, 0, SDL_CONTROLLER_BUTTON_MAX);

    return true;
}

void InputSystem::shutdown() {
    SDL_CloseGamepad(_controller);
}

void InputSystem::PrepareForUpdate() {
    // Copy current state to previous (because SDL overwrite its original key buffer)
    // Keyboard
    memcpy(_inputState.Keyboard._prevState, _inputState.Keyboard._curState, SDL_NUM_SCANCODES);

    // Mouse
    // mState.Mouse.mIsRelative = false;  // TODO: Bug reset?
    _inputState.Mouse._prevButtons = _inputState.Mouse._curButtons;
    _inputState.Mouse._scrollWheel = Vector2::Zero;  // only triggers on frames where the scroll wheel moves

    // Controller
    memcpy(_inputState.Controller.mPrevButtons, _inputState.Controller.mCurrButtons, SDL_CONTROLLER_BUTTON_MAX);
}

void InputSystem::Update() {
    // Mouse
    int x = 0, y = 0;
    if (_inputState.Mouse._isRelative) {
        _inputState.Mouse._curButtons = SDL_GetRelativeMouseState(&x, &y);
    } else {
        _inputState.Mouse._curButtons = SDL_GetMouseState(&x, &y);
    }

    _inputState.Mouse._mousePos.x = static_cast<float>(x);
    _inputState.Mouse._mousePos.y = static_cast<float>(y);

    // Controller --------------------------------------------------------
    // Buttons
    for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++) {
        _inputState.Controller.mCurrButtons[i] =
                SDL_GameControllerGetButton(_controller,
                                            SDL_GameControllerButton(i));
    }

    // Triggers
    _inputState.Controller.mLeftTrigger =
            Filter1D(SDL_GameControllerGetAxis(_controller,
                                               SDL_CONTROLLER_AXIS_TRIGGERLEFT));
    _inputState.Controller.mRightTrigger =
            Filter1D(SDL_GameControllerGetAxis(_controller,
                                               SDL_CONTROLLER_AXIS_TRIGGERRIGHT));

    // Sticks, negates y because SDL report y axis in down positive
    x = SDL_GameControllerGetAxis(_controller,
                                  SDL_CONTROLLER_AXIS_LEFTX);
    y = -SDL_GameControllerGetAxis(_controller,
                                   SDL_CONTROLLER_AXIS_LEFTY);
    _inputState.Controller.mLeftStick = Filter2D(x, y);

    x = SDL_GameControllerGetAxis(_controller,
                                  SDL_CONTROLLER_AXIS_RIGHTX);
    y = -SDL_GameControllerGetAxis(_controller,
                                   SDL_CONTROLLER_AXIS_RIGHTY);
    _inputState.Controller.mRightStick = Filter2D(x, y);
}

void InputSystem::ProcessEvent(SDL_Event &event) {
    switch (event.type) {
        case SDL_MOUSEWHEEL:
            _inputState.Mouse._scrollWheel = Vector2(
                    static_cast<float>(event.wheel.x),
                    static_cast<float>(event.wheel.y));
            break;
        default:
            break;
    }
}

void InputSystem::setRelativeMouseMode(bool value) {
    SDL_bool set = value ? SDL_TRUE : SDL_FALSE;
    SDL_SetRelativeMouseMode(set);
    if (value) SDL_GetRelativeMouseState(nullptr, nullptr);  // clear out for relative
    _inputState.Mouse._isRelative = value;
}

float InputSystem::Filter1D(int input) {
    // Could make this parameter user configurable
    // A value < dead zone is interpreted as 0%
    const int deadZone = 250;
    // A value > max value is interpreted as 100%
    const int maxValue = 30000;

    float retVal = 0.0f;

    // Take absolute value of input
    int absValue = std::abs(input);
    // Ignore input within dead zone
    if (absValue > deadZone) {
        // Compute fractional value between dead zone and max value
        retVal = static_cast<float>(absValue - deadZone) / (maxValue - deadZone);
        // Make sure sign matches original value
        retVal = input > 0 ? retVal : -1.0f * retVal;
        // Clamp between -1.0f and 1.0f
        retVal = std::clamp(retVal, -1.0f, 1.0f);
    }

    return retVal;
}

Vector2 InputSystem::Filter2D(int inputX, int inputY) {
    const float deadZone = 8000.0f;
    const float maxValue = 30000.0f;

    // Make into 2D vector
    Vector2 dir{};
    dir.x = static_cast<float>(inputX);
    dir.y = static_cast<float>(inputY);

    float length = dir.Length();

    // If length < deadZone, should be no input
    if (length < deadZone) {
        dir = Vector2::Zero;
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
