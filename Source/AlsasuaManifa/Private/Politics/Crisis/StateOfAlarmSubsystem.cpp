#include "Politics/Crisis/StateOfAlarmSubsystem.h"

void UStateOfAlarmSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CurrentLevel = EStateOfAlarmLevel::Normal;
}

void UStateOfAlarmSubsystem::UpdateState(float StateInfluence, float ProtestIntensity)
{
    EStateOfAlarmLevel NewLevel = EStateOfAlarmLevel::Normal;

    // Lógica de escalada sistémica:
    // Si la influencia es baja y la protesta es alta, el estado entra en pánico (Toque de queda)
    if (StateInfluence < 20.f && ProtestIntensity > 0.7f)
    {
        NewLevel = EStateOfAlarmLevel::Curfew;
    }
    else if (StateInfluence < 40.f || ProtestIntensity > 0.4f)
    {
        NewLevel = EStateOfAlarmLevel::StateOfAlarm;
    }
    else if (StateInfluence < 60.f)
    {
        NewLevel = EStateOfAlarmLevel::HighVigilance;
    }

    if (NewLevel != CurrentLevel)
    {
        CurrentLevel = NewLevel;
        OnAlarmLevelChanged.Broadcast(CurrentLevel);
        UE_LOG(LogTemp, Error, TEXT("EL ESTADO HA CAMBIADO EL NIVEL DE ALARMA A: %d"), (int32)CurrentLevel);
    }
}
