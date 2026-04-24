#include <Windowsx.h>
#include "InputHandler.h"

namespace CommonUtilities
{
	InputHandler::InputHandler()
	{
		myCurrentMousePos = { 0,0 };
		myPreviousMousePos = { 0,0 };
		myTentativeMousePos = { 0,0 };
		myWheelX = 0;
		myWheelY = 0;
		myTentativeWheelX = 0;
		myTentativeWheelY = 0;
		myHwnd = nullptr;
		myAutoMouseCapture = false;
		myMouseCaptured = false;
	}

	InputHandler::~InputHandler()
	{
	}

	bool InputHandler::UpdateEvents(UINT message, WPARAM wParam, [[maybe_unused]] LPARAM lParam)
	{
		switch (message)
		{
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			const UINT vk = (UINT)wParam & 0xFFu;
			myTentativeState.set(vk, true);
			return true;
		}
		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			const UINT vk = (UINT)wParam & 0xFFu;
			myTentativeState.set(vk, false);
			return true;
		}
		case WM_KILLFOCUS:
		{
			myTentativeState.reset();
			return false;
		}
		case WM_ACTIVATE:
		{
			const WORD code = LOWORD(wParam);
			if (code == WA_INACTIVE)
			{
				myTentativeState.reset();
			}
			return false;
		}
		case WM_CAPTURECHANGED:
		{
			myMouseCaptured = false;
			return false;
		}

