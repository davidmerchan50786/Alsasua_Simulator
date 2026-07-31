#include "World/AlsasuaAtmosphereFogComponent.h"
#include "World/Time/TimeOfDayManager.h"
#include "World/AlsasuaVisualEffectsManager.h"
#include "World/Weather/WeatherSubsystem.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/World.h"

UAlsasuaAtmosphereFogComponent::UAlsasuaAtmosphereFogComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.2f;
}

void UAlsasuaAtmosphereFogComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UAlsasuaAtmosphereFogComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateFog(DeltaTime);
}

void UAlsasuaAtmosphereFogComponent::UpdateFog(float DeltaTime)
{
	UWorld* W = GetWorld();
	if (!W) return;

	UTimeOfDayManager* TimeMgr = W->GetSubsystem<UTimeOfDayManager>();
	UAlsasuaVisualEffectsManager* VFXMgr = W->GetSubsystem<UAlsasuaVisualEffectsManager>();
	UWeatherSubsystem* Weather = W->GetSubsystem<UWeatherSubsystem>();

	if (!TimeMgr || !VFXMgr) return;

	const float Hour = TimeMgr->CurrentTime;
	const float Wetness = VFXMgr->GlobalWetness;

	// Day/night fog density blend
	float TargetDensity = FogDensityDay;
	FLinearColor TargetColor = FogColorDay;

	if (Hour >= 18.f || Hour < 6.f)
	{
		TargetDensity = FogDensityNight;
		TargetColor = FogColorNight;
	}
	else if (Hour >= 5.f && Hour < 7.f)
	{
		const float T = (Hour - 5.f) / 2.f;
		TargetDensity = FMath::Lerp(FogDensityNight, FogDensityDay, T);
		TargetColor = FMath::Lerp(FogColorNight, FogColorDawn, T);
	}
	else if (Hour >= 17.f && Hour < 19.f)
	{
		const float T = (Hour - 17.f) / 2.f;
		TargetDensity = FMath::Lerp(FogDensityDay, FogDensityNight, T);
		TargetColor = FMath::Lerp(FogColorDawn, FogColorNight, T);
	}

	// Rain boost
	if (Weather && (Weather->CurrentWeather == EWeatherSubsystemState::Rainy ||
		Weather->CurrentWeather == EWeatherSubsystemState::Thunderstorm))
	{
		TargetDensity *= 3.f;
	}
	else if (Weather && Weather->CurrentWeather == EWeatherSubsystemState::HeavyFog)
	{
		TargetDensity *= 5.f;
	}

	// Wetness boost
	TargetDensity *= (1.f + Wetness * 0.5f);

	CurrentFogDensity = FMath::FInterpTo(CurrentFogDensity, TargetDensity, DeltaTime, 1.f);

	// Valley fog: boost at low altitudes
	AActor* Owner = GetOwner();
	if (Owner)
	{
		const float Altitude = Owner->GetActorLocation().Z;
		if (Altitude < ValleyFogMaxAltitude && Altitude > ValleyFogMinAltitude)
		{
			const float ValleyFactor = 1.f - (Altitude - ValleyFogMinAltitude) /
				(ValleyFogMaxAltitude - ValleyFogMinAltitude);
			CurrentFogDensity += ValleyFogDensity * ValleyFactor;
		}
	}

	VFXMgr->FogDensityMult = CurrentFogDensity / FMath::Max(FogDensityDay, 0.0001f);
}
