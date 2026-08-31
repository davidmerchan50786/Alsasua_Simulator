#include "World/AlsasuaVisualEffectsManager.h"
#include "World/AlsasuaVFXManager.h"
#include "CargarMaterialComun.h"
#include "World/Time/TimeOfDayManager.h"
#include "Services/IWeatherService.h"
#include "AlsasuaServiceRegistry.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Engine/World.h"

void UAlsasuaVisualEffectsManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<UTimeOfDayManager>();

	CachedMPC = CargarMPCClima();

	// MPC del paquete DZ_Assets para el viento de árboles (MF_SimpleTreeWind).
	MPCWind = LoadObject<UMaterialParameterCollection>(
		nullptr, TEXT("/Game/DZ_Assets/DZ_Common/MPC/MPC_Wind_Control.MPC_Wind_Control"));
}

UAlsasuaVFXManager* UAlsasuaVisualEffectsManager::GetVFXManager() const
{
	UWorld* W = GetWorld();
	return W && W->GetGameInstance()
		? W->GetGameInstance()->GetSubsystem<UAlsasuaVFXManager>()
		: nullptr;
}

void UAlsasuaVisualEffectsManager::Tick(float DeltaTime)
{
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
	UWorld* W0 = GetWorld();
	UAlsasuaServiceRegistry* Reg0 = W0 ? UAlsasuaServiceRegistry::Get(W0) : nullptr;
	IWeatherService* Weather0 = Reg0 ? Reg0->PedirComo<IWeatherService>(FName("Weather")) : nullptr;
	const float RainInt = Weather0 ? Weather0->GetRainIntensity() : 0.f;
	const float Vis = Weather0 ? Weather0->GetVisibilityMultiplier() : 1.f;
	const bool bRaining = RainInt > 0.1f;
	const bool bFoggy = Vis < 0.5f;

	float TargetWetness = 0.f;
	if (bRaining) TargetWetness = 1.f;
	else if (bFoggy) TargetWetness = 0.3f;

	const float Speed = (TargetWetness > GlobalWetness) ? WetnessRainSpeed : WetnessDrySpeed;
	GlobalWetness = FMath::FInterpTo(GlobalWetness, TargetWetness, DeltaTime, Speed);

	PuddleAmount = (GlobalWetness > PuddleFormationThreshold)
		? FMath::FInterpTo(PuddleAmount, (GlobalWetness - PuddleFormationThreshold) / (1.f - PuddleFormationThreshold), DeltaTime, 0.5f)
		: FMath::FInterpTo(PuddleAmount, 0.f, DeltaTime, 0.3f);

	RoadWearAmount = FMath::FInterpTo(RoadWearAmount, GlobalWetness * 0.3f, DeltaTime, 0.2f);

	// Partículas de lluvia en torno al jugador (GameInstanceSubsystem).
	// El threshold interno de 0.05 apaga la pieza cuando escampa.
	if (UAlsasuaVFXManager* VFX = GetVFXManager())
		VFX->SpawnRainParticles(RainInt);
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

	// Read wind from weather subsystem
	UWorld* W1 = GetWorld();
	UAlsasuaServiceRegistry* Reg1 = W1 ? UAlsasuaServiceRegistry::Get(W1) : nullptr;
	IWeatherService* Weather1 = Reg1 ? Reg1->PedirComo<IWeatherService>(FName("Weather")) : nullptr;

	const float WeatherWindSpd = Weather1 ? Weather1->GetWindSpeed() : 0.f;
	const float WeatherWindNorm = FMath::Clamp(WeatherWindSpd / 30.f, 0.f, 1.f);

	// Gust overlay
	const float Gust = FMath::Sin(TimeAccumulator * WindGustFrequency * 6.283f) * WindGustAmplitude;
	const float Noise = FMath::Sin(TimeAccumulator * 0.7f) * 0.1f;

	// Blend weather base wind with procedural gusts
	const float TargetWind = FMath::Clamp(WeatherWindNorm + Gust + Noise, 0.05f, 1.f);
	WindIntensity = FMath::FInterpTo(WindIntensity, TargetWind, DeltaTime, 2.0f);

	if (Weather1 && Weather1->GetRainIntensity() > 0.8f)
	{
		WindIntensity = FMath::Max(WindIntensity, 0.8f);
	}

	// Hojas arrastradas por el viento en torno al jugador. WeatherWindSpd en km/h:
	// el threshold interno (5.0) deja quietas las hojas con viento flojo.
	if (UAlsasuaVFXManager* VFX = GetVFXManager())
		VFX->SpawnLeafParticles(WeatherWindSpd);
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

	// Propagar viento al MPC de DZ_Assets (MF_SimpleTreeWind lee de aquí).
	if (MPCWind)
	{
		if (UMaterialParameterCollectionInstance* WI = W->GetParameterCollectionInstance(MPCWind))
		{
			WI->SetScalarParameterValue(FName("WindIntensity"), WindIntensity);
			WI->SetScalarParameterValue(FName("WindDirection"), WindDirection);
		}
	}
}
