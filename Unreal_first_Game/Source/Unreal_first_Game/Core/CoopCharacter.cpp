#include "Core/CoopCharacter.h"
#include "Core/CoopHealthComponent.h"
#include "Core/CoopDownedComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Core/CoopStatusBarWidget.h"
#include "Core/GameConstants.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Animation/AnimInstance.h"

ACoopCharacter::ACoopCharacter()
{
	HealthComponent = CreateDefaultSubobject<UCoopHealthComponent>(TEXT("HealthComponent"));
	DownedComponent = CreateDefaultSubobject<UCoopDownedComponent>(TEXT("DownedComponent"));

	// Disabled by default (NoCollision) -- UCoopDownedComponent enables it, sized to
	// ReviveRadiusUnits, only while this character is actually Downed.
	ReviveTriggerVolume = CreateDefaultSubobject<USphereComponent>(TEXT("ReviveTriggerVolume"));
	ReviveTriggerVolume->SetupAttachment(RootComponent);
	ReviveTriggerVolume->SetCollisionProfileName(TEXT("NoCollision"));

	// Structural setup only -- the tunable height offset (GameConstants) and the WidgetClass
	// reference (set on BP_PlayerCharacter's CDO) aren't valid yet at construction time, only from
	// BeginPlay onward. Screen space needs no camera reference at all (see header comment) and
	// SetDrawAtDesiredSize means no separate pixel-size tunable is needed either.
	StatusBarWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("StatusBarWidgetComponent"));
	StatusBarWidgetComponent->SetupAttachment(RootComponent);
	StatusBarWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	StatusBarWidgetComponent->SetDrawAtDesiredSize(true);
}

void ACoopCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Covers the server (PlayerState is already valid by the time BeginPlay runs, since
	// possession happens first) and the locally controlled client's first frame. Remote
	// clients pick up PlayerState later, via OnRep_PlayerState below.
	if (GetPlayerState())
	{
		ApplyPlayerColorTint();
	}

	// Status badge setup. Unlike ApplyPlayerColorTint above, this doesn't depend on PlayerState --
	// `this` is always valid the moment BeginPlay runs, on every net mode -- so a single call site
	// here is correct and sufficient, no OnRep_PlayerState/PossessedBy mirror needed.
	const float HeightOffset = GameConstants ? GameConstants->StatusBarHeightOffsetUnits : 180.0f;
	if (!GameConstants)
	{
		UE_LOG(LogTemp, Warning, TEXT("ACoopCharacter::BeginPlay: GameConstants not set on %s, falling back to 180.0 status bar height offset."), *GetNameSafe(this));
	}
	StatusBarWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, HeightOffset));

	if (UCoopStatusBarWidget* StatusWidget = Cast<UCoopStatusBarWidget>(StatusBarWidgetComponent->GetUserWidgetObject()))
	{
		StatusWidget->SetOwningCharacter(this);
	}
	else
	{
		// Same failure-mode shape as M6's GameConstants lesson (BUILD_1_PROGRESS.md): a null widget
		// here means WidgetClass was never set on BP_PlayerCharacter's CDO -- the component exists
		// but has nothing to show. Logged so this isn't shipped silently broken.
		UE_LOG(LogTemp, Warning, TEXT("ACoopCharacter::BeginPlay: StatusBarWidgetComponent has no widget instance on %s -- is WidgetClass set on BP_PlayerCharacter's CDO?"), *GetNameSafe(this));
	}
}

void ACoopCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	ApplyPlayerColorTint();
}

void ACoopCharacter::PossessedBy(AController* NewController)
{
	// Super sets PlayerState (see APawn::PossessedBy) -- must run first so
	// ApplyPlayerColorTint sees the real PlayerId, not null.
	Super::PossessedBy(NewController);
	ApplyPlayerColorTint();
}

void ACoopCharacter::ApplyStatusTag(FGameplayTag Tag, float DurationSeconds)
{
	if (!HasAuthority())
	{
		return;
	}

	ActiveStatusTags.AddTag(Tag);

	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	const float Now = GameState ? GameState->GetServerWorldTimeSeconds() : 0.0f;
	StatusTagExpiryServerTime.Add(Tag, Now + DurationSeconds);

	// Reapplying before expiry refreshes the timer rather than stacking a second one.
	FTimerHandle& Handle = StatusTagExpiryTimers.FindOrAdd(Tag);
	FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, &ACoopCharacter::RemoveStatusTag, Tag);
	GetWorldTimerManager().SetTimer(Handle, Delegate, DurationSeconds, false);
}

