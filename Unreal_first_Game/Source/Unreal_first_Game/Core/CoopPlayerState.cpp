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

void ACoopPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACoopPlayerState, bInvulnerable);
}
