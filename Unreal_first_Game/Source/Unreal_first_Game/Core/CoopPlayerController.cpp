#include "Core/CoopPlayerController.h"
#include "Core/GameConstants.h"
#include "Core/CoopGameState.h"
#include "Core/CoopPlayerState.h"
#include "Core/CoopGameMode.h"
#include "Core/CoopCharacter.h"
#include "Core/CoopHealthComponent.h"
#include "Abilities/CoopTankAbilities.h"
#include "Abilities/CoopControlAbilities.h"
#include "Abilities/CoopSupportAbilities.h"
#include "Abilities/CoopRunnerAbilities.h"
#include "Abilities/CoopDamageAbilities.h"
#include "Core/CoopDownedComponent.h"
#include "Core/CoopMonsterCharacter.h"
#include "Core/CoopTargetRing.h"
#include "Tags/CoopGameplayTags.h"
#include "Dev/DummyAIController.h"
#include "Camera/CoopOrbitCamera.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"
#include "EngineUtils.h"

ACoopPlayerController::ACoopPlayerController()
{
	// Without this, APlayerController::OnPossess snaps the view target back to the possessed pawn
	// on every (re)possession (see AutoManageActiveCameraTarget), which would silently undo
	// BeginPlay's SetViewTarget(OrbitCamera) below the moment a pawn is possessed. We still want
	// our own ACoopOrbitCamera as the view target even though it now follows the pawn (CLAUDE.md
	// §5, DECISIONS.md's "Camera follows the player" entry) -- it tracks the pawn's location
	// itself in Tick, not by handing the engine the pawn as the view target directly, so a
	// dev-mode Possess() swap doesn't rip the view away to whatever the newly-possessed pawn's
	// own default view would be.
	bAutoManageActiveCameraTarget = false;
}

void ACoopPlayerController::Server_PressButton_Implementation()
{
	// Server-only, per CLAUDE.md §4.1: this is where intent becomes a result. The RPC itself
	// carries no payload beyond "this player pressed a button" -- ACoopGameState is the single
	// source of truth every client's cosmetic response reads from afterward.
	if (ACoopGameState* CoopGameState = GetWorld()->GetGameState<ACoopGameState>())
	{
		CoopGameState->ToggleButtonPressed();
	}
}

void ACoopPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Local-only camera (CLAUDE.md §5): only the machine actually controlling this specific
	// PlayerController should spawn one. On a Listen Server there are up to 5 PlayerController
	// instances with HasAuthority()==true, but only the host's own is IsLocalController() here;
	// each remote client likewise only ever has its own single PlayerController, always local to
	// itself. This keeps the camera per-client with zero cross-client effect, by construction.
	if (!IsLocalController())
	{
		return;
	}

	OrbitCamera = GetWorld()->SpawnActor<ACoopOrbitCamera>();
	if (OrbitCamera)
	{
		OrbitCamera->Initialize(this, GameConstants);
		SetViewTarget(OrbitCamera);
	}

	// Cursor-targeting feature (cursor_progress.md): the mouse cursor is visible for the whole match
	// and input is GameAndUI, so a click can both hit-test the world (SelectTargetUnderCursor) and
	// drive UMG buttons (RoleSelect). This REPLACES UCoopRoleSelectWidget's old per-phase
	// cursor/input-mode toggle -- that block was deleted; cursor ownership lives here now.
	// DoNotLock + SetHideCursorDuringCapture(false) so the right-click-drag orbit camera (which
	// reads the raw mouse delta in ACoopOrbitCamera::Tick) keeps working with the cursor shown.
	SetShowMouseCursor(true);
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}

	// M6: one shared visible timer (CLAUDE.md §7). The widget itself only ever reads
	// ACoopGameState::GetElapsedMatchTime() -- no gameplay state lives here, purely local display.
	if (MatchTimerWidgetClass)
	{
		MatchTimerWidget = CreateWidget<UUserWidget>(this, MatchTimerWidgetClass);
		if (MatchTimerWidget)
		{
			MatchTimerWidget->AddToViewport();
		}
	}

	// Build 1, M5: role-select screen and prep-arena HUD (ability cards + synergy hint). Both are
	// created once here and stay in the viewport for the whole match, same reasoning as
	// MatchTimerWidget above -- each one's own Visibility binding hides it outside its phase.
	if (RoleSelectWidgetClass)
	{
		RoleSelectWidget = CreateWidget<UUserWidget>(this, RoleSelectWidgetClass);
		if (RoleSelectWidget)
		{
			RoleSelectWidget->AddToViewport();
		}
	}

	if (PrepArenaHUDWidgetClass)
	{
		PrepArenaHUDWidget = CreateWidget<UUserWidget>(this, PrepArenaHUDWidgetClass);
		if (PrepArenaHUDWidget)
		{
			PrepArenaHUDWidget->AddToViewport();
		}
	}

	// Build 1: bottom-screen ability bar. UCoopActionBarWidget keeps itself hidden until the Prep
	// phase, same self-gating-on-replicated-phase pattern as RoleSelectWidget/PrepArenaHUDWidget.
	if (ActionBarWidgetClass)
	{
		ActionBarWidget = CreateWidget<UUserWidget>(this, ActionBarWidgetClass);
		if (ActionBarWidget)
		{
			ActionBarWidget->AddToViewport();
		}
	}

	// Cursor-targeting feature (cursor_progress.md): top-left target frame + always-on 5-row party
	// stack. Same create-once / leave-in-viewport pattern; each UCoopUnitFrameWidget row self-gates.
	if (TargetFrameWidgetClass)
	{
		TargetFrameWidget = CreateWidget<UUserWidget>(this, TargetFrameWidgetClass);
		if (TargetFrameWidget)
		{
			TargetFrameWidget->AddToViewport();
		}
	}

	if (PartyFrameWidgetClass)
	{
		PartyFrameWidget = CreateWidget<UUserWidget>(this, PartyFrameWidgetClass);
		if (PartyFrameWidget)
		{
			PartyFrameWidget->AddToViewport();
		}
	}

	// Ability kit expansion: centre-screen toast ("Please choose a target"). Self-gates via
	// RenderOpacity in its own NativeTick -- created here with nothing to show, same pattern as
	// every widget above.
	if (ToastWidgetClass)
	{
		ToastWidget = CreateWidget<UUserWidget>(this, ToastWidgetClass);
		if (ToastWidget)
		{
			ToastWidget->AddToViewport();
		}
	}

	// Local-only ground ring under the current target. Unset class -> no ring (cursor_progress.md
	// decision #3 can slip without blocking the frames).
	if (TargetRingClass)
	{
		TargetRing = GetWorld()->SpawnActor<ACoopTargetRing>(TargetRingClass);
		if (TargetRing)
		{
			const float RingRadius = GameConstants ? GameConstants->TargetRingRadiusUnits : 90.0f;
			const float GroundOffset = GameConstants ? GameConstants->TargetRingGroundOffsetUnits : 88.0f;
			TargetRing->Initialize(this, RingRadius, GroundOffset);
		}
	}
}

void ACoopPlayerController::DumpGameState()
{
	const ACoopGameState* CoopGameState = GetWorld() ? GetWorld()->GetGameState<ACoopGameState>() : nullptr;
	if (!CoopGameState)
	{
		UE_LOG(LogTemp, Warning, TEXT("DumpGameState: no GameState yet."));
		return;
	}

	FString Dump = TEXT("{\n");
	Dump += FString::Printf(TEXT("  \"HasAuthority\": %s,\n"), HasAuthority() ? TEXT("true") : TEXT("false"));
	Dump += FString::Printf(TEXT("  \"ElapsedMatchTime\": %.2f,\n"), CoopGameState->GetElapsedMatchTime());
	Dump += FString::Printf(TEXT("  \"ButtonPressed\": %s,\n"), CoopGameState->IsButtonPressed() ? TEXT("true") : TEXT("false"));
	Dump += TEXT("  \"Players\": [\n");

	const TArray<TObjectPtr<APlayerState>>& Players = CoopGameState->PlayerArray;
	for (int32 Index = 0; Index < Players.Num(); ++Index)
	{
		const APlayerState* PS = Players[Index];
		if (!PS)
		{
			continue;
		}

		FVector Location = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		if (const APawn* PlayerPawn = PS->GetPawn())
		{
			Location = PlayerPawn->GetActorLocation();
			Velocity = PlayerPawn->GetVelocity();
		}

		Dump += FString::Printf(
			TEXT("    { \"PlayerId\": %d, \"Name\": \"%s\", \"PingMs\": %.0f, \"Location\": {\"X\": %.1f, \"Y\": %.1f, \"Z\": %.1f}, \"Velocity\": {\"X\": %.1f, \"Y\": %.1f, \"Z\": %.1f} }%s\n"),
			PS->GetPlayerId(),
			*PS->GetPlayerName(),
			PS->GetPingInMilliseconds(),
			Location.X, Location.Y, Location.Z,
			Velocity.X, Velocity.Y, Velocity.Z,
			(Index < Players.Num() - 1) ? TEXT(",") : TEXT("")
		);
	}

	Dump += TEXT("  ]\n}");

	UE_LOG(LogTemp, Log, TEXT("DumpGameState:\n%s"), *Dump);
}