		default:
			break;
		}

		return false;
	}

	bool InputHandler::IsKeyDown(const int aKeyCode) const
	{
		const unsigned int vk = static_cast<unsigned int>(aKeyCode) & 0xFFu;
		return myCurrentState.test(vk);
	}

	bool InputHandler::IsKeyPressed(const int aKeyCode) const
	{
		const unsigned int vk = static_cast<unsigned int>(aKeyCode) & 0xFFu;
		return myCurrentState.test(vk) && !myPreviousState.test(vk);
	}

	bool InputHandler::IsKeyReleased(const int aKeyCode) const
	{
		const unsigned int vk = static_cast<unsigned int>(aKeyCode) & 0xFFu;
		return !myCurrentState.test(vk) && myPreviousState.test(vk);
	}

	bool InputHandler::IsKeyDown(Keys key) const
	{
		return IsKeyDown(static_cast<int>(key));
	}

	bool InputHandler::IsKeyPressed(Keys key) const
	{
		return IsKeyPressed(static_cast<int>(key));
	}

	bool InputHandler::IsKeyReleased(Keys key) const
	{
		return IsKeyReleased(static_cast<int>(key));
	}

	POINT InputHandler::GetMousePos() const
	{
		return myCurrentMousePos;
	}

	POINT InputHandler::GetMouseDelta() const
	{
		return POINT{
			myCurrentMousePos.x - myPreviousMousePos.x,
			myCurrentMousePos.y - myPreviousMousePos.y };
	}

	POINT InputHandler::GetMousePosScreen() const
	{
		if (myHwnd)
		{
			POINT p = myCurrentMousePos;
			ClientToScreen(myHwnd, &p);
			return p;
		}
		POINT p{};
		GetCursorPos(&p);
		return p;
	}

	int InputHandler::GetWheelDeltaX() const
	{
		return myWheelX;
	}

	int InputHandler::GetWheelDeltaY() const
	{
		return myWheelY;
	}

	bool InputHandler::IsMouseButtonDown(int buttonVk) const
	{
		return IsKeyDown(buttonVk);
	}

	bool InputHandler::IsMouseButtonPressed(int buttonVk) const
	{
		return IsKeyPressed(buttonVk);
	}

	bool InputHandler::IsMouseButtonReleased(int buttonVk) const
	{
		return IsKeyReleased(buttonVk);
	}

	bool InputHandler::IsMouseButtonDown(Keys key) const
	{
		return IsMouseButtonDown(static_cast<int>(key));
	}

	bool InputHandler::IsMouseButtonPressed(Keys key) const
	{
		return IsMouseButtonPressed(static_cast<int>(key));
	}

	bool InputHandler::IsMouseButtonReleased(Keys key) const
	{
		return IsMouseButtonReleased(static_cast<int>(key));
	}

	void InputHandler::SetWindowHandle(HWND hWnd)
	{
		myHwnd = hWnd;
	}
	
	HWND InputHandler::GetWindowHandle() const
	{
		return myHwnd;
	}

	void InputHandler::SetAutoMouseCapture(bool enabled)
	{
		myAutoMouseCapture = enabled;
	}

	bool InputHandler::GetAutoMouseCapture() const
	{
		return myAutoMouseCapture;
	}

	bool InputHandler::IsMouseCaptured() const
	{
		return myMouseCaptured;
	}

	bool InputHandler::UpdateMouseInput(UINT aMessage, WPARAM wParam, LPARAM lParam)
	{
		switch (aMessage)
		{
		case WM_MOUSEMOVE:
		{
			myTentativeMousePos.x = GET_X_LPARAM(lParam);
			myTentativeMousePos.y = GET_Y_LPARAM(lParam);
			return true;
		}

		case WM_LBUTTONDOWN:
			myTentativeState.set(0x01u, true);
			TryBeginMouseCapture();
			return true;
		case WM_LBUTTONUP:
			myTentativeState.set(0x01u, false);
			TryEndMouseCaptureIfNoButtons();
			return true;

		case WM_RBUTTONDOWN:
			myTentativeState.set(0x02u, true);
			TryBeginMouseCapture();
			return true;
		case WM_RBUTTONUP:
			myTentativeState.set(0x02u, false);
			TryEndMouseCaptureIfNoButtons();
			return true;

		case WM_MBUTTONDOWN:
			myTentativeState.set(0x04u, true);
			TryBeginMouseCapture();
			return true;
		case WM_MBUTTONUP:
			myTentativeState.set(0x04u, false);
			TryEndMouseCaptureIfNoButtons();
			return true;

		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
		{
			const WORD xb = HIWORD(wParam);
			const UINT vk = (xb == XBUTTON1) ? 0x05u : 0x06u;
			const bool down = (aMessage == WM_XBUTTONDOWN);
			myTentativeState.set(vk, down);
			if (down)
				TryBeginMouseCapture();
			else
				TryEndMouseCaptureIfNoButtons();
			return true;
		}

		case WM_MOUSEWHEEL:
		{
			const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
			myTentativeWheelY += delta;
			return true;
		}
		case WM_MOUSEHWHEEL:
		{
			const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
			myTentativeWheelX += delta;
			return true;
		}

		default:
			return false;
		}
	}

	void InputHandler::UpdateInput()
	{
		myPreviousState = myCurrentState;
		myCurrentState = myTentativeState;

		myPreviousMousePos = myCurrentMousePos;
		myCurrentMousePos = myTentativeMousePos;

		myWheelX = myTentativeWheelX;
		myWheelY = myTentativeWheelY;
		myTentativeWheelX = 0;
		myTentativeWheelY = 0;
	}
} // namespace CommonUtilities

using namespace CommonUtilities;
	// Private helpers
	bool InputHandler::AnyMouseButtonDownTentative() const
	{
		return myTentativeState.test(0x01u) || // L
			myTentativeState.test(0x02u) || // R
			myTentativeState.test(0x04u) || // M
			myTentativeState.test(0x05u) || // X1
			myTentativeState.test(0x06u);   // X2
	}

	void InputHandler::TryBeginMouseCapture()
	{
		if (!myAutoMouseCapture)
			return;
		if (myMouseCaptured)
			return;
		if (!myHwnd)
			return;
		SetCapture(myHwnd);
		myMouseCaptured = true;
	}

	void InputHandler::TryEndMouseCaptureIfNoButtons()
	{
		if (!myAutoMouseCapture)
			return;
		if (!myMouseCaptured)
			return;
		if (AnyMouseButtonDownTentative())
			return;
		ReleaseCapture();
		myMouseCaptured = false;
	}