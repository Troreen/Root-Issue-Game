#include "InputMapper.h"
#include <type_traits>
#include <cassert>
#include <climits>
#include <cmath>

//#############################[ PUBLIC STATIC ]#############################\\

void InputMapper::Init(Tga::InputManager& anInputManager, Tga::XInput& anXboxInput)
{
	InputMapper& instance = GetInstance();

	instance.mySharedInputManager = &anInputManager;
	instance.mySharedXboxInput = &anXboxInput;
}

unsigned InputMapper::AddInputEventListener(std::function<void(const InputEvent& event, bool& outConsumeEvent)> aCallback)
{
	InputMapper& instance = GetInstance();

	instance.myEventListeners.emplace(++instance.myInputEventListenerCount, std::move(aCallback));
	return instance.myInputEventListenerCount;
}

void InputMapper::RemoveInputEventListener(unsigned anInputEventListenerID)
{
	if (GetInstance().myEventListeners.erase(anInputEventListenerID) == static_cast<size_t>(0))
	{
		assert(false && "An invalid input event listener ID was used to attempt removal.");
	}
}

void InputMapper::Update()
{
	InputMapper& instance = GetInstance();
	instance.UpdateMouseAndKeyboard();
	instance.UpdateGamepad();
}

void InputMapper::BindActionToEvent(EInputAction anInputAction, EInputEventType anInputEvent)
{
	GetInstance().myInputMap.emplace(anInputAction, anInputEvent);
}

EInputDeviceType InputMapper::GetLastUsedDevice()
{
	return GetInstance().myLastUsedDevice;
}

bool InputMapper::isDeviceType(const InputEventData& anInputEventData, EInputDeviceType& outDeviceType)
{
	if (std::holds_alternative<EInputDeviceType>(anInputEventData))
	{
		outDeviceType = std::get<EInputDeviceType>(anInputEventData);
		return true;
	}

	return false;
}

bool InputMapper::IsButtonData(const InputEventData& anInputEventData, ButtonInputData& outButtonData)
{
	if (std::holds_alternative<ButtonInputData>(anInputEventData))
	{
		outButtonData = std::get<ButtonInputData>(anInputEventData);
		return true;
	}

	return false;
}

bool InputMapper::IsMouseData(const InputEventData& anInputEventData, MouseInputData& outMouseData)
{
	if (std::holds_alternative<MouseInputData>(anInputEventData))
	{
		outMouseData = std::get<MouseInputData>(anInputEventData);
		return true;
	}

	return false;
}

bool InputMapper::IsRangeData(const InputEventData& anInputEventData, RangeInputData& outRangeData)
{
	if (std::holds_alternative<RangeInputData>(anInputEventData))
	{
		outRangeData = std::get<RangeInputData>(anInputEventData);
		return true;
	}

	return false;
}

bool InputMapper::IsAnalogData(const InputEventData& anInputEventData, AnalogInputData& outAnalogData)
{
	if (std::holds_alternative<AnalogInputData>(anInputEventData))
	{
		outAnalogData = std::get<AnalogInputData>(anInputEventData);
		return true;
	}

	return false;
}

//#############################[ PRIVATE STATIC ]#############################\\

InputMapper& InputMapper::GetInstance()
{
	static InputMapper instance;
	return instance;
}

//#############################[ PRIVATE ]#############################\\

InputMapper::InputMapper()
	: mySharedInputManager(nullptr)
	, mySharedXboxInput(nullptr)
	, myInputEventListenerCount(0u)
	, myPreviousGamepadState(0u)
	, myLastUsedDevice(EInputDeviceType::MouseAndKeyboard)
	, myPreviousLTriggerWasNonZero(false)
	, myPreviousRTriggerWasNonZero(false)
    , myPreviousLThumbWasNonZero(false)
    , myPreviousRThumbWasNonZero(false)
{}

InputMapper::~InputMapper() {}

