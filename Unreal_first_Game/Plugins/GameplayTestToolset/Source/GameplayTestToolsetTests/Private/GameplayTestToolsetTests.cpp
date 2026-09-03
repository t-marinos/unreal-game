#include "CQTest.h"

#include "Components/ActorTestSpawner.h"
#include "Editor.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTestToolset.h"
#include "GameplayTestToolsetSubsystem.h"
#include "InputAction.h"
#include "ToolsetRegistry/UToolsetRegistry.h"

TEST_CLASS(GameplayTestToolsetTest, "AI.Toolsets.GameplayTestToolset")
{
	TEST_METHOD(Subsystem_IsAvailable)
	{
		UGameplayTestToolsetSubsystem* Subsystem = GEditor->GetEditorSubsystem<UGameplayTestToolsetSubsystem>();
		ASSERT_THAT(IsNotNull(Subsystem));
	}

	TEST_METHOD(Subsystem_RegistersToolsetByDefault)
	{
		// The subsystem registers UGameplayTestToolset in Initialize when the
		// GameplayTestToolset.Enable CVar is true (the default).
		ASSERT_THAT(IsTrue(UToolsetRegistry::IsToolsetClassRegistered(UGameplayTestToolset::StaticClass())));
	}

	// RaiseScriptError does not surface as a catchable automation error when the tool is
	// called directly from C++ instead of through the Blueprint VM (a limitation already
	// documented against this same engine version's UMGToolSetTest), so these error-path
	// tests confirm the tool returns safely rather than asserting the error was logged.

	TEST_METHOD(TriggerInputAction_NullController_DoesNotCrash)
	{
		UGameplayTestToolset::TriggerInputAction(nullptr, nullptr);
	}

	TEST_METHOD(TriggerInputAction_NullAction_WithValidController_DoesNotCrash)
	{
		FActorTestSpawner Spawner;
		APlayerController& Controller = Spawner.SpawnActor<APlayerController>();
		UGameplayTestToolset::TriggerInputAction(&Controller, nullptr);
	}

	TEST_METHOD(TriggerInputAction_NoLocalPlayer_DoesNotCrash)
	{
		FActorTestSpawner Spawner;
		APlayerController& Controller = Spawner.SpawnActor<APlayerController>();
		UInputAction& Action = Spawner.SpawnObject<UInputAction>();
		ASSERT_THAT(IsNull(Controller.GetLocalPlayer()));
		UGameplayTestToolset::TriggerInputAction(&Controller, &Action);
	}

	TEST_METHOD(ExecCommand_NullController_DoesNotCrash)
	{
		UGameplayTestToolset::ExecCommand(nullptr, TEXT("SomeCommand"));
	}

	TEST_METHOD(ExecCommand_EmptyCommand_WithValidController_DoesNotCrash)
	{
		FActorTestSpawner Spawner;
		APlayerController& Controller = Spawner.SpawnActor<APlayerController>();
		UGameplayTestToolset::ExecCommand(&Controller, TEXT(""));
	}

	TEST_METHOD(ExecCommand_NoLocalPlayer_DoesNotCrash)
	{
		FActorTestSpawner Spawner;
		APlayerController& Controller = Spawner.SpawnActor<APlayerController>();
		ASSERT_THAT(IsNull(Controller.GetLocalPlayer()));
		UGameplayTestToolset::ExecCommand(&Controller, TEXT("SomeCommand"));
	}
};
