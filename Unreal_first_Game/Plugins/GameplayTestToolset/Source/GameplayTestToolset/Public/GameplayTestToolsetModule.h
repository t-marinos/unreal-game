// GameplayTestToolset module. Registration lives in GameplayTestToolsetSubsystem, not here --
// same split as the engine's own LiveCodingToolset plugin.

#pragma once

#include "Modules/ModuleInterface.h"

class FGameplayTestToolsetModule : public IModuleInterface
{
public:

	virtual void StartupModule() override;

	virtual void ShutdownModule() override;
};
