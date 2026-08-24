#include "Unreal_first_Game.h"
#include "Modules/ModuleManager.h"

// Empty primary game module — Build 0 M1 just proves the C++ module compiles and loads.
// Core gameplay classes (GameMode, GameState, etc.) are added in M2 onward, under Core/.
IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, Unreal_first_Game, "Unreal_first_Game");
