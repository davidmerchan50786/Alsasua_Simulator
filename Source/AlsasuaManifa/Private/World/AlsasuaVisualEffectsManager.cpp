#include "World/AlsasuaVisualEffectsManager.h"
#include "World/Time/TimeOfDayManager.h"
#include "World/Weather/WeatherSubsystem.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Engine/World.h"

void UAlsasuaVisualEffectsManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<UTimeOfDayManager>();
	Collection.InitializeDependency<UWeatherSubsystem>();

	CachedMPC = LoadObject<UMaterialParameterCollection>(
		nullptr, TEXT("/Game/Materials/MPC_AlsasuaGlobal.MPC_AlsasuaGlobal"));
}

void UAlsasuaVisualEffectsManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateWetness(DeltaTime);
	UpdateTimeOfDay();
	UpdateWind(DeltaTime);
	UpdateNightEmissive();
	UpdateMPC(DeltaTime);
}

void UAlsasuaVisualEffectsManager::SetRainIntensity(float Intensity)
{
	GlobalWetness = FMath::Clamp(Intensity, 0.f, 1.f);
}

void UAlsasuaVisualEffectsManager::SetSnowIntensity(float Intensity)
{
	SnowAmount = FMath::Clamp(Intensity, 0.f, 1.f);
}

void UAlsasuaVisualEffectsManager::SetWind(float Intensity, float Direction)
{
	WindIntensity = FMath::Clamp(Intensity, 0.f, 1.f);
	WindDirection = Direction;
}

void UAlsasuaVisualEffectsManager::SetRoadWear(float Wear)
{
	RoadWearAmount = FMath::Clamp(Wear, 0.f, 1.f);
}

void UAlsasuaVisualEffectsManager::UpdateWetness(float DeltaTime)
{
	UWeatherSubsystem* Weather = GetWorld() ? GetWorld()->GetSubsystem<UWeatherSubsystem>() : nullptr;
	const bool bRaining = Weather && (Weather->CurrentWeather == EWeatherState::Rainy || Weather->CurrentWeather == EWeatherState::Thunderstorm);
	const bool bFoggy = Weather && Weather->CurrentWeather == EWeatherState::HeavyFog;

	float TargetWetness = 0.f;
	if (bRaining) TargetWetness = 1.f;
	else if (bFoggy) TargetWetness = 0.3f;

	const float Speed = (TargetWetness > GlobalWetness) ? WetnessRainSpeed : WetnessDrySpeed;
	GlobalWetness = FMath::FInterpTo(GlobalWetness, TargetWetness, DeltaTime, Speed);

	PuddleAmount = (GlobalWetness > PuddleFormationThreshold)
		? FMath::FInterpTo(PuddleAmount, (GlobalWetness - PuddleFormationThreshold) / (1.f - PuddleFormationThreshold), DeltaTime, 0.5f)
		: FMath::FInterpTo(PuddleAmount, 0.f, DeltaTime, 0.3f);

	RoadWearAmount = FMath::FInterpTo(RoadWearAmount, GlobalWetness * 0.3f, DeltaTime, 0.2f);
}

void UAlsasuaVisualEffectsManager::UpdateTimeOfDay()
{
	UTimeOfDayManager* TimeMgr = GetWorld() ? GetWorld()->GetSubsystem<UTimeOfDayManager>() : nullptr;
	if (!TimeMgr) return;

	NormalizedTime = TimeMgr->CurrentTime / 24.f;

	const float Hour = TimeMgr->CurrentTime;
	if (Hour >= 6.f && Hour <= 20.f)
	{
		const float MidDay = 13.f;
		const float Dist = FMath::Abs(Hour - MidDay) / 7.f;
		DayNightBlend = FMath::Clamp(1.f - Dist * 0.3f, 0.7f, 1.f);
	}
	else
	{
		DayNightBlend = 0.f;
	}
}

void UAlsasuaVisualEffectsManager::UpdateWind(float DeltaTime)
{
	TimeAccumulator += DeltaTime;

	const float Gust = FMath::Sin(TimeAccumulator * WindGustFrequency * 6.283f) * WindGustAmplitude;
	const float Noise = FMath::Sin(TimeAccumulator * 0.7f) * 0.1f;
	WindIntensity = FMath::Clamp(WindIntensity + (Gust + Noise) * DeltaTime, 0.05f, 1.f);

	UWeatherSubsystem* Weather = GetWorld() ? GetWorld()->GetSubsystem<UWeatherSubsystem>() : nullptr;
	if (Weather && Weather->CurrentWeather == EWeatherState::Thunderstorm)
	{
		WindIntensity = FMath::Max(WindIntensity, 0.8f);
	}
}

void UAlsasuaVisualEffectsManager::UpdateNightEmissive()
{
	UTimeOfDayManager* TimeMgr = GetWorld() ? GetWorld()->GetSubsystem<UTimeOfDayManager>() : nullptr;
	if (!TimeMgr) return;

	const float Hour = TimeMgr->CurrentTime;
	if (Hour >= 18.f || Hour < 6.f)
	{
		NightEmissiveIntensity = FMath::FInterpTo(NightEmissiveIntensity, 1.f, 0.016f, 0.5f);
	}
	else if (Hour >= 6.f && Hour < 8.f)
	{
		const float T = (Hour - 6.f) / 2.f;
		NightEmissiveIntensity = FMath::FInterpTo(NightEmissiveIntensity, 1.f - T, 0.016f, 0.5f);
	}
	else if (Hour >= 16.f && Hour < 18.f)
	{
		const float T = (Hour - 16.f) / 2.f;
		NightEmissiveIntensity = FMath::FInterpTo(NightEmissiveIntensity, T, 0.016f, 0.5f);
	}
	else
	{
		NightEmissiveIntensity = 0.f;
	}
}

void UAlsasuaVisualEffectsManager::UpdateMPC(float DeltaTime)
{
	if (!CachedMPC) return;

	UWorld* W = GetWorld();
	if (!W) return;

	UMaterialParameterCollectionInstance* Inst = W->GetParameterCollectionInstance(CachedMPC);
	if (!Inst) return;

	Inst->SetScalarParameterValue(FName("GlobalWetness"), GlobalWetness);
	Inst->SetScalarParameterValue(FName("PuddleOpacity"), PuddleAmount);
	Inst->SetScalarParameterValue(FName("RainIntensity"), GlobalWetness * 0.8f);
	Inst->SetScalarParameterValue(FName("NormalizedTimeOfDay"), NormalizedTime);
	Inst->SetScalarParameterValue(FName("DayNightBlend"), DayNightBlend);
	Inst->SetScalarParameterValue(FName("WindIntensity"), WindIntensity);
	Inst->SetScalarParameterValue(FName("WindDirection"), WindDirection);
	Inst->SetScalarParameterValue(FName("SnowAmount"), SnowAmount);
	Inst->SetScalarParameterValue(FName("FogDensityMult"), FogDensityMult);
	Inst->SetScalarParameterValue(FName("RoadWearAmount"), RoadWearAmount);
	Inst->SetScalarParameterValue(FName("NightEmissiveIntensity"), NightEmissiveIntensity);

	Inst->SetVectorParameterValue(FName("WindVector"),
		FLinearColor(FMath::Cos(WindDirection), FMath::Sin(WindDirection), 0, WindIntensity));
}
