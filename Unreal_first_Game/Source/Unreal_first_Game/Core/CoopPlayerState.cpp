#include "Core/CoopPlayerState.h"
#include "Net/UnrealNetwork.h"

void ACoopPlayerState::SetInvulnerable(bool bNewInvulnerable)
{
	if (!HasAuthority())
	{
		return;
	}
	bInvulnerable = bNewInvulnerable;
}

void ACoopPlayerState::SetRole(EPlayerRole NewRole)
{
	if (!HasAuthority())
	{
		return;
	}
	PlayerRole = NewRole;
}

void ACoopPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACoopPlayerState, bInvulnerable);
	DOREPLIFETIME(ACoopPlayerState, PlayerRole);
}
