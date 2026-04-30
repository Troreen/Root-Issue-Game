#include "stdafx.h"

#include "InputManager.h"
#include "Windowsx.h"
#include "EnumKeys.h"

#include <Xinput.h>


#pragma comment(lib, "Xinput.lib")

using namespace Tga;

InputManager::InputManager(HWND aWindowHandle)
{
	myOwnerHWND = aWindowHandle;
	
	#ifndef HID_USAGE_PAGE_GENERIC
	#define HID_USAGE_PAGE_GENERIC         ((USHORT) 0x01)
	#endif
	#ifndef HID_USAGE_GENERIC_MOUSE
	#define HID_USAGE_GENERIC_MOUSE        ((USHORT) 0x02)
	#endif

	RAWINPUTDEVICE Rid[1];
	Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
	Rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
	Rid[0].dwFlags = RIDEV_INPUTSINK;
	Rid[0].hwndTarget = myOwnerHWND;
	RegisterRawInputDevices(Rid, 1, sizeof(Rid[0]));
	
	myTentativeMousePosition = { 0,0 };
	myCurrentMousePosition = { 0,0 };
	myPreviousMousePosition = { 0,0 };
	myMouseDelta = { 0,0 };

	myTentativeMouseWheelDelta = 0;
	myMouseWheelDelta = 0;
}

bool InputManager::IsKeyHeld(const int aKeyCode) const
{
	return myCurrentState[aKeyCode] && myPreviousState[aKeyCode];
}

bool InputManager::IsKeyPressed(const int aKeyCode) const
{
	return myCurrentState[aKeyCode] && !myPreviousState[aKeyCode];
}

bool InputManager::IsKeyReleased(const int aKeyCode) const
{
	return !myCurrentState[aKeyCode] && myPreviousState[aKeyCode];
}

Vector2f InputManager::GetMouseDelta() const
{
	return Vector2f((float)myMouseDelta.x, (float)myMouseDelta.y);
}

Vector2f InputManager::GetMousePosition() const
{
	return Vector2f((float)myCurrentMousePosition.x, (float)myCurrentMousePosition.y);
}

void InputManager::ShowMouse() const
{
	ShowCursor(true);
}

void InputManager::HideMouse() const
{
	ShowCursor(false);
}

void InputManager::CaptureMouse() const
{
	RECT clipRect;

	GetClientRect(myOwnerHWND, &clipRect);

	POINT upperLeft;
	upperLeft.x = clipRect.left;
	upperLeft.y = clipRect.top;

	POINT lowerRight;
	lowerRight.x = clipRect.right;
	lowerRight.y = clipRect.bottom;

	MapWindowPoints(myOwnerHWND, nullptr, &upperLeft, 1);
	MapWindowPoints(myOwnerHWND, nullptr, &lowerRight, 1);

	clipRect.left = upperLeft.x;
	clipRect.top = upperLeft.y;
	clipRect.right = lowerRight.x;
	clipRect.bottom = lowerRight.y;

	ClipCursor(&clipRect);
}

void InputManager::ReleaseMouse() const
{
	ClipCursor(nullptr);
}

