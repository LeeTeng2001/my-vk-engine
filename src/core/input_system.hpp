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
	[[nodiscard]] bool GetButtonValue(SDL_GameControllerButton button) const;
	[[nodiscard]] ButtonState GetButtonState(SDL_GameControllerButton button) const;

    // Controller specific
	[[nodiscard]] const Vector2& GetLeftStick() const { return mLeftStick; }
	[[nodiscard]] const Vector2& GetRightStick() const { return mRightStick; }
	[[nodiscard]] float GetLeftTrigger() const { return mLeftTrigger; }
	[[nodiscard]] float GetRightTrigger() const { return mRightTrigger; }
	[[nodiscard]] bool GetIsConnected() const { return mIsConnected; }

private:
	// Current/previous buttons
	Uint8 mCurrButtons[SDL_CONTROLLER_BUTTON_MAX];
	Uint8 mPrevButtons[SDL_CONTROLLER_BUTTON_MAX];
	// Left/right sticks
	Vector2 mLeftStick;
	Vector2 mRightStick;
	// Left/right trigger
	float mLeftTrigger;
	float mRightTrigger;
	// Is this controller connected?
	bool mIsConnected;
};

// Wrapper that contains current state of input
struct InputState {
	KeyboardState Keyboard;
	MouseState Mouse;
	ControllerState Controller;
};


class InputSystem {
public:
    // Like Game
	bool initialise();
	void shutdown();

	// Called right before SDL_PollEvents loop
	void PrepareForUpdate();
	// Called after SDL_PollEvents loop
	void Update();
	// Called to process an SDL event in input system (like mouse wheel)
	void ProcessEvent(union SDL_Event& event);

    // GET SET
	[[nodiscard]] const InputState& getState() const { return _inputState; }
	void setRelativeMouseMode(bool value);

private:
	static float Filter1D(int input);
	static glm::vec2 Filter2D(int inputX, int inputY);

	InputState _inputState{};
	SDL_Gamepad* _controller = nullptr;
};
