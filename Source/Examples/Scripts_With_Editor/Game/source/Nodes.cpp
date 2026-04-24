#include "Nodes.h"

#include <tge/script/ScriptNodeBase.h>
#include <tge/script/Contexts/ScriptExecutionContext.h>
#include <tge/script/BaseProperties.h>
#include <tge/script/ScriptNodeTypeRegistry.h>

#include "GameWorld.h"


class CheckPressurePlateNode : public Tga::ScriptNodeBase
{
	Tga::ScriptPinId myEntityNamePin;

	Tga::ScriptPinId myOnPressed;
	Tga::ScriptPinId myOnReleased;

public:
	void Init(const Tga::ScriptCreationContext& context) override
	{
		{
			Tga::ScriptPin pin = {};
			pin.type = Tga::ScriptLinkType::Property;
			pin.dataType = Tga::GetPropertyType<Tga::StringId>();
			pin.defaultValue = Tga::Property::Create<Tga::StringId>();
			pin.name = "EntityName"_tgaid;
			pin.node = context.GetNodeId();
			pin.role = Tga::ScriptPinRole::Input;

			myEntityNamePin = context.FindOrCreatePin(pin);
		}

		{
			Tga::ScriptPin outputPin = {};
			outputPin.type = Tga::ScriptLinkType::Flow;
			outputPin.name = "OnPressed"_tgaid;
			outputPin.node = context.GetNodeId();
			outputPin.role = Tga::ScriptPinRole::Output;

			myOnPressed = context.FindOrCreatePin(outputPin);
		}

		{
			Tga::ScriptPin outputPin = {};
			outputPin.type = Tga::ScriptLinkType::Flow;
			outputPin.name = "OnReleased"_tgaid;
			outputPin.node = context.GetNodeId();
			outputPin.role = Tga::ScriptPinRole::Output;

			myOnReleased = context.FindOrCreatePin(outputPin);
		}
	}

	// this is a bit wasteful, this node is set up to run each frame, but only triggers its output if a plate is pressed or released
	// a better design would be to somehow register it as an observer and only schedule it for running if needed, with ScriptRuntimeInstance::TriggerPin
	Tga::ScriptNodeResult Execute(Tga::ScriptExecutionContext& context, Tga::ScriptPinId) const override
	{
		GameScriptUpdateContext* gameContext = dynamic_cast<GameScriptUpdateContext*>(&context.GetUpdateContext());

		if (gameContext)
		{
			Tga::Property nameProperty = context.ReadInputPin(myEntityNamePin);
			Tga::StringId name = *nameProperty.Get<Tga::StringId>();

			bool isPressed = gameContext->gameWorld->IsPlatePressed(name);
			bool wasPressed = gameContext->gameWorld->WasPlatePressed(name);

			if (isPressed && !wasPressed)
			{
				context.TriggerOutputPin(myOnPressed);
			}

			if (!isPressed && wasPressed)
			{
				context.TriggerOutputPin(myOnReleased);
			}
		}

		return Tga::ScriptNodeResult::KeepRunning;
	}

	bool ShouldExecuteAtStart() const override { return true; }
};

struct IsPlayerNearbyNodeState
{
	bool wasNearby;
};
class IsPlayerNearbyNode : public Tga::ScriptNodeWithRuntimeData<IsPlayerNearbyNodeState>
{
	Tga::ScriptPinId myPositionToCheck;

	Tga::ScriptPinId myOnEnter;
	Tga::ScriptPinId myOnExit;

public:
	void Init(const Tga::ScriptCreationContext& context) override
	{
		{
			Tga::ScriptPin pin = {};
			pin.type = Tga::ScriptLinkType::Property;
			pin.dataType = Tga::GetPropertyType<Tga::Vector2f>();
			pin.defaultValue = Tga::Property::Create<Tga::Vector2f>();
			pin.name = "Position"_tgaid;
			pin.node = context.GetNodeId();
			pin.role = Tga::ScriptPinRole::Input;

			myPositionToCheck = context.FindOrCreatePin(pin);
		}

		{
			Tga::ScriptPin outputPin = {};
			outputPin.type = Tga::ScriptLinkType::Flow;
			outputPin.name = "OnEnter"_tgaid;
			outputPin.node = context.GetNodeId();
			outputPin.role = Tga::ScriptPinRole::Output;

			myOnEnter = context.FindOrCreatePin(outputPin);
		}

		{
			Tga::ScriptPin outputPin = {};
			outputPin.type = Tga::ScriptLinkType::Flow;
			outputPin.name = "OnExit"_tgaid;
			outputPin.node = context.GetNodeId();
			outputPin.role = Tga::ScriptPinRole::Output;

			myOnExit = context.FindOrCreatePin(outputPin);
		}
	}