bool InputManager::UpdateEvents(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_KEYDOWN:
		myTentativeState[wParam] = true;
		return true;
	case WM_KEYUP:
		myTentativeState[wParam] = false;
		return true;

	case WM_LBUTTONDOWN:
		myTentativeState[VK_LBUTTON] = true;
		return true;

	case WM_LBUTTONUP:
		myTentativeState[VK_LBUTTON] = false;
		return true;

	case WM_RBUTTONDOWN:
		myTentativeState[VK_RBUTTON] = true;
		return true;

	case WM_RBUTTONUP:
		myTentativeState[VK_RBUTTON] = false;
		return true;

	case WM_MBUTTONDOWN:
		myTentativeState[VK_MBUTTON] = true;
		return true;

	case WM_MBUTTONUP:
		myTentativeState[VK_MBUTTON] = false;
		return true;
	case WM_SYSKEYDOWN:
		myTentativeState[wParam] = true;
		return true;
	case WM_SYSKEYUP:
		myTentativeState[wParam] = false;
		return true;

	case WM_XBUTTONDOWN:
		{
			const int xButton = GET_XBUTTON_WPARAM(wParam);
			if (xButton == 1)
				myTentativeState[VK_XBUTTON1] = true;
			else
				myTentativeState[VK_XBUTTON2] = true;

			return true;
		}

	case WM_XBUTTONUP:
		{
			const int xButton = GET_XBUTTON_WPARAM(wParam);
			if (xButton == 1)
				myTentativeState[VK_XBUTTON1] = false;
			else
				myTentativeState[VK_XBUTTON2] = false;
			return true;
		}

	case WM_MOUSEWHEEL:
		myTentativeMouseWheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		return true;

	// This is only used for when you want X/Y coordinates.
	// Reason being is that it's clunky to rely on for delta
	// movements of the mouse. ClipRect and SetMousePos all
	// cause their own problems for input which are easily
	// solved by registering for the raw HID data and listening
	// for WM_INPUT instead.
	case WM_MOUSEMOVE:
		{
			const int xPos = GET_X_LPARAM(lParam);
			const int yPos = GET_Y_LPARAM(lParam);

			myTentativeMousePosition.X = xPos;
			myTentativeMousePosition.Y = yPos;

			return true;
		}

	// Handles mouse delta, used in 3D navigation etc.
	case WM_INPUT:
		{
			UINT dwSize = sizeof(RAWINPUT);
			static BYTE lpb[sizeof(RAWINPUT)];

			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));

			RAWINPUT* raw = (RAWINPUT*)lpb;

			if (raw->header.dwType == RIM_TYPEMOUSE)
			{
				myTentativeMouseDelta.X += raw->data.mouse.lLastX;
				myTentativeMouseDelta.Y += raw->data.mouse.lLastY;
			}
			return true;
		}
	}

	return false;
}

void InputManager::Update()
{
	myPreviousMousePosition = myCurrentMousePosition;
	myCurrentMousePosition = myTentativeMousePosition;

	myMouseDelta = myTentativeMouseDelta;
	myTentativeMouseDelta = { 0, 0};

	myMouseWheelDelta = myTentativeMouseWheelDelta / abs(myTentativeMouseWheelDelta);
	myTentativeMouseWheelDelta = 0;
	
	myPreviousState = myCurrentState;
	myCurrentState = myTentativeState;
}

#include <algorithm>



//NEW CODE!!
void InputManager::UpdateInput()
{
	myPreviousControllerState = myCurrentControllerState;

	XINPUT_STATE xState{};
	ZeroMemory(&xState, sizeof(XINPUT_STATE));

	DWORD result = XInputGetState(0, &xState);
	if (result != ERROR_SUCCESS)
	{
		myCurrentControllerState = {};
		myCurrentControllerState.connected = false;
		return;
	}
	myCurrentControllerState.connected = true;

	XINPUT_GAMEPAD& g = xState.Gamepad;

	myCurrentControllerState.buttons[static_cast<int>(GamepadButton::A)] = g.wButtons & XINPUT_GAMEPAD_A;
	myCurrentControllerState.buttons[static_cast<int>(GamepadButton::B)] = g.wButtons & XINPUT_GAMEPAD_B;
	myCurrentControllerState.buttons[static_cast<int>(GamepadButton::X)] = g.wButtons & XINPUT_GAMEPAD_X;
	myCurrentControllerState.buttons[static_cast<int>(GamepadButton::Y)] = g.wButtons & XINPUT_GAMEPAD_Y;
	myCurrentControllerState.buttons[static_cast<int>(GamepadButton::LB)] = g.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
	myCurrentControllerState.buttons[static_cast<int>(GamepadButton::RB)] = g.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
	myCurrentControllerState.buttons[static_cast<int>(GamepadButton::Dpad_Up)] = g.wButtons & XINPUT_GAMEPAD_DPAD_UP;
	myCurrentControllerState.buttons[static_cast<int>(GamepadButton::Dpad_Down)] = g.wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
	myCurrentControllerState.buttons[static_cast<int>(GamepadButton::Dpad_Right)] = g.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
	myCurrentControllerState.buttons[static_cast<int>(GamepadButton::Dpad_Left)] = g.wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
	myCurrentControllerState.buttons[static_cast<int>(GamepadButton::Start)] = g.wButtons & XINPUT_GAMEPAD_START;
	myCurrentControllerState.buttons[static_cast<int>(GamepadButton::Back)] = g.wButtons & XINPUT_GAMEPAD_BACK;

	const float deadzone = 0.15f;

	Tga::Vector2f LS;
	LS.x = g.sThumbLX;
	LS.y = g.sThumbLY;
	LS = ApplyDeadzone(LS, deadzone);
	myCurrentControllerState.leftStick = NormalizeStick(LS);

	Tga::Vector2f RS;
	RS.x = g.sThumbRX;
	RS.y = g.sThumbRY;
	RS = ApplyDeadzone(RS, deadzone);
	myCurrentControllerState.rightStick = NormalizeStick(RS);

	myCurrentControllerState.leftTrigger = static_cast<float>(g.bLeftTrigger) / 255.f;
	myCurrentControllerState.rightTrigger = static_cast<float>(g.bRightTrigger) / 255.f;
}