void ACoopPlayerController::PossessDummy(int32 Index)
{
	Server_PossessDummy(Index);
}

void ACoopPlayerController::Server_PossessDummy_Implementation(int32 Index)
{
	TArray<ADummyAIController*> Dummies;
	for (TActorIterator<ADummyAIController> It(GetWorld()); It; ++It)
	{
		Dummies.Add(*It);
	}

	if (!Dummies.IsValidIndex(Index))
	{
		UE_LOG(LogTemp, Warning, TEXT("PossessDummy: index %d out of range (found %d dummies)."), Index, Dummies.Num());
		return;
	}

	APawn* DummyPawn = Dummies[Index]->GetPawn();
	if (!DummyPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("PossessDummy: dummy %d has no pawn."), Index);
		return;
	}

	// Swap: this controller takes the dummy's pawn, and the dummy controller takes back this
	// controller's old pawn (so it isn't left standing uncontrolled) -- CLAUDE.md §7's
	// "Possess/UnPossess swap (built-in Unreal API, no custom prediction needed)".
	APawn* PreviousPawn = GetPawn();
	Possess(DummyPawn);
	if (PreviousPawn)
	{
		Dummies[Index]->Possess(PreviousPawn);
		Dummies[Index]->SetBehavior(EDummyBehavior::Idle);
	}
}

void ACoopPlayerController::SceneSkip()
{
	// Stub -- CLAUDE.md §7: Build 0 has no scenes yet. Real and working, wired up now so Build 1
	// only needs to give it content, not build the command itself.
	UE_LOG(LogTemp, Log, TEXT("SceneSkip: stub, no scenes exist yet (Build 1 will wire this up)."));
}

void ACoopPlayerController::ToggleGodMode()
{
	Server_ToggleGodMode();
}

void ACoopPlayerController::Server_ToggleGodMode_Implementation()
{
	if (ACoopPlayerState* CoopPS = GetPlayerState<ACoopPlayerState>())
	{
		CoopPS->SetInvulnerable(!CoopPS->IsInvulnerable());
		UE_LOG(LogTemp, Log, TEXT("ToggleGodMode: %s is now %s."), *CoopPS->GetPlayerName(), CoopPS->IsInvulnerable() ? TEXT("invulnerable") : TEXT("vulnerable"));
	}
}

void ACoopPlayerController::Server_ClaimRole_Implementation(EPlayerRole DesiredRole)
{
	ACoopPlayerState* CoopPS = GetPlayerState<ACoopPlayerState>();
	ACoopGameMode* CoopGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACoopGameMode>() : nullptr;
	if (CoopPS && CoopGameMode)
	{
		CoopGameMode->TryClaimRole(CoopPS, DesiredRole);
	}
}

void ACoopPlayerController::ApplyTestDamage(float Amount)
{
	Server_ApplyTestDamage(Amount);
}

void ACoopPlayerController::Server_ApplyTestDamage_Implementation(float Amount)
{
	if (ACoopCharacter* CoopCharacter = Cast<ACoopCharacter>(GetPawn()))
	{
		if (UCoopHealthComponent* Health = CoopCharacter->GetHealthComponent())
		{
			Health->ApplyDamage(Amount);
			UE_LOG(LogTemp, Log, TEXT("ApplyTestDamage: %s took %.1f, now %.1f/%.1f."),
				*GetNameSafe(CoopCharacter), Amount, Health->GetCurrentHealth(), Health->GetMaxHealth());
		}
	}
}

void ACoopPlayerController::ActivateShield()
{
	// Client-side cooldown gate: "Ability not ready" instead of a wasted RPC / silent nothing. No
	// client ROLE gate here -- a wrong-role player's Shield cooldown is always -1 (never set), so
	// this is a silent no-op for them and Server_ActivateShield's own role gate is the real guard.
	if (const ACoopCharacter* C = Cast<ACoopCharacter>(GetPawn()))
	{
		if (!IsAbilityReady(C->GetShieldCooldownEndServerTime()))
		{
			ShowToast(NSLOCTEXT("CoopAbilities", "AbilityNotReady", "Ability not ready"));
			return;
		}
	}
	Server_ActivateShield();
}

