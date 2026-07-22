#include "World/Time/TimeOfDayManager.h"

void UTimeOfDayManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UTimeOfDayManager::Deinitialize()
{
    Super::Deinitialize();
}

void UTimeOfDayManager::UpdateTime(float DeltaTime)
{
    CurrentTime += DeltaTime * TimeSpeed;
    if (CurrentTime >= 24.0f) CurrentTime = 0.0f;

    int32 CurrentHour = FMath::FloorToInt(CurrentTime);
    if (CurrentHour != LastHour)
    {
        LastHour = CurrentHour;
        OnHourChanged.Broadcast(CurrentHour);
    }

    bool bIsNightNow = IsNight();
    if (bIsNightNow != bWasNight)
    {
        bWasNight = bIsNightNow;
        OnNightStarted.Broadcast(bIsNightNow);
    }
}

FString UTimeOfDayManager::GetFormattedTime() const
{
    int32 Hours = FMath::FloorToInt(CurrentTime);
    int32 Minutes = FMath::FloorToInt((CurrentTime - Hours) * 60.0f);
    return FString::Printf(TEXT("%02d:%02d"), Hours, Minutes);
}