void ACoopCharacter::ApplyPersistentStatusTag(FGameplayTag Tag)
{
	if (!HasAuthority())
	{
		return;
	}

	// No expiry timestamp, no timer -- ActiveStatusTags is the only thing that changes. RemoveTag on
	// a container that never had the tag, or TMap::Remove on a key that was never added, are both
	// safe no-ops, so RemoveStatusTag below works unmodified for a tag applied this way.
	ActiveStatusTags.AddTag(Tag);
}

void ACoopCharacter::RemoveStatusTag(FGameplayTag Tag)
{
	if (!HasAuthority())
	{
		return;
	}

	ActiveStatusTags.RemoveTag(Tag);
	StatusTagExpiryServerTime.Remove(Tag);
	StatusTagExpiryTimers.Remove(Tag);
}

void ACoopCharacter::PlayCastMontage(UAnimMontage* Montage)
{
	if (!HasAuthority() || !Montage)
	{
		return;
	}

	Multicast_PlayCastMontage(Montage);
}

void ACoopCharacter::Multicast_PlayCastMontage_Implementation(UAnimMontage* Montage)
{
	if (!Montage || !GetMesh())
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(Montage);
	}
}

void ACoopCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACoopCharacter, ActiveStatusTags);

	// Owner-only: each player's client only needs its own cooldowns, for its own action bar
	// (UCoopAbilitySlotWidget's WoW-style sweep). Still server-authored -- see CoopCharacter.h.
	DOREPLIFETIME_CONDITION(ACoopCharacter, ShieldCooldownEndServerTime, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ACoopCharacter, StabilizeCooldownEndServerTime, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ACoopCharacter, SpeedCooldownEndServerTime, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ACoopCharacter, DashCooldownEndServerTime, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ACoopCharacter, ExecutionCooldownEndServerTime, COND_OwnerOnly);
}

FLinearColor ACoopCharacter::GetColorForPlayerId(int32 PlayerId)
{
	// Five visually distinct colours, one per player slot (CLAUDE.md §6.1: exactly 5 players).
	// Indexed by PlayerId modulo the array size so it degrades gracefully (repeats) rather than
	// crashing if PlayerId ever exceeds 4.
	static const TArray<FLinearColor> PlayerColors = {
		FLinearColor(1.0f, 0.05f, 0.05f), // Red
		FLinearColor(0.05f, 0.3f, 1.0f),  // Blue
		FLinearColor(0.1f, 0.9f, 0.1f),   // Green
		FLinearColor(1.0f, 0.85f, 0.0f),  // Yellow
		FLinearColor(0.7f, 0.1f, 0.9f),   // Purple
	};

	const int32 Index = ((PlayerId % PlayerColors.Num()) + PlayerColors.Num()) % PlayerColors.Num();
	return PlayerColors[Index];
}

float ACoopCharacter::GetBackpedalSpeedMultiplier() const
{
	return GameConstants ? GameConstants->BackpedalSpeedMultiplier : 1.0f;
}

void ACoopCharacter::ApplyPlayerColorTint()
{
	if (!GetPlayerState() || !GetMesh())
	{
		return;
	}

	const FLinearColor Color = GetColorForPlayerId(GetPlayerState()->GetPlayerId());

	// Both of the Mannequin's material slots (body + extras) expose a "Paint Tint" vector
	// parameter -- apply to every slot so the whole character tints consistently.
	const int32 NumMaterials = GetMesh()->GetNumMaterials();
	for (int32 SlotIndex = 0; SlotIndex < NumMaterials; ++SlotIndex)
	{
		if (UMaterialInstanceDynamic* DynamicMaterial = GetMesh()->CreateAndSetMaterialInstanceDynamic(SlotIndex))
		{
			DynamicMaterial->SetVectorParameterValue(TEXT("Paint Tint"), Color);
		}
	}
}
