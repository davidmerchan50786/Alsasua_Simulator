#include "World/AlsasuaNightLightingSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "World/Time/TimeOfDayManager.h"

UAlsasuaNightLightingSystem::UAlsasuaNightLightingSystem()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.5f;
}

void UAlsasuaNightLightingSystem::BeginPlay()
{
    Super::BeginPlay();
}


void UAlsasuaNightLightingSystem::UpdateNightFactor()
{
    UWorld* World = GetWorld();
    if (!World) return;

    // Mismo reloj que el sol y que el alumbrado. Antes salía de
    // GetServerWorldTimeSeconds(), un tiempo sin relación con la hora del
    // mundo: las ventanas se encendían con el sol en lo alto.
    const UTimeOfDayManager* TimeMgr = World->GetSubsystem<UTimeOfDayManager>();
    if (!TimeMgr) return;

    CurrentHour = TimeMgr->CurrentTime;

    // Rampa continua en el crepúsculo, envolviendo la medianoche.
    const float HoursIntoNight = FMath::Fmod(CurrentHour - SunsetHour + 24.f, 24.f);
    const float NightLength = FMath::Fmod(SunriseHour - SunsetHour + 24.f, 24.f);
    const float FadeSpan = FMath::Max(TransitionDuration, 0.01f);

    if (HoursIntoNight > NightLength)
    {
        NightFactor = 0.f;   // es de día
    }
    else
    {
        const float FadeIn = FMath::Clamp(HoursIntoNight / FadeSpan, 0.f, 1.f);
        const float FadeOut = FMath::Clamp((NightLength - HoursIntoNight) / FadeSpan, 0.f, 1.f);
        NightFactor = FMath::Min(FadeIn, FadeOut);
    }
}


void UAlsasuaNightLightingSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    UpdateNightFactor();
}
