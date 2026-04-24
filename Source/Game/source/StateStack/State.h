#pragma once
#include <SimpleInput.h>

enum class eState
{
    eMainMenu,
    eOptions,
    ePlaying,
    ePopState,
    ePopStack,
    COUNT
};

class State
{
    friend class StateStack;

public:
	eState virtual Update() = 0;
	void virtual Render() = 0;


};