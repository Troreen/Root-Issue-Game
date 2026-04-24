#pragma once
#include <tge/input/InputManager.h>
#include <tge/math/vector2.h>
#include <tge/input/XInput.h>
#include <unordered_map>
#include <functional>
#include <variant>

enum class EInputAction : int
{
	Mouse_Move = 0x00,
	Mouse_L = 0x01,
	Mouse_R = 0x02,
	Mouse_M = 0x04,
	Key_Enter = 0x0D,
	Key_Shift = 0x10,
	Key_Control = 0x11,
	Key_Escape = 0x1B,
	Key_Space = 0x20,
	Key_Left = 0x25,
	Key_Up = 0x26,
	Key_Right = 0x27,
	Key_Down = 0x28,
	Key_A = 0x41,
	Key_D = 0x44,
	Key_E = 0x45,
	Key_S = 0x53,
	Key_W = 0x57,
	Pad_Up,
	Pad_Down,
	Pad_Left,
	Pad_Right,
	Pad_Start,
	Pad_Back,
	Pad_LThumb,
	Pad_RThumb,
	Pad_LShoulder,
	Pad_RShoulder,
	Pad_A,
	Pad_B,
	Pad_X,
	Pad_Y,
	Pad_LTrigger,
	Pad_RTrigger,
	Pad_LStick,
	Pad_RStick,
	InputDeviceSwitch
};

enum class EInputEventType
{
	Move_Analog,
	Move_Up,
	Move_Down,
	Move_Left,
	Move_Right,
	Jump,
	Dash,
	Use,
	Select,
	Back,
	Pause,
	New_Device
};

enum class EInputDeviceType
{
	MouseAndKeyboard,
	Gamepad
};

struct ButtonInputData
{
	bool isPressed;
};

struct MouseInputData
{
	Tga::Vector2f position;
	Tga::Vector2f delta;
};

struct RangeInputData
{
	float weight;
};

struct AnalogInputData
{
	Tga::Vector2f offset;
};

using InputEventData = std::variant<EInputDeviceType, ButtonInputData, MouseInputData, RangeInputData, AnalogInputData>;

struct InputEvent
{
	EInputEventType type;
	InputEventData data;
};

class InputMapper
{
	public:
		static void Init(Tga::InputManager& anInputManager, Tga::XInput& anXboxInput);

		static void Update();
		static unsigned AddInputEventListener(std::function<void(const InputEvent& event, bool& outConsumeEvent)> aCallback);
		static void RemoveInputEventListener(unsigned anInputEventListenerID);
		static void BindActionToEvent(EInputAction anInputAction, EInputEventType anInputEvent);
		static EInputDeviceType GetLastUsedDevice();

		static bool isDeviceType(const InputEventData& anInputEventData, EInputDeviceType& outDeviceType);
		static bool IsButtonData(const InputEventData& anInputEventData, ButtonInputData& outButtonData);
		static bool IsMouseData(const InputEventData& anInputEventData, MouseInputData& outMouseData);
		static bool IsRangeData(const InputEventData& anInputEventData, RangeInputData& outRangeData);
		static bool IsAnalogData(const InputEventData& anInputEventData, AnalogInputData& outAnalogData);

	private:
		static InputMapper& GetInstance();

		std::unordered_map<unsigned, std::function<void(const InputEvent&, bool&)>> myEventListeners;
		std::unordered_multimap<EInputAction, EInputEventType> myInputMap;
		Tga::InputManager* mySharedInputManager;
		Tga::XInput* mySharedXboxInput;
		unsigned myInputEventListenerCount;
		uint16_t myPreviousGamepadState;
		EInputDeviceType myLastUsedDevice;
		bool myPreviousLTriggerWasNonZero;
		bool myPreviousRTriggerWasNonZero;
		bool myPreviousLThumbWasNonZero;
		bool myPreviousRThumbWasNonZero;

		InputMapper();
		~InputMapper();

		void UpdateMouseAndKeyboard();
		void UpdateGamepad();
		void TranslateActionToEvent(const EInputAction anInputAction, const InputEventData& someData) const;
		void DispatchEvent(const InputEvent& anInputEvent) const;

		Tga::Vector2f GetTransformedAnalogValues(const short aThumbXValue, const short aThumbYValue, const int aThreshold) const;
		float GetTransformedRangeValue(const unsigned char aTriggerValue, const int aThreshold) const;
};