void InputMapper::UpdateMouseAndKeyboard()
{
	mySharedInputManager->Update();
	bool wasEventDispatched = false;

	{
		const Tga::Vector2f mouseDelta = mySharedInputManager->GetMouseDelta();

		if (mouseDelta.LengthSqr() > 0.0f)
		{
			const MouseInputData data{ mySharedInputManager->GetMousePosition(), mouseDelta };
			TranslateActionToEvent(EInputAction::Mouse_Move, data);
			wasEventDispatched = true;
		}
	}

	if (mySharedInputManager->IsKeyPressed(static_cast<int>(EInputAction::Mouse_L)))
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Mouse_L, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyReleased(static_cast<int>(EInputAction::Mouse_L)))
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Mouse_L, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyPressed(static_cast<int>(EInputAction::Mouse_R)))
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Mouse_R, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyReleased(static_cast<int>(EInputAction::Mouse_R)))
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Mouse_R, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyPressed(static_cast<int>(EInputAction::Mouse_M)))
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Mouse_M, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyReleased(static_cast<int>(EInputAction::Mouse_M)))
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Mouse_M, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyPressed(static_cast<int>(EInputAction::Key_Enter)))
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Key_Enter, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyReleased(static_cast<int>(EInputAction::Key_Enter)))
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Key_Enter, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyPressed(static_cast<int>(EInputAction::Key_Shift)))
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Key_Shift, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyReleased(static_cast<int>(EInputAction::Key_Shift)))
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Key_Shift, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyPressed(static_cast<int>(EInputAction::Key_Control)))
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Key_Control, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyReleased(static_cast<int>(EInputAction::Key_Control)))
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Key_Control, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyPressed(static_cast<int>(EInputAction::Key_Escape)))
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Key_Escape, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyReleased(static_cast<int>(EInputAction::Key_Escape)))
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Key_Escape, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyPressed(static_cast<int>(EInputAction::Key_Space)))
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Key_Space, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyReleased(static_cast<int>(EInputAction::Key_Space)))
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Key_Space, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyPressed(static_cast<int>(EInputAction::Key_Left)))
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Key_Left, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyReleased(static_cast<int>(EInputAction::Key_Left)))
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Key_Left, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyPressed(static_cast<int>(EInputAction::Key_Up)))
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Key_Up, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyReleased(static_cast<int>(EInputAction::Key_Up)))
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Key_Up, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyPressed(static_cast<int>(EInputAction::Key_Right)))
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Key_Right, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyReleased(static_cast<int>(EInputAction::Key_Right)))
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Key_Right, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyPressed(static_cast<int>(EInputAction::Key_Down)))
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Key_Down, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyReleased(static_cast<int>(EInputAction::Key_Down)))
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Key_Down, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyPressed(static_cast<int>(EInputAction::Key_A)))
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Key_A, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyReleased(static_cast<int>(EInputAction::Key_A)))
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Key_A, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyPressed(static_cast<int>(EInputAction::Key_D)))
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Key_D, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyReleased(static_cast<int>(EInputAction::Key_D)))
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Key_D, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyPressed(static_cast<int>(EInputAction::Key_E)))
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Key_E, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyReleased(static_cast<int>(EInputAction::Key_E)))
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Key_E, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyPressed(static_cast<int>(EInputAction::Key_S)))
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Key_S, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyReleased(static_cast<int>(EInputAction::Key_S)))
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Key_S, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyPressed(static_cast<int>(EInputAction::Key_W)))
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Key_W, data);
		wasEventDispatched = true;
	}

	if (mySharedInputManager->IsKeyReleased(static_cast<int>(EInputAction::Key_W)))
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Key_W, data);
		wasEventDispatched = true;
	}

	if (myLastUsedDevice == EInputDeviceType::Gamepad && wasEventDispatched)
	{
		TranslateActionToEvent(EInputAction::InputDeviceSwitch, EInputDeviceType::MouseAndKeyboard);
		myLastUsedDevice = EInputDeviceType::MouseAndKeyboard;
	}
}