bool InputManager::IsButtonPressed(GamepadButton aButton) const
{
	return myCurrentControllerState.buttons[static_cast<int>(aButton)] && !myPreviousControllerState.buttons[static_cast<int>(aButton)];
}

bool InputManager::IsButtonDown(GamepadButton aButton) const
{
	return myCurrentControllerState.buttons[static_cast<int>(aButton)];
}

bool InputManager::IsButtonReleased(GamepadButton aButton) const
{
	return !myCurrentControllerState.buttons[static_cast<int>(aButton)] && myPreviousControllerState.buttons[static_cast<int>(aButton)];
}

bool InputManager::IsConnected() const
{
	return myCurrentControllerState.connected;
}

Tga::Vector2f InputManager::LeftStick() const
{
	return myCurrentControllerState.leftStick;
}

Tga::Vector2f InputManager::RightStick() const
{
	return myCurrentControllerState.rightStick;
}

bool InputManager::LeftStickHeldLeft()
{
	return LeftStick().x < -0.5f;
}

bool InputManager::LeftStickHeldRight()
{
	return LeftStick().x > 0.5f;
}

bool InputManager::RightStickHeldRight()
{
	return RightStick().x > 0.5f;
}

bool InputManager::RightStickHeldLeft()
{
	return RightStick().x < -0.5f;
}


bool InputManager::RightStickHeldUp()
{
	return RightStick().y > 0.5f;
}

bool InputManager::RightStickHeldDown()
{
	return RightStick().y < -0.5f;
}


bool InputManager::LeftStickHeldUp()
{
	return LeftStick().y > 0.5f;
}

bool InputManager::LeftStickHeldDown()
{
	return LeftStick().y < -0.5f;
}

float InputManager::LeftTrigger() const
{
	return myCurrentControllerState.leftTrigger;
}

float InputManager::RightTrigger() const
{
	return myCurrentControllerState.rightTrigger;
}


bool InputManager::PressingPlayerMovingLeft()
{
	return (
		IsKeyHeld(static_cast<int>(EngineCU::Keys::A)) && !IsKeyReleased(static_cast<int>(EngineCU::Keys::A)) ||
		IsKeyHeld(static_cast<int>(EngineCU::Keys::LEFT)) && !IsKeyReleased(static_cast<int>(EngineCU::Keys::LEFT)) ||
		IsButtonDown(Tga::GamepadButton::Dpad_Left) && !IsButtonReleased(Tga::GamepadButton::Dpad_Left) ||
		LeftStickHeldLeft() || RightStickHeldLeft() || LeftTrigger()
		);
}


bool InputManager::PressingPlayerMovingRight()
{
	return (
		IsKeyHeld(static_cast<int>(EngineCU::Keys::D)) && !IsKeyReleased(static_cast<int>(EngineCU::Keys::D)) ||
		IsKeyHeld(static_cast<int>(EngineCU::Keys::RIGHT)) && !IsKeyReleased(static_cast<int>(EngineCU::Keys::RIGHT)) ||
		IsButtonDown(Tga::GamepadButton::Dpad_Right) && !IsButtonReleased(Tga::GamepadButton::Dpad_Right) ||
		LeftStickHeldRight() || RightStickHeldRight() || RightTrigger()
		);
}

bool InputManager::PressingJump() const
{
	return (
		(IsButtonPressed(Tga::GamepadButton::A) && !IsButtonReleased(Tga::GamepadButton::A)) ||
		IsKeyPressed(static_cast<int>(EngineCU::Keys::W)) && !IsKeyReleased(static_cast<int>(EngineCU::Keys::W)) ||
		IsKeyPressed(static_cast<int>(EngineCU::Keys::SPACE)) && !IsKeyReleased(static_cast<int>(EngineCU::Keys::SPACE)) ||
		IsKeyPressed(static_cast<int>(EngineCU::Keys::UP)) && !IsKeyReleased(static_cast<int>(EngineCU::Keys::UP))
		);
}

