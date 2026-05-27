#pragma once

#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX 
#include "Windows.h"
#include <bitset>
#include <tge/Math/Vector.h>

namespace Tga
{
	enum class GamepadButton
	{
		A,
		B,
		X,
		Y,
		LB,
		RB,
		Dpad_Up,
		Dpad_Down,
		Dpad_Right,
		Dpad_Left,
		Start,
		Back,

		Count
	};
	struct ControllerState
	{
		bool connected = false;

		float leftTrigger;
		float rightTrigger;

		Tga::Vector2f leftStick;
		Tga::Vector2f rightStick;

		bool buttons[static_cast<int>(GamepadButton::Count)] = {};
	};

/// <summary>
/// Very bare-bones InputManager to show the ropes of this assignment
/// Has nothing more than keyboard key checks but the
/// principle is the same for other types of input.
/// </summary>
class InputManager
{
	// Holds the "live" state that is being updated by
	// the message pump thread. This can be used in place
	// of myCurrentState but depending on how the game is
	// threaded it may be prudent to keep them separate.
	std::bitset<256> myTentativeState{};

	// The current snapshot when we last ran Update.
	std::bitset<256> myCurrentState{};

	// The previous snapshot.
	std::bitset<256> myPreviousState{};

	HWND myOwnerHWND;
	

	// TEMP PUBLIC
	Vector2i myTentativeMousePosition;
	Vector2i myCurrentMousePosition;
	Vector2i myPreviousMousePosition;

	Vector2i myTentativeMouseDelta;
	Vector2i myMouseDelta;

	float myTentativeMouseWheelDelta;
	float myMouseWheelDelta;

	
public:
	
	InputManager(HWND aWindowHandle);

	bool IsKeyHeld(const int aKeyCode) const;
	bool IsKeyPressed(const int aKeyCode) const;
	bool IsKeyReleased(const int aKeyCode) const;

	Vector2f GetMouseDelta() const;
	Vector2f GetMousePosition() const;

	void ShowMouse() const;
	void HideMouse() const;

	void CaptureMouse() const;
	void ReleaseMouse() const;

	bool UpdateEvents(UINT message, WPARAM wParam, LPARAM lParam);
	void Update();

	void BindAction(std::wstring Action, void (*actionInputFunction)());
	void BindAxis(std::wstring Axis, void (*axisInputFunction)(Vector2f axisValue));

	//NEW CODE
	void UpdateInput();

	bool IsButtonPressed(GamepadButton aButton) const;
	bool IsButtonDown(GamepadButton aButton) const;
	bool IsButtonReleased(GamepadButton aButton) const;
	bool IsConnected() const;

	Tga::Vector2f LeftStick() const;
	Tga::Vector2f RightStick() const;

	bool LeftStickHeldLeft();
	bool LeftStickHeldRight();
	bool LeftStickHeldUp();
	bool LeftStickHeldDown();

	bool RightStickHeldLeft();
	bool RightStickHeldRight();
	bool RightStickHeldUp();
	bool RightStickHeldDown();


	bool PressingPlayerMovingLeft();
	bool PressingPlayerMovingRight();
	bool PressingPlayerMovingUp();
	bool PressingPlayerMovingDown();
	bool PressingJump() const;
	bool ReleasingJump();
	bool PressingPlayerAim();

	bool PressingToggleUp();
	bool PressingToggleLeft() const;
	bool PressingToggleRight() const;
	bool PressingToggleDown();
	bool PressingConfirm() const;


	float LeftTrigger() const;
	float RightTrigger() const;

	bool AnyInputPressed() const;

private:
	Tga::Vector2f ApplyDeadzone(const Tga::Vector2f& aStickValue, float aDeadzoneValue);
	Tga::Vector2f NormalizeStick(const Tga::Vector2f& aStickValue);

	ControllerState myCurrentControllerState;
	ControllerState myPreviousControllerState;
};

} // namespace Tga