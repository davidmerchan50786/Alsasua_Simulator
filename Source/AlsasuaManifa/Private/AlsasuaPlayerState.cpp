#include "AlsasuaPlayerState.h"
#include "Net/UnrealNetwork.h"

AAlsasuaPlayerState::AAlsasuaPlayerState()
{
}

void AAlsasuaPlayerState::AddReputation(float Delta)
{
    Reputation = FMath::Clamp(Reputation + Delta, 0.0f, 100.0f);
}

void AAlsasuaPlayerState::IncrementCompletedMissions()
{
    CompletedMissions++;
}

void AAlsasuaPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAlsasuaPlayerState, Reputation);
    DOREPLIFETIME(AAlsasuaPlayerState, CompletedMissions);
}
