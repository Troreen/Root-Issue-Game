#pragma once
#include <vector>
#include "State.hpp"

class StateStack
{
private:
	std::vector<std::vector<State*>> stateStack;
public:
	void PushStack(std::vector<State*> aStateStack)
	{
		stateStack.push_back(aStateStack);
	}
	void PopStack()
	{
		if (!stateStack.empty())
		{
			stateStack.pop_back();
		}
	}
	std::vector<State*>* GetCurrentStateStack()
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
			return stateStack.back().back();
		}
		return nullptr;
	}
	void PushState(State* aState)
	{
		if (!stateStack.empty())
		{
			stateStack.back().push_back(aState);
		}
	}
	void PopState()
	{
		if (stateStack.back().size() < 2)
		{
			std::cout << "[WARNING] TRIED TO POP A STACK WITH ONLY ONE STATE!!!" << std::endl;
			return;
		}
		if (!stateStack.empty() && !stateStack.back().empty())
		{
			/*auto& Camera = *stateStack.back().back()->GetCameraSystem();*/
			delete stateStack.back().back();
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