void InputMapper::UpdateGamepad()
{
	mySharedXboxInput->Refresh();

	XINPUT_GAMEPAD* gamepadState = mySharedXboxInput->GetState();
	const uint16_t deltaGamepad = gamepadState->wButtons ^ myPreviousGamepadState;
	const uint16_t isGamepadPressed = deltaGamepad & gamepadState->wButtons;
	const uint16_t isGamepadReleased = deltaGamepad & myPreviousGamepadState;

	bool wasEventDispatched = false;

	if ((isGamepadPressed & XINPUT_GAMEPAD_DPAD_UP) > 0u)
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Pad_Up, data);
		wasEventDispatched = true;
	}

	if ((isGamepadReleased & XINPUT_GAMEPAD_DPAD_UP) > 0u)
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Pad_Up, data);
		wasEventDispatched = true;
	}

	if ((isGamepadPressed & XINPUT_GAMEPAD_DPAD_DOWN) > 0u)
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Pad_Down, data);
		wasEventDispatched = true;
	}

	if ((isGamepadReleased & XINPUT_GAMEPAD_DPAD_DOWN) > 0u)
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Pad_Down, data);
		wasEventDispatched = true;
	}

	if ((isGamepadPressed & XINPUT_GAMEPAD_DPAD_LEFT) > 0u)
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Pad_Left, data);
		wasEventDispatched = true;
	}

	if ((isGamepadReleased & XINPUT_GAMEPAD_DPAD_LEFT) > 0u)
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Pad_Left, data);
		wasEventDispatched = true;
	}

	if ((isGamepadPressed & XINPUT_GAMEPAD_DPAD_RIGHT) > 0u)
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Pad_Right, data);
		wasEventDispatched = true;
	}

	if ((isGamepadReleased & XINPUT_GAMEPAD_DPAD_RIGHT) > 0u)
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Pad_Right, data);
		wasEventDispatched = true;
	}

	if ((isGamepadPressed & XINPUT_GAMEPAD_START) > 0u)
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Pad_Start, data);
		wasEventDispatched = true;
	}

	if ((isGamepadReleased & XINPUT_GAMEPAD_START) > 0u)
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Pad_Start, data);
		wasEventDispatched = true;
	}

	if ((isGamepadPressed & XINPUT_GAMEPAD_BACK) > 0u)
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Pad_Back, data);
		wasEventDispatched = true;
	}

	if ((isGamepadReleased & XINPUT_GAMEPAD_BACK) > 0u)
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Pad_Back, data);
		wasEventDispatched = true;
	}

	if ((isGamepadPressed & XINPUT_GAMEPAD_LEFT_THUMB) > 0u)
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Pad_LThumb, data);
		wasEventDispatched = true;
	}

	if ((isGamepadReleased & XINPUT_GAMEPAD_LEFT_THUMB) > 0u)
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Pad_LThumb, data);
		wasEventDispatched = true;
	}

	if ((isGamepadPressed & XINPUT_GAMEPAD_RIGHT_THUMB) > 0u)
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Pad_RThumb, data);
		wasEventDispatched = true;
	}

	if ((isGamepadReleased & XINPUT_GAMEPAD_RIGHT_THUMB) > 0u)
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Pad_RThumb, data);
		wasEventDispatched = true;
	}

	if ((isGamepadPressed & XINPUT_GAMEPAD_LEFT_SHOULDER) > 0u)
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Pad_LShoulder, data);
		wasEventDispatched = true;
	}

	if ((isGamepadReleased & XINPUT_GAMEPAD_LEFT_SHOULDER) > 0u)
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Pad_LShoulder, data);
		wasEventDispatched = true;
	}

	if ((isGamepadPressed & XINPUT_GAMEPAD_RIGHT_SHOULDER) > 0u)
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Pad_RShoulder, data);
		wasEventDispatched = true;
	}

	if ((isGamepadReleased & XINPUT_GAMEPAD_RIGHT_SHOULDER) > 0u)
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Pad_RShoulder, data);
		wasEventDispatched = true;
	}

	if ((isGamepadPressed & XINPUT_GAMEPAD_A) > 0u)
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Pad_A, data);
		wasEventDispatched = true;
	}

	if ((isGamepadReleased & XINPUT_GAMEPAD_A) > 0u)
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Pad_A, data);
		wasEventDispatched = true;
	}

	if ((isGamepadPressed & XINPUT_GAMEPAD_B) > 0u)
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Pad_B, data);
		wasEventDispatched = true;
	}

	if ((isGamepadReleased & XINPUT_GAMEPAD_B) > 0u)
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Pad_B, data);
		wasEventDispatched = true;
	}

	if ((isGamepadPressed & XINPUT_GAMEPAD_X) > 0u)
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Pad_X, data);
		wasEventDispatched = true;
	}

	if ((isGamepadReleased & XINPUT_GAMEPAD_X) > 0u)
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Pad_X, data);
		wasEventDispatched = true;
	}

	if ((isGamepadPressed & XINPUT_GAMEPAD_Y) > 0u)
	{
		const ButtonInputData data{ true };
		TranslateActionToEvent(EInputAction::Pad_Y, data);
		wasEventDispatched = true;
	}

	if ((isGamepadReleased & XINPUT_GAMEPAD_Y) > 0u)
	{
		const ButtonInputData data{ false };
		TranslateActionToEvent(EInputAction::Pad_Y, data);
		wasEventDispatched = true;
	}

	{
		const float value = GetTransformedRangeValue(gamepadState->bLeftTrigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
		const bool isNonZero = value != 0.0f;

		if (isNonZero || myPreviousLTriggerWasNonZero)
		{
			const RangeInputData data{ value };
			TranslateActionToEvent(EInputAction::Pad_LTrigger, data);
			myPreviousLTriggerWasNonZero = isNonZero;
			wasEventDispatched = true;
		}
	}

	{
		const float value = GetTransformedRangeValue(gamepadState->bRightTrigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
		const bool isNonZero = value != 0.0f;

		if (isNonZero || myPreviousRTriggerWasNonZero)
		{
			const RangeInputData data{ value };
			TranslateActionToEvent(EInputAction::Pad_RTrigger, data);
			myPreviousRTriggerWasNonZero = isNonZero;
			wasEventDispatched = true;
		}
	}

	{
		const Tga::Vector2f analogValues = GetTransformedAnalogValues(gamepadState->sThumbLX, gamepadState->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
		const bool isNonZero = analogValues.x != 0.0f || analogValues.y != 0.0f;
		
		if (isNonZero || myPreviousLThumbWasNonZero)
		{
			const AnalogInputData data{ analogValues };
			TranslateActionToEvent(EInputAction::Pad_LStick, data);
			myPreviousLThumbWasNonZero = isNonZero;
			wasEventDispatched = true;
		}
	}

	{
		const Tga::Vector2f analogValues = GetTransformedAnalogValues(gamepadState->sThumbRX, gamepadState->sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
		const bool isNonZero = analogValues.x != 0.0f || analogValues.y != 0.0f;

		if (isNonZero || myPreviousRThumbWasNonZero)
		{
			const AnalogInputData data{ analogValues };
			TranslateActionToEvent(EInputAction::Pad_RStick, data);
			myPreviousRThumbWasNonZero = isNonZero;
			wasEventDispatched = true;
		}
	}

	if (myLastUsedDevice == EInputDeviceType::MouseAndKeyboard && wasEventDispatched)
	{
		TranslateActionToEvent(EInputAction::InputDeviceSwitch, EInputDeviceType::Gamepad);
		myLastUsedDevice = EInputDeviceType::Gamepad;
	}

	myPreviousGamepadState = gamepadState->wButtons;
}

void InputMapper::TranslateActionToEvent(const EInputAction anInputAction, const InputEventData& someData) const
{
	auto range = myInputMap.equal_range(anInputAction);

	for (auto& i = range.first; i != range.second; ++i)
	{
		InputEvent event{ i->second, someData };
		DispatchEvent(event);
	}
}

void InputMapper::DispatchEvent(const InputEvent& anInputEvent) const
{
	bool wasEventConsumed = false;

	for (const auto& pair : myEventListeners)
	{
		std::invoke(pair.second, anInputEvent, wasEventConsumed);

		if (wasEventConsumed)
		{
			return;
		}
	}
}

Tga::Vector2f InputMapper::GetTransformedAnalogValues(const short aThumbXValue, const short aThumbYValue, const int aThreshold) const
{
	const float invMax = 1.0f / (std::numeric_limits<short>::max() - aThreshold);
	float valueX = 0.0f;
	float valueY = 0.0f;

	{
		const int absThumbX = std::abs(aThumbXValue);

		if (absThumbX > aThreshold)
		{
			const int rawX = absThumbX - aThreshold;
			const float signX = aThumbXValue < 0 ? -1.0f : 1.0f;
			valueX = rawX * signX * invMax;
		}
	}
	{
		const int absThumbY = std::abs(aThumbYValue);

		if (absThumbY > aThreshold)
		{
			const int rawY = absThumbY - aThreshold;
			const float signY = aThumbYValue < 0 ? -1.0f : 1.0f;
			valueY = rawY * signY * invMax;
		}
	}

	return Tga::Vector2f{ valueX, valueY };
}

float InputMapper::GetTransformedRangeValue(const unsigned char aTriggerValue, const int aThreshold) const
{
	float value = 0.0f;

	if (aTriggerValue > aThreshold)
	{
		const float invMax = 1.0f / (std::numeric_limits<unsigned char>::max() - aThreshold);
		value = (aTriggerValue - aThreshold) * invMax;
	}

	return value;
}
