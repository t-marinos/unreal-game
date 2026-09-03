#include "GameplayTestToolsetSubsystem.h"

#include "Editor.h"
#include "GameplayTestToolset.h"
#include "Subsystems/SubsystemCollection.h"
#include "ToolsetRegistry/ToolsetRegistrySubsystem.h"
#include "ToolsetRegistry/UToolsetRegistry.h"

static bool bEnableGameplayTestToolset = true;

static void OnEnableGameplayTestToolsetChanged(IConsoleVariable* Variable)
{
	if (GEditor == nullptr)
	{
		return;
	}

	if (UGameplayTestToolsetSubsystem* Subsystem = GEditor->GetEditorSubsystem<UGameplayTestToolsetSubsystem>())
	{
		Subsystem->SetToolsetEnabled(Variable->GetBool());
	}
}

static FAutoConsoleVariableRef CVarEnableGameplayTestToolset(
	TEXT("GameplayTestToolset.Enable"),
	bEnableGameplayTestToolset,
	TEXT("Enable or disable GameplayTestToolset registration. When disabled, its tools will not appear in the MCP tool list."),
	FConsoleVariableDelegate::CreateStatic(&OnEnableGameplayTestToolsetChanged),
	ECVF_Default);

void UGameplayTestToolsetSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// Ensure UToolsetRegistrySubsystem is initialized before we call RegisterToolsetClass.
	// RegisterToolsetClass silently no-ops if the registry subsystem is not yet ready.
	Collection.InitializeDependency<UToolsetRegistrySubsystem>();
	SetToolsetEnabled(bEnableGameplayTestToolset);
}

void UGameplayTestToolsetSubsystem::Deinitialize()
{
	SetToolsetEnabled(false);
	Super::Deinitialize();
}

void UGameplayTestToolsetSubsystem::SetToolsetEnabled(bool bEnabled)
{
	if (bEnabled && !bToolsetRegistered)
	{
		UToolsetRegistry::RegisterToolsetClass(UGameplayTestToolset::StaticClass());
		bToolsetRegistered = true;
	}
	else if (!bEnabled && bToolsetRegistered)
	{
		UToolsetRegistry::UnregisterToolsetClass(UGameplayTestToolset::StaticClass());
		bToolsetRegistered = false;
	}
}
