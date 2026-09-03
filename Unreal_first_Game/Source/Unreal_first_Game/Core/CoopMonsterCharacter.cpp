#include "Core/CoopMonsterCharacter.h"
#include "Core/CoopHealthComponent.h"
#include "Core/CoopFixateRetargetComponent.h"
#include "Core/CoopDownedComponent.h"
#include "Core/CoopCharacter.h"
#include "Core/GameConstants.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

ACoopMonsterCharacter::ACoopMonsterCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// Hardcoded engine content, same "flat coloured primitive" bar as every other placeholder actor
	// in this project (CLAUDE.md §5) -- a cone reads as a threat without any custom art.
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMeshFinder(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeMeshFinder.Succeeded())
	{
		Mesh->SetStaticMesh(ConeMeshFinder.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MonsterMaterialFinder(TEXT("/Game/Materials/M_CoopButton.M_CoopButton"));
	if (MonsterMaterialFinder.Succeeded())
	{
		Mesh->SetMaterial(0, MonsterMaterialFinder.Object);
	}
	SetActorScale3D(FVector(1.5f, 1.5f, 2.0f));

	HealthComponent = CreateDefaultSubobject<UCoopHealthComponent>(TEXT("HealthComponent"));
	TargetingComponent = CreateDefaultSubobject<UCoopFixateRetargetComponent>(TEXT("TargetingComponent"));
}

void ACoopMonsterCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Dark red tint to read as a threat, distinct from plates/gate/button's green-or-grey palette.
	if (UMaterialInstanceDynamic* DynamicMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(0))
	{
		DynamicMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.6f, 0.05f, 0.05f));
	}

	if (HasAuthority())
	{
		HealthComponent->OnHealthDepleted.AddDynamic(this, &ACoopMonsterCharacter::HandleHealthDepleted);
	}
}

void ACoopMonsterCharacter::InitializeMonster(const TArray<AActor*>& InitialCandidates)
{
	if (!HasAuthority())
	{
		return;
	}

	const float MaxHealth = GameConstants ? GameConstants->MonsterHealth : 50.0f;
	HealthComponent->SetMaxHealth(MaxHealth);

	TargetingComponent->PickInitialTarget(InitialCandidates);
	BindToTargetDownedDelegate();

	const float AttackIntervalSeconds = GameConstants ? GameConstants->MonsterAttackIntervalSeconds : 2.0f;
	GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &ACoopMonsterCharacter::PerformAttackTick, AttackIntervalSeconds, true);
}

void ACoopMonsterCharacter::BindToTargetDownedDelegate()
{
	ACoopCharacter* TargetCharacter = Cast<ACoopCharacter>(TargetingComponent->GetCurrentTarget());
	if (!TargetCharacter)
	{
		return;
	}

	if (UCoopDownedComponent* Downed = TargetCharacter->GetDownedComponent())
	{
		Downed->OnDownedStateChanged.AddDynamic(this, &ACoopMonsterCharacter::HandleTargetDownedStateChanged);
		BoundTargetForDelegate = TargetCharacter;
	}
}

void ACoopMonsterCharacter::UnbindFromTargetDownedDelegate()
{
	if (ACoopCharacter* PreviouslyBound = BoundTargetForDelegate.Get())
	{
		if (UCoopDownedComponent* Downed = PreviouslyBound->GetDownedComponent())
		{
			Downed->OnDownedStateChanged.RemoveDynamic(this, &ACoopMonsterCharacter::HandleTargetDownedStateChanged);
		}
	}
	BoundTargetForDelegate = nullptr;
}

void ACoopMonsterCharacter::HandleTargetDownedStateChanged()
{
	// OnDownedStateChanged fires on both entering AND leaving Downed -- only entering Downed should
	// trigger a retarget; a revive completing is not this monster's concern.
	ACoopCharacter* CurrentTargetCharacter = BoundTargetForDelegate.Get();
	if (!CurrentTargetCharacter || !CurrentTargetCharacter->GetDownedComponent()->IsDowned())
	{
		return;
	}

	UnbindFromTargetDownedDelegate();

	// A brief pause before actually re-fixating (GameConstants->MonsterFixateSwitchDelaySeconds) --
	// a readable "it hesitates, then locks onto someone else" beat rather than an instant retarget.
	const float SwitchDelaySeconds = GameConstants ? GameConstants->MonsterFixateSwitchDelaySeconds : 0.5f;
	GetWorldTimerManager().SetTimer(RetargetDelayTimerHandle, this, &ACoopMonsterCharacter::PerformRetarget, SwitchDelaySeconds, false);
}

void ACoopMonsterCharacter::PerformRetarget()
{
	TargetingComponent->OnTargetDowned();
	BindToTargetDownedDelegate();
}

void ACoopMonsterCharacter::PerformAttackTick()
{
	if (!HasAuthority())
	{
		return;
	}

	ACoopCharacter* TargetCharacter = Cast<ACoopCharacter>(TargetingComponent->GetCurrentTarget());
	if (!TargetCharacter || !TargetCharacter->GetDownedComponent() || TargetCharacter->GetDownedComponent()->IsDowned())
	{
		// No valid target right now (e.g. everyone currently Downed) -- skip this tick, try again
		// next interval rather than forcing an immediate retarget search here.
		return;
	}

	// Attacks are abstracted as a periodic direct damage tick against the current target -- no
	// physical projectile actor, no range check. Documented simplification, same shape as Shield's
	// own "negates all incoming damage, not just directional" precedent (CoopHealthComponent.cpp);
	// Tank's Shield/Fortress already counter this via HasStatusTag negation inside ApplyDamage.
	const float AttackDamage = GameConstants ? GameConstants->MonsterAttackDamage : 5.0f;
	if (UCoopHealthComponent* TargetHealth = TargetCharacter->GetHealthComponent())
	{
		TargetHealth->ApplyDamage(AttackDamage);
	}
}

void ACoopMonsterCharacter::ApplyStatusTag(FGameplayTag Tag, float DurationSeconds)
{
	if (!HasAuthority())
	{
		return;
	}

	ActiveStatusTags.AddTag(Tag);

	// Reapplying before expiry refreshes the timer rather than stacking a second one -- same
	// pattern as ACoopCharacter::ApplyStatusTag.
	FTimerHandle& Handle = StatusTagExpiryTimers.FindOrAdd(Tag);
	FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, &ACoopMonsterCharacter::RemoveStatusTag, Tag);
	GetWorldTimerManager().SetTimer(Handle, Delegate, DurationSeconds, false);
}

void ACoopMonsterCharacter::RemoveStatusTag(FGameplayTag Tag)
{
	if (!HasAuthority())
	{
		return;
	}

	ActiveStatusTags.RemoveTag(Tag);
	StatusTagExpiryTimers.Remove(Tag);
}

void ACoopMonsterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACoopMonsterCharacter, ActiveStatusTags);
}

void ACoopMonsterCharacter::HandleHealthDepleted()
{
	// M11 scope: monster death just means it stops attacking and vanishes -- no scoring/loot/XP
	// (CLAUDE.md §8, out of scope). Unbind first so a dangling delegate can't fire into an
	// about-to-be-destroyed monster.
	UnbindFromTargetDownedDelegate();
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	GetWorldTimerManager().ClearTimer(RetargetDelayTimerHandle);
	Destroy();
}
