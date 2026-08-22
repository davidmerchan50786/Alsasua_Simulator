#include "World/AlsasuaProceduralAudio.h"
#include "World/Time/TimeOfDayManager.h"
#include "World/Weather/WeatherSubsystem.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

UAlsasuaProceduralAudio::UAlsasuaProceduralAudio()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.2f;
}

void UAlsasuaProceduralAudio::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner) return;

	// Create audio components
	if (WindSoundCalm)
	{
		WindAudio = UGameplayStatics::SpawnSoundAttached(WindSoundCalm, Owner->GetRootComponent());
		if (WindAudio) WindAudio->SetVolumeMultiplier(0.f);
	}

	if (BirdSound)
	{
		BirdAudio = UGameplayStatics::SpawnSoundAttached(BirdSound, Owner->GetRootComponent());
		if (BirdAudio) BirdAudio->SetVolumeMultiplier(0.f);
	}

	if (RiverSound)
	{
		RiverAudio = UGameplayStatics::SpawnSoundAttached(RiverSound, Owner->GetRootComponent());
		if (RiverAudio) RiverAudio->SetVolumeMultiplier(0.f);
	}

	if (RainAmbience)
	{
		RainAudio = UGameplayStatics::SpawnSoundAttached(RainAmbience, Owner->GetRootComponent());
		if (RainAudio) RainAudio->SetVolumeMultiplier(0.f);
	}

	if (CricketSound)
	{
		CricketAudio = UGameplayStatics::SpawnSoundAttached(CricketSound, Owner->GetRootComponent());
		if (CricketAudio) CricketAudio->SetVolumeMultiplier(0.f);
	}
}

void UAlsasuaProceduralAudio::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateAudioLayers(DeltaTime);
}

void UAlsasuaProceduralAudio::UpdateAudioLayers(float DeltaTime)
{
	UWorld* W = GetWorld();
	if (!W) return;

	UTimeOfDayManager* TimeMgr = W->GetSubsystem<UTimeOfDayManager>();
	UWeatherSubsystem* Weather = W->GetSubsystem<UWeatherSubsystem>();

	if (!TimeMgr) return;

	const float Hour = TimeMgr->CurrentTime;
	const bool bRaining = Weather && (Weather->CurrentWeather == EWeatherSubsystemState::Rainy ||
		Weather->CurrentWeather == EWeatherSubsystemState::Thunderstorm);
	const bool bIsNight = Hour >= CricketStartHour || Hour < CricketEndHour;
	const bool bIsDay = Hour >= BirdStartHour && Hour < BirdEndHour;

	// Wind approximation from weather state (no VEM dependency)
	float Wind = 0.3f;
	if (Weather && Weather->CurrentWeather == EWeatherSubsystemState::Thunderstorm) Wind = 0.9f;
	else if (bRaining) Wind = 0.6f;

	// --- Wind ---
	const float TargetWindVol = FMath::Lerp(0.1f, WindVolumeMax, Wind);
	CurrentWindVolume = FMath::FInterpTo(CurrentWindVolume, TargetWindVol, DeltaTime, WindFadeSpeed);
	if (WindAudio) WindAudio->SetVolumeMultiplier(CurrentWindVolume);

	// --- Birds ---
	float TargetBirdVol = 0.f;
	if (bIsDay && !bRaining)
	{
		TargetBirdVol = BirdVolumeMax * FMath::Lerp(BirdDensityUrban, BirdDensityNature, 0.5f);
	}
	CurrentBirdVolume = FMath::FInterpTo(CurrentBirdVolume, TargetBirdVol, DeltaTime, 0.3f);
	if (BirdAudio) BirdAudio->SetVolumeMultiplier(CurrentBirdVolume);

	// --- River ---
	CurrentRiverVolume = FMath::FInterpTo(CurrentRiverVolume, RiverVolumeMax, DeltaTime, 0.3f);
	if (RiverAudio) RiverAudio->SetVolumeMultiplier(CurrentRiverVolume);

	// --- Rain ---
	const float TargetRainVol = bRaining ? RainVolumeMax : 0.f;
	CurrentRainVolume = FMath::FInterpTo(CurrentRainVolume, TargetRainVol, DeltaTime, 0.5f);
	if (RainAudio) RainAudio->SetVolumeMultiplier(CurrentRainVolume);

	// --- Crickets ---
	float TargetCricketVol = 0.f;
	if (bIsNight && !bRaining)
	{
		TargetCricketVol = CricketVolumeMax;
	}
	CurrentCricketVolume = FMath::FInterpTo(CurrentCricketVolume, TargetCricketVol, DeltaTime, 0.3f);
	if (CricketAudio) CricketAudio->SetVolumeMultiplier(CurrentCricketVolume);
}
