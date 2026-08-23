#include "AlsasuaGameState.h"
#include "Net/UnrealNetwork.h"

AAlsasuaGameState::AAlsasuaGameState()
{
    PrimaryActorTick.bCanEverTick = false;
}

ETimeOfDay AAlsasuaGameState::GetCurrentPeriod() const
{
    if (TimeOfDay < 5.0f) return ETimeOfDay::Night;
    if (TimeOfDay < 7.0f) return ETimeOfDay::Dawn;
    if (TimeOfDay < 12.0f) return ETimeOfDay::Morning;
    if (TimeOfDay < 14.0f) return ETimeOfDay::Noon;
    if (TimeOfDay < 18.0f) return ETimeOfDay::Afternoon;
    if (TimeOfDay < 21.0f) return ETimeOfDay::Evening;
    return ETimeOfDay::Night;
}

void AAlsasuaGameState::SetCrowdTension(float NewTension)
{
    CrowdTension = FMath::Clamp(NewTension, 0.0f, 1.0f);
}

void AAlsasuaGameState::SetTimeOfDay(float NewTime)
{
    TimeOfDay = FMath::Fmod(NewTime, 24.0f);
}

void AAlsasuaGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAlsasuaGameState, TimeOfDay);
    DOREPLIFETIME(AAlsasuaGameState, CrowdTension);
}