void ACoopPlayerController::Server_ActivateShield_Implementation()
{
	// Shield is Tank-only. A non-Tank pressing the key is a harmless no-op -- friends, not
	// adversarial input (CLAUDE.md §8) -- rather than something that needs client-side UI gating in
	// Build 1's stub-only ability cards for the other roles.
	const ACoopPlayerState* CoopPS = GetPlayerState<ACoopPlayerState>();
	ACoopCharacter* CoopCharacter = Cast<ACoopCharacter>(GetPawn());
	if (!CoopPS || !CoopCharacter || CoopPS->GetRole() != EPlayerRole::Tank)
	{
		return;
	}

	// Build 1, M9: Downed characters can't use abilities (CLAUDE.md §6.6).
	if (CoopCharacter->HasStatusTag(CoopGameplayTags::Status_Downed))
	{
		return;
	}

	CoopTankAbilities::ApplyShield(CoopCharacter, GameConstants);
}

void ACoopPlayerController::ActivateStabilize()
{
	// Client-side cooldown gate -- see ActivateShield. Wrong-role Stabilize cooldown is always -1.
	if (const ACoopCharacter* C = Cast<ACoopCharacter>(GetPawn()))
	{
		if (!IsAbilityReady(C->GetStabilizeCooldownEndServerTime()))
		{
			ShowToast(NSLOCTEXT("CoopAbilities", "AbilityNotReady", "Ability not ready"));
			return;
		}
	}
	Server_ActivateStabilize();
}

void ACoopPlayerController::Server_ActivateStabilize_Implementation()
{
	// Stabilize is Control-only -- same "friends, not adversarial input" no-op reasoning as
	// Server_ActivateShield above (CLAUDE.md §8).
	const ACoopPlayerState* CoopPS = GetPlayerState<ACoopPlayerState>();
	ACoopCharacter* CoopCharacter = Cast<ACoopCharacter>(GetPawn());
	if (!CoopPS || !CoopCharacter || CoopPS->GetRole() != EPlayerRole::Control)
	{
		return;
	}

	// Build 1, M9: Downed characters can't use abilities (CLAUDE.md §6.6).
	if (CoopCharacter->HasStatusTag(CoopGameplayTags::Status_Downed))
	{
		return;
	}

	CoopControlAbilities::ResolveStabilize(CoopCharacter, GameConstants);
}

void ACoopPlayerController::Server_AttemptRevive_Implementation()
{
	ACoopCharacter* Reviver = Cast<ACoopCharacter>(GetPawn());
	if (!Reviver || Reviver->HasStatusTag(CoopGameplayTags::Status_Downed))
	{
		// A Downed player can't revive anyone -- CLAUDE.md §6.6.
		return;
	}

	const float RadiusUnits = GameConstants ? GameConstants->ReviveRadiusUnits : 150.0f;
	const FVector ReviverLocation = Reviver->GetActorLocation();

	ACoopCharacter* NearestDowned = nullptr;
	float NearestDistSq = FMath::Square(RadiusUnits);

	for (TActorIterator<ACoopCharacter> It(GetWorld()); It; ++It)
	{
		ACoopCharacter* Other = *It;
		if (!Other || Other == Reviver || !Other->HasStatusTag(CoopGameplayTags::Status_Downed))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(ReviverLocation, Other->GetActorLocation());
		if (DistSq <= NearestDistSq)
		{
			NearestDistSq = DistSq;
			NearestDowned = Other;
		}
	}

	if (NearestDowned && NearestDowned->GetDownedComponent())
	{
		NearestDowned->GetDownedComponent()->BeginRevive(Reviver);
	}
}

void ACoopPlayerController::ActivateSpeed()
{
	// Client-side cooldown gate -- see ActivateShield. Wrong-role Speed cooldown is always -1.
	if (const ACoopCharacter* C = Cast<ACoopCharacter>(GetPawn()))
	{
		if (!IsAbilityReady(C->GetSpeedCooldownEndServerTime()))
		{
			ShowToast(NSLOCTEXT("CoopAbilities", "AbilityNotReady", "Ability not ready"));
			return;
		}
	}
	Server_ActivateSpeed();
}

