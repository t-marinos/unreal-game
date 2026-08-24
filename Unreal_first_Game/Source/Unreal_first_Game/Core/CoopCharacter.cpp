#include "Core/CoopCharacter.h"
#include "GameFramework/PlayerState.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

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
}

void ACoopCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	ApplyPlayerColorTint();
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
