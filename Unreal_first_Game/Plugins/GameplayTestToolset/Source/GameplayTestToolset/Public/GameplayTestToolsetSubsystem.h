#pragma once

#include "EditorSubsystem.h"

#include "GameplayTestToolsetSubsystem.generated.h"

class FSubsystemCollectionBase;

/**
 * Registers GameplayTestToolset with UToolsetRegistry for the lifetime of the editor.
 * Registration can be toggled at runtime via the GameplayTestToolset.Enable CVar.
 */
UCLASS(MinimalAPI)
class UGameplayTestToolsetSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

	/** Register or unregister the toolset with UToolsetRegistry at runtime. */
	void SetToolsetEnabled(bool bEnabled);

private:

	bool bToolsetRegistered = false;
};