	// this is a bit wasteful, this node is set up to run each frame, but only triggers its output if a plate is pressed or released
	// a better design would be to somehow register it as an observer and only schedule it for running if needed, with ScriptRuntimeInstance::TriggerPin
	Tga::ScriptNodeResult Execute(Tga::ScriptExecutionContext& context, Tga::ScriptPinId) const override
	{
		GameScriptUpdateContext* gameContext = dynamic_cast<GameScriptUpdateContext*>(&context.GetUpdateContext());

		if (gameContext)
		{
			Tga::Property posProperty = context.ReadInputPin(myPositionToCheck);
			Tga::Vector2f pos = *posProperty.Get<Tga::Vector2f>();

			Tga::Vector2i intPos = { (int)round(pos.x), (int)round(pos.y) };

			bool isNearby = gameContext->gameWorld->IsPlayerNearby(intPos);

			IsPlayerNearbyNodeState& state = GetRuntimeData(context);

			if (isNearby && !state.wasNearby)
			{
				context.TriggerOutputPin(myOnEnter);
			}

			if (!isNearby && state.wasNearby)
			{
				context.TriggerOutputPin(myOnExit);
			}

			state.wasNearby = isNearby;
		}

		return Tga::ScriptNodeResult::KeepRunning;
	}

	bool ShouldExecuteAtStart() const override { return true; }
};


class ShowMessageNode : public Tga::ScriptNodeBase
{
	Tga::ScriptPinId myMessage;

	Tga::ScriptPinId myInputFlow;

public:
	void Init(const Tga::ScriptCreationContext& context) override
	{
		{
			Tga::ScriptPin pin = {};
			pin.type = Tga::ScriptLinkType::Flow;
			pin.name = ""_tgaid;
			pin.node = context.GetNodeId();
			pin.role = Tga::ScriptPinRole::Input;

			myInputFlow = context.FindOrCreatePin(pin);
		}

		{
			Tga::ScriptPin pin = {};
			pin.type = Tga::ScriptLinkType::Property;
			pin.dataType = Tga::GetPropertyType<Tga::StringId>();
			pin.defaultValue = Tga::Property::Create<Tga::StringId>();
			pin.name = "Message"_tgaid;
			pin.node = context.GetNodeId();
			pin.role = Tga::ScriptPinRole::Input;

			myMessage = context.FindOrCreatePin(pin);
		}

	}

	Tga::ScriptNodeResult Execute(Tga::ScriptExecutionContext& context, Tga::ScriptPinId) const override
	{
		GameScriptUpdateContext* gameContext = dynamic_cast<GameScriptUpdateContext*>(&context.GetUpdateContext());

		if (gameContext)
		{
			Tga::Property msgProperty = context.ReadInputPin(myMessage);
			Tga::StringId msg = *msgProperty.Get<Tga::StringId>();

			gameContext->gameWorld->SetMessage(msg);
		}

		return Tga::ScriptNodeResult::Finished;
	}
};

void RegisterGameNodes()
{
	Tga::ScriptNodeTypeRegistry::RegisterType<CheckPressurePlateNode>("Game/CheckPressurePlate", "Used for checking if a pressure plate is pressed");
	Tga::ScriptNodeTypeRegistry::RegisterType<IsPlayerNearbyNode>("Game/IsPlayerNearby", "Checks if the player is on the cell or one away");
	Tga::ScriptNodeTypeRegistry::RegisterType<ShowMessageNode>("Game/ShowMessage", "Writes a message to screen");

}