void ACoopPlayerController::Server_ActivateSpeed_Implementation()
{
	// Speed is Support-only -- same "friends, not adversarial input" no-op reasoning as
	// Server_ActivateShield above (CLAUDE.md §8).
	const ACoopPlayerState* CoopPS = GetPlayerState<ACoopPlayerState>();
	ACoopCharacter* CoopCharacter = Cast<ACoopCharacter>(GetPawn());
	if (!CoopPS || !CoopCharacter || CoopPS->GetRole() != EPlayerRole::Support)
	{
		return;
	}

	if (CoopCharacter->HasStatusTag(CoopGameplayTags::Status_Downed))
	{
		return;
	}

	CoopSupportAbilities::ApplySpeed(CoopCharacter, GameConstants);
}

void ACoopPlayerController::ActivateDash()
{
	// Client-side cooldown gate -- see ActivateShield. Wrong-role Dash cooldown is always -1.
	if (const ACoopCharacter* C = Cast<ACoopCharacter>(GetPawn()))
	{
		if (!IsAbilityReady(C->GetDashCooldownEndServerTime()))
		{
			ShowToast(NSLOCTEXT("CoopAbilities", "AbilityNotReady", "Ability not ready"));
			return;
		}
	}
	Server_ActivateDash();
}

void ACoopPlayerController::Server_ActivateDash_Implementation()
{
	// Dash is Runner-only.
	const ACoopPlayerState* CoopPS = GetPlayerState<ACoopPlayerState>();
	ACoopCharacter* CoopCharacter = Cast<ACoopCharacter>(GetPawn());
	if (!CoopPS || !CoopCharacter || CoopPS->GetRole() != EPlayerRole::Runner)
	{
		return;
	}

	if (CoopCharacter->HasStatusTag(CoopGameplayTags::Status_Downed))
	{
		return;
	}

	CoopRunnerAbilities::ResolveDash(CoopCharacter, GameConstants);
}

void ACoopPlayerController::ActivateExecution()
{
	// Client-side ROLE gate: IA_Execution shares the Q key with the four other first-abilities, so
	// this wrapper fires on every role's Q press. Bail silently for a non-Damage player -- without
	// this they see a spurious "Please choose a target" every time they press Q for their own
	// ability. Server_ActivateExecution_Implementation still role-gates authoritatively.
	const ACoopPlayerState* PS = GetPlayerState<ACoopPlayerState>();
	if (!PS || PS->GetRole() != EPlayerRole::Damage)
	{
		return;
	}

	// Client-side cooldown gate -- fires before the target check (a not-ready ability is the more
	// fundamental blocker). CoopDamageAbilities::ResolveExecution re-checks the cooldown server-side.
	if (const ACoopCharacter* C = Cast<ACoopCharacter>(GetPawn()))
	{
		if (!IsAbilityReady(C->GetExecutionCooldownEndServerTime()))
		{
			ShowToast(NSLOCTEXT("CoopAbilities", "AbilityNotReady", "Ability not ready"));
			return;
		}
	}

	// Target-required (DECISIONS.md "Target-required abilities need a click-selected target"). Gate
	// client-side: there is no point sending an RPC the server will only reject for a missing
	// target, and this is where the "Please choose a target" feedback belongs. The server still
	// re-validates the target it *does* receive (Server_ActivateExecution_Implementation below).
	AActor* Target = GetCurrentTargetActor();
	if (!Target)
	{
		ShowToast(NSLOCTEXT("CoopAbilities", "ChooseTarget", "Please choose a target"));
		return;
	}
	Server_ActivateExecution(Target);
}

void ACoopPlayerController::Server_ActivateExecution_Implementation(AActor* Target)
{
	// Execution is Damage-only.
	const ACoopPlayerState* CoopPS = GetPlayerState<ACoopPlayerState>();
	ACoopCharacter* CoopCharacter = Cast<ACoopCharacter>(GetPawn());
	if (!CoopPS || !CoopCharacter || CoopPS->GetRole() != EPlayerRole::Damage)
	{
		return;
	}

	if (CoopCharacter->HasStatusTag(CoopGameplayTags::Status_Downed))
	{
		return;
	}

	// Target arrives as client intent; CoopDamageAbilities::ResolveExecution re-validates its
	// type / range / Vulnerable tag (CLAUDE.md §4.1).
	CoopDamageAbilities::ResolveExecution(CoopCharacter, Target, GameConstants);
}

void ACoopPlayerController::ActivateArmorBreak()
{
	// Role + cooldown + target gates, identical shape to ActivateExecution above (Tank-only).
	const ACoopPlayerState* PS = GetPlayerState<ACoopPlayerState>();
	if (!PS || PS->GetRole() != EPlayerRole::Tank)
	{
		return;
	}

	if (const ACoopCharacter* C = Cast<ACoopCharacter>(GetPawn()))
	{
		if (!IsAbilityReady(C->GetArmorBreakCooldownEndServerTime()))
		{
			ShowToast(NSLOCTEXT("CoopAbilities", "AbilityNotReady", "Ability not ready"));
			return;
		}
	}

	AActor* Target = GetCurrentTargetActor();
	if (!Target)
	{
		ShowToast(NSLOCTEXT("CoopAbilities", "ChooseTarget", "Please choose a target"));
		return;
	}
	Server_ActivateArmorBreak(Target);
}

void ACoopPlayerController::Server_ActivateArmorBreak_Implementation(AActor* Target)
{
	// Armor Break is Tank-only.
	const ACoopPlayerState* CoopPS = GetPlayerState<ACoopPlayerState>();
	ACoopCharacter* CoopCharacter = Cast<ACoopCharacter>(GetPawn());
	if (!CoopPS || !CoopCharacter || CoopPS->GetRole() != EPlayerRole::Tank)
	{
		return;
	}

	if (CoopCharacter->HasStatusTag(CoopGameplayTags::Status_Downed))
	{
		return;
	}

	CoopTankAbilities::ResolveArmorBreak(CoopCharacter, Target, GameConstants);
}

void ACoopPlayerController::ActivateOverload()
{
	// Role + cooldown + target gates, identical shape to ActivateExecution above (Damage-only).
	const ACoopPlayerState* PS = GetPlayerState<ACoopPlayerState>();
	if (!PS || PS->GetRole() != EPlayerRole::Damage)
	{
		return;
	}

	if (const ACoopCharacter* C = Cast<ACoopCharacter>(GetPawn()))
	{
		if (!IsAbilityReady(C->GetOverloadCooldownEndServerTime()))
		{
			ShowToast(NSLOCTEXT("CoopAbilities", "AbilityNotReady", "Ability not ready"));
			return;
		}
	}

	AActor* Target = GetCurrentTargetActor();
	if (!Target)
	{
		ShowToast(NSLOCTEXT("CoopAbilities", "ChooseTarget", "Please choose a target"));
		return;
	}
	Server_ActivateOverload(Target);
}

void ACoopPlayerController::Server_ActivateOverload_Implementation(AActor* Target)
{
	// Overload is Damage-only.
	const ACoopPlayerState* CoopPS = GetPlayerState<ACoopPlayerState>();
	ACoopCharacter* CoopCharacter = Cast<ACoopCharacter>(GetPawn());
	if (!CoopPS || !CoopCharacter || CoopPS->GetRole() != EPlayerRole::Damage)
	{
		return;
	}

	if (CoopCharacter->HasStatusTag(CoopGameplayTags::Status_Downed))
	{
		return;
	}

	CoopDamageAbilities::ResolveOverload(CoopCharacter, Target, GameConstants);
}

bool ACoopPlayerController::IsAbilityReady(float CooldownEndServerTime) const
{
	// Client-side check only, for the pre-RPC "Ability not ready" toast. GetServerWorldTimeSeconds()
	// works client-side (CLAUDE.md §4.5) and *CooldownEndServerTime replicates COND_OwnerOnly to
	// this pawn's own client. -1 (never cast) reads as ready. The server re-checks authoritatively.
	const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	const float Now = GS ? GS->GetServerWorldTimeSeconds() : 0.0f;
	return Now >= CooldownEndServerTime;
}

void ACoopPlayerController::ShowToast(const FText& Message)
{
	// Local, cosmetic (CLAUDE.md §4.2). UCoopToastWidget::NativeTick reads these two fields and
	// fades itself; nothing here is replicated or sent to the server.
	PendingToastText = Message;
	PendingToastStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
}

void ACoopPlayerController::ApplyTestVulnerable()
{
	Server_ApplyTestVulnerable();
}

void ACoopPlayerController::Server_ApplyTestVulnerable_Implementation()
{
	if (!GetPawn() || !GetWorld())
	{
		return;
	}

	const float RangeUnits = GameConstants ? GameConstants->TestVulnerableRangeUnits : 1000.0f;
	const float DurationSeconds = GameConstants ? GameConstants->TestVulnerableDurationSeconds : 6.0f;
	const FVector MyLocation = GetPawn()->GetActorLocation();

	ACoopMonsterCharacter* NearestMonster = nullptr;
	float NearestDistSq = FMath::Square(RangeUnits);

	for (TActorIterator<ACoopMonsterCharacter> It(GetWorld()); It; ++It)
	{
		ACoopMonsterCharacter* Monster = *It;
		if (!Monster)
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(MyLocation, Monster->GetActorLocation());
		if (DistSq <= NearestDistSq)
		{
			NearestDistSq = DistSq;
			NearestMonster = Monster;
		}
	}

	if (!NearestMonster)
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyTestVulnerable: no ACoopMonsterCharacter within %.0f units."), RangeUnits);
		return;
	}

	NearestMonster->ApplyStatusTag(CoopGameplayTags::Status_Vulnerable_Physical, DurationSeconds);
	UE_LOG(LogTemp, Log, TEXT("ApplyTestVulnerable: granted Status.Vulnerable.Physical to %s for %.1fs."), *GetNameSafe(NearestMonster), DurationSeconds);
}

void ACoopPlayerController::ApplyTestVulnerableMagic()
{
	Server_ApplyTestVulnerableMagic();
}

void ACoopPlayerController::Server_ApplyTestVulnerableMagic_Implementation()
{
	// Dev/test only -- an explicit copy of Server_ApplyTestVulnerable, keyed to the Magic branch, so
	// Damage's Overload has something to read before "The Heart" (Scene 5) exists. DELETE both when
	// that scene lands (DECISIONS.md).
	if (!GetPawn() || !GetWorld())
	{
		return;
	}

	const float RangeUnits = GameConstants ? GameConstants->TestVulnerableRangeUnits : 1000.0f;
	const float DurationSeconds = GameConstants ? GameConstants->TestVulnerableDurationSeconds : 6.0f;
	const FVector MyLocation = GetPawn()->GetActorLocation();

	ACoopMonsterCharacter* NearestMonster = nullptr;
	float NearestDistSq = FMath::Square(RangeUnits);

	for (TActorIterator<ACoopMonsterCharacter> It(GetWorld()); It; ++It)
	{
		ACoopMonsterCharacter* Monster = *It;
		if (!Monster)
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(MyLocation, Monster->GetActorLocation());
		if (DistSq <= NearestDistSq)
		{
			NearestDistSq = DistSq;
			NearestMonster = Monster;
		}
	}

	if (!NearestMonster)
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyTestVulnerableMagic: no ACoopMonsterCharacter within %.0f units."), RangeUnits);
		return;
	}

	NearestMonster->ApplyStatusTag(CoopGameplayTags::Status_Vulnerable_Magic, DurationSeconds);
	UE_LOG(LogTemp, Log, TEXT("ApplyTestVulnerableMagic: granted Status.Vulnerable.Magic to %s for %.1fs."), *GetNameSafe(NearestMonster), DurationSeconds);
}

void ACoopPlayerController::SelectTargetUnderCursor()
{
	// Local-only (CLAUDE.md §4.2): this only decides what THIS player's target frame / party stack /
	// ground ring draw. No RPC, no replicated state -- the server never learns or needs it.
	FHitResult Hit;
	if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		AActor* HitActor = Hit.GetActor();
		if (Cast<ACoopCharacter>(HitActor) || Cast<ACoopMonsterCharacter>(HitActor))
		{
			CurrentTargetActor = HitActor;
			return;
		}
	}

	// Clicked the ground / a wall / nothing selectable -> clear (cursor_progress.md decision #2).
	ClearTarget();
}

void ACoopPlayerController::SetCurrentTarget(AActor* NewTarget)
{
	// Local-only (CLAUDE.md §4.2), same as SelectTargetUnderCursor -- just skips the cursor trace
	// because the caller (a clicked party-frame row) already has the actor. Accept only the two
	// selectable types; ignore anything else rather than clearing.
	if (Cast<ACoopCharacter>(NewTarget) || Cast<ACoopMonsterCharacter>(NewTarget))
	{
		CurrentTargetActor = NewTarget;
	}
}

void ACoopPlayerController::ClearTarget()
{
	CurrentTargetActor = nullptr;
}
