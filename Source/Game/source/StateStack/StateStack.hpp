#pragma once
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "State.hpp"

class StateStack
{
public:
	using StatePtr = std::unique_ptr<State>;
	using StateList = std::vector<StatePtr>;

private:
	std::vector<StateList> stateStack;
public:
	void PushStack(StateList aStateStack)
	{
		stateStack.push_back(std::move(aStateStack));
	}
	void PopStack()
	{
		if (!stateStack.empty())
		{
			stateStack.pop_back();
		}
	}
	StateList* GetCurrentStateStack()
	{
		if (!stateStack.empty())
		{
			return &stateStack.back();
		}
		return nullptr;
	}
	bool IsEmpty() const
	{
		return stateStack.empty();
	}
	bool IsStackEmpty()
	{
		return stateStack.back().empty();
	}

	State* GetCurrentState()
	{
		if (!stateStack.empty() && !stateStack.back().empty())
		{
			return stateStack.back().back().get();
		}
		return nullptr;
	}
	State* PushState(StatePtr aState)
	{
		if (stateStack.empty() || !aState)
		{
			return nullptr;
		}

		State* state = aState.get();
		stateStack.back().push_back(std::move(aState));
		return state;
	}
	void PopState()
	{
		if (stateStack.empty() || stateStack.back().size() < 2)
		{
			std::cout << "[WARNING] TRIED TO POP A STACK WITH ONLY ONE STATE!!!" << std::endl;
			return;
		}
		if (!stateStack.empty() && !stateStack.back().empty())
		{
			stateStack.back().pop_back();
			if (stateStack.back().empty())
			{
				stateStack.pop_back();
			}
			/*if (!stateStack.empty() && !stateStack.back().empty())
			{
				Camera = *stateStack.back().back()->GetCameraSystem();
				Essentials::SetPlayer(*stateStack.back().back()->GetPlayer());
			}*/
		}
	}

	void GetStateName(eState aState)
	{
		switch (aState)
		{
		case eState::eMainMenu:
			//
			return;
		case eState::eOptions:
			//
			return;
		case eState::ePlaying:
			//
			return;
		case eState::ePopState:
			//
			return;
		case eState::ePopStack:
			//
			return;
		default:
			//
			return;
		}
	}
};
