#pragma once

#include <SDL_scancode.h>
#include <SDL_gamepad.h>
#include <SDL_mouse.h>

#include "utils/common.hpp"
#include "SDL_oldnames.h"

enum ButtonState {
	ENone,
	EPressed,
	EReleased,
	EHeld
};

class KeyboardState {
public:
	// Friend so InputSystem can easily update it
	friend class InputSystem;

	// Get just the boolean true/false value of key
	[[nodiscard]] bool getKeyValue(SDL_Scancode keyCode) const;
	// Get a state based on current and previous frame
	[[nodiscard]] ButtonState getKeyState(SDL_Scancode keyCode) const;

private:
	const Uint8* _curState;
	Uint8 _prevState[SDL_NUM_SCANCODES];  // buffer to save state before update
};

// Helper for mouse input
class MouseState {
public:
	friend class InputSystem;

	// For buttons
	[[nodiscard]] bool getButtonValue(int button) const;
	[[nodiscard]] ButtonState getButtonState(int button) const;

	// For mouse specific
	[[nodiscard]] const glm::vec2& getPosition() const { return _mousePos; }
	[[nodiscard]] const glm::vec2& getScrollWheel() const { return _scrollWheel; }
	[[nodiscard]] bool isRelative() const { return _isRelative; }

private:
	// mouse button, position, scroll wheel
	Uint32 _curButtons;
	Uint32 _prevButtons;
	glm::vec2 _mousePos;
    glm::vec2 _scrollWheel;
	// Are we in relative mouse mode
	bool _isRelative;
};

// Helper for controller input
class ControllerState {
public:
	friend class InputSystem;

	// For buttons
	[[nodiscard]] bool getButtonValue(SDL_GamepadButton button) const;
	[[nodiscard]] ButtonState getButtonState(SDL_GamepadButton button) const;

    // getter
	[[nodiscard]] const glm::vec2& getLeftStick() const { return _leftStick; }
	[[nodiscard]] const glm::vec2& getRightStick() const { return _rightStick; }
	[[nodiscard]] float getLeftTrigger() const { return _leftTrigger; }
	[[nodiscard]] float getRightTrigger() const { return _rightTrigger; }
	[[nodiscard]] bool getIsConnected() const { return _isConnected; }

private:
	// Current/previous buttons
	Uint8 _curButtons[SDL_GAMEPAD_BUTTON_MAX];
	Uint8 _prevButtons[SDL_GAMEPAD_BUTTON_MAX];
	// Left/right sticks
	glm::vec2 _leftStick;
	glm::vec2 _rightStick;
	// Left/right trigger
	float _leftTrigger;
	float _rightTrigger;
	// Is this controller connected?
	bool _isConnected;
};

// Wrapper that contains current state of input
struct InputState {
	KeyboardState Keyboard;
	MouseState Mouse;
	ControllerState Controller;
};

class InputSystem {
public:
	bool initialise();
	void shutdown();

	// Called right before SDL_PollEvents loop
	void prepareForUpdate();
	// Called after SDL_PollEvents loop
	void update();
	// Called to process an SDL event in input system (like mouse wheel)
	void processEvent(union SDL_Event& event);

    // GET SET
	[[nodiscard]] const InputState& getState() const { return _inputState; }
	void setRelativeMouseMode(bool value);

private:
	static float filter1D(int input);
	static glm::vec2 filter2D(int inputX, int inputY);

	InputState _inputState{};
	SDL_Gamepad* _controller = nullptr;
};
