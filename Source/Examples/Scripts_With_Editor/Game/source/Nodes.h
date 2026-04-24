#pragma once
#include "tge/script/Contexts/ScriptUpdateContext.h"

struct Entity;
class GameWorld;

void RegisterGameNodes();

// we're inheriting ScriptUpdateContext and adding data to it, to be able to access more context from nodes
struct GameScriptUpdateContext : public Tga::ScriptUpdateContext
{
	GameWorld* gameWorld;
	Entity* currentEntity;
};


