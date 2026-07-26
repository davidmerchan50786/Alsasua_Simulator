#include "World/Time/TimeOfDayManager.h"
#include "Engine/World.h"

void UTimeOfDayManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bWasNight = IsNight();
    LastPeriod = GetCurrentPeriod();
    LastHour = FMath::FloorToInt(CurrentTime);
}

void UTimeOfDayManager::Deinitialize()
{
    Super::Deinitialize();
}

void UTimeOfDayManager::UpdateTime(float DeltaTime)
{
    CurrentTime += DeltaTime * TimeSpeed;
    if (CurrentTime >= 24.0f) CurrentTime -= 24.0f;

    const int32 CurrentHour = FMath::FloorToInt(CurrentTime);
    if (CurrentHour != LastHour)
    {
        LastHour = CurrentHour;
        OnHourChanged.Broadcast(CurrentHour);
    }

    const bool bIsNightNow = IsNight();
    if (bIsNightNow != bWasNight)
    {
        bWasNight = bIsNightNow;
        OnNightStarted.Broadcast(bIsNightNow);
    }

    const ETimePeriod NewPeriod = GetCurrentPeriod();
    if (NewPeriod != LastPeriod)
    {
        LastPeriod = NewPeriod;
        static const FName PeriodNames[] = { TEXT("Morning"), TEXT("Day"), TEXT("Evening"), TEXT("Night") };
        OnPeriodChanged.Broadcast(PeriodNames[(int32)NewPeriod]);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  GetFormattedTime
// ─────────────────────────────────────────────────────────────────────────────
FString UTimeOfDayManager::GetFormattedTime() const
{
    const int32 Hours = FMath::FloorToInt(CurrentTime);
    const int32 Minutes = FMath::FloorToInt((CurrentTime - Hours) * 60.0f);
    return FString::Printf(TEXT("%02d:%02d"), Hours, Minutes);
}

// ─────────────────────────────────────────────────────────────────────────────
//  GetCurrentPeriod
// ─────────────────────────────────────────────────────────────────────────────
ETimePeriod UTimeOfDayManager::GetCurrentPeriod() const
{
    if (CurrentTime >= 6.f && CurrentTime < 10.f) return ETimePeriod::Morning;
    if (CurrentTime >= 10.f && CurrentTime < 18.f) return ETimePeriod::Day;
    if (CurrentTime >= 18.f && CurrentTime < 22.f) return ETimePeriod::Evening;
    return ETimePeriod::Night;
}

// ─────────────────────────────────────────────────────────────────────────────
//  GetSunAngle: ángulo del sol para rotar la directional light.
//  0° = amanecer, 90° = mediodía, 180° = atardecer.
// ─────────────────────────────────────────────────────────────────────────────
float UTimeOfDayManager::GetSunAngle() const
{
    // Mapear 6:00-21:00 a 0-180 grados.
    constexpr float DayStart = 6.f;
    constexpr float DayEnd = 21.f;
    constexpr float DayDuration = DayEnd - DayStart;

    if (CurrentTime < DayStart || CurrentTime > DayEnd)
    {
        // Fuera del día: sol bajo el horizonte.
        return -30.f;
    }

    const float DayProgress = (CurrentTime - DayStart) / DayDuration;
    return FMath::Lerp(-10.f, 190.f, DayProgress);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Gameplay queries
// ─────────────────────────────────────────────────────────────────────────────
int32 UTimeOfDayManager::GetGuardDensity() const
{
    int32 Base = DayGuardCount;

    if (IsNight())
    {
        Base += NightGuardBonus;
    }

    if (IsCurfewActive())
    {
        Base += CurfewGuardBonus;
    }

    return Base;
}

bool UTimeOfDayManager::IsCurfewActive() const
{
    return CurrentTime >= 22.f || CurrentTime < 6.f;
}

float UTimeOfDayManager::GetTrafficMultiplier() const
{
    const ETimePeriod Period = GetCurrentPeriod();

    switch (Period)
    {
    case ETimePeriod::Morning:   return 1.2f;  // Hora punta mañana.
    case ETimePeriod::Day:       return MaxTrafficMultiplier;
    case ETimePeriod::Evening:   return 1.0f;
    case ETimePeriod::Night:     return MinTrafficMultiplier;
    default:                     return 1.0f;
    }
}

float UTimeOfDayManager::GetPedestrianMultiplier() const
{
    const ETimePeriod Period = GetCurrentPeriod();

    switch (Period)
    {
    case ETimePeriod::Morning:   return 0.8f;
    case ETimePeriod::Day:       return 1.0f;
    case ETimePeriod::Evening:   return 0.7f;
    case ETimePeriod::Night:     return 0.2f;
    default:                     return 1.0f;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  SetTime
// ─────────────────────────────────────────────────────────────────────────────
void UTimeOfDayManager::SetTime(float NewTime)
{
    CurrentTime = FMath::Fmod(NewTime, 24.0f);
    if (CurrentTime < 0.f) CurrentTime += 24.0f;

    const int32 CurrentHour = FMath::FloorToInt(CurrentTime);
    if (CurrentHour != LastHour)
    {
        LastHour = CurrentHour;
        OnHourChanged.Broadcast(CurrentHour);
    }

    const bool bIsNightNow = IsNight();
    if (bIsNightNow != bWasNight)
    {
        bWasNight = bIsNightNow;
        OnNightStarted.Broadcast(bIsNightNow);
    }

    const ETimePeriod NewPeriod = GetCurrentPeriod();
    if (NewPeriod != LastPeriod)
    {
        LastPeriod = NewPeriod;
        static const FName PeriodNames[] = { TEXT("Morning"), TEXT("Day"), TEXT("Evening"), TEXT("Night") };
        OnPeriodChanged.Broadcast(PeriodNames[(int32)NewPeriod]);
    }
}