bool InputManager::ReleasingJump()
{
	return (
		IsKeyReleased(static_cast<int>(EngineCU::Keys::SPACE)) ||
		IsKeyReleased(static_cast<int>(EngineCU::Keys::W)) ||
		IsKeyReleased(static_cast<int>(EngineCU::Keys::UP)) ||
		IsButtonReleased(Tga::GamepadButton::A)
		);
}



bool InputManager::PressingToggleUp()
{
	return (
		IsButtonPressed(Tga::GamepadButton::Dpad_Up) && !IsButtonReleased(Tga::GamepadButton::Dpad_Up) ||
		IsKeyPressed(static_cast<int>(EngineCU::Keys::W)) && !IsKeyReleased(static_cast<int>(EngineCU::Keys::W)) ||
		IsKeyPressed(static_cast<int>(EngineCU::Keys::UP)) && !IsKeyReleased(static_cast<int>(EngineCU::Keys::UP))/*||
		LeftStickHeldUp()*/
		);
}


bool InputManager::PressingToggleDown()
{
	return (
		IsButtonPressed(Tga::GamepadButton::Dpad_Down) && !IsButtonReleased(Tga::GamepadButton::Dpad_Down) ||
		IsKeyPressed(static_cast<int>(EngineCU::Keys::S)) && !IsKeyReleased(static_cast<int>(EngineCU::Keys::S)) ||
		IsKeyPressed(static_cast<int>(EngineCU::Keys::DOWN)) && !IsKeyReleased(static_cast<int>(EngineCU::Keys::DOWN)) /*||
		LeftStickHeldDown()*/
		);
}

bool InputManager::PressingConfirm() const
{
	return (
		IsButtonPressed(Tga::GamepadButton::A) && !IsButtonReleased(Tga::GamepadButton::A) ||
		IsKeyPressed(static_cast<int>(EngineCU::Keys::SPACE)) && !IsKeyReleased(static_cast<int>(EngineCU::Keys::SPACE)) ||
		IsKeyPressed(static_cast<int>(EngineCU::Keys::ENTER)) && !IsKeyReleased(static_cast<int>(EngineCU::Keys::ENTER))
		);
}


bool InputManager::PressingToggleLeft() const
{
	return (
		IsButtonPressed(Tga::GamepadButton::Dpad_Left) && !IsButtonReleased(Tga::GamepadButton::Dpad_Left) ||
		IsKeyPressed(static_cast<int>(EngineCU::Keys::A)) && !IsKeyReleased(static_cast<int>(EngineCU::Keys::A)) ||
		IsKeyPressed(static_cast<int>(EngineCU::Keys::LEFT)) && !IsKeyReleased(static_cast<int>(EngineCU::Keys::LEFT))
		);
}

bool InputManager::PressingToggleRight() const
{
	return (
		IsButtonPressed(Tga::GamepadButton::Dpad_Right) && !IsButtonReleased(Tga::GamepadButton::Dpad_Right) ||
		IsKeyPressed(static_cast<int>(EngineCU::Keys::D)) && !IsKeyReleased(static_cast<int>(EngineCU::Keys::D)) ||
		IsKeyPressed(static_cast<int>(EngineCU::Keys::RIGHT)) && !IsKeyReleased(static_cast<int>(EngineCU::Keys::RIGHT))
		);
}

Tga::Vector2f InputManager::ApplyDeadzone(const Tga::Vector2f& aStickValue, float aDeadzoneValue)
{
	float magnitude = sqrt(aStickValue.x * aStickValue.x + aStickValue.y * aStickValue.y);

	if (magnitude < aDeadzoneValue)
	{
		return { 0.0f, 0.0f };
	}

	return aStickValue;
}

Tga::Vector2f InputManager::NormalizeStick(const Tga::Vector2f& aStickValue)
{
	Tga::Vector2f returnValue;
	returnValue.x = aStickValue.x / 32767.0f;
	returnValue.y = aStickValue.y / 32767.0f;

	returnValue.x = std::clamp(returnValue.x, -1.0f, 1.0f);
	returnValue.y = std::clamp(returnValue.y, -1.0f, 1.0f);

	return returnValue;
}

bool InputManager::AnyInputPressed() const
{
	for (int i = 0; i < 256; i++)
	{
		if (IsKeyPressed(i))
		{
			return true;
		}
	}

	for (int i = 0; i < static_cast<int>(GamepadButton::Count); i++)
	{
		if (IsButtonPressed(static_cast<GamepadButton>(i)))
		{
			return true;
		}
	}

	return false;
}
