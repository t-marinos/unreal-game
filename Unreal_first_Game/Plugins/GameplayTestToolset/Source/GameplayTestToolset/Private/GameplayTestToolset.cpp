#include "GameplayTestToolset.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Kismet/KismetSystemLibrary.h"

void UGameplayTestToolset::TriggerInputAction(APlayerController* Controller, UInputAction* Action)
{
	if (!Controller)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Controller must not be null."));
		return;
	}
	if (!Action)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Action must not be null."));
		return;
	}

	ULocalPlayer* LocalPlayer = Controller->GetLocalPlayer();
	if (!LocalPlayer)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Controller has no LocalPlayer -- pass that client's own client-local PlayerController, not a server-side replica of a remote client."));
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!Subsystem)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("No EnhancedInputLocalPlayerSubsystem found for this player."));
		return;
	}

	Subsystem->InjectInputForAction(Action, FInputActionValue(true), {}, {});
	Subsystem->InjectInputForAction(Action, FInputActionValue(false), {}, {});
}

void UGameplayTestToolset::ExecCommand(APlayerController* Controller, const FString& Command)
{
	if (!Controller)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Controller must not be null."));
		return;
	}
	if (Command.IsEmpty())
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Command must not be empty."));
		return;
	}
	if (!Controller->GetLocalPlayer())
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Controller has no LocalPlayer -- pass that client's own client-local PlayerController, not a server-side replica of a remote client."));
		return;
	}

	UKismetSystemLibrary::ExecuteConsoleCommand(Controller->GetWorld(), Command, Controller);
}
