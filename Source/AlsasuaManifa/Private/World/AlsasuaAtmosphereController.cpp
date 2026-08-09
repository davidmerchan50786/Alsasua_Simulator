#include "World/AlsasuaAtmosphereController.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "World/Time/TimeOfDayManager.h"
#include "World/Weather/WeatherSubsystem.h"
#include "Engine/World.h"

void UAlsasuaAtmosphereController::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<UTimeOfDayManager>();

	FindOrCreateAtmosphereActors();
}

void UAlsasuaAtmosphereController::FindOrCreateAtmosphereActors()
{
	UWorld* W = GetWorld();
	if (!W) return;

	for (TActorIterator<ADirectionalLight> It(W); It; ++It)
	{
		SunLight = *It;
		break;
	}
	if (!SunLight)
	{
		FActorSpawnParameters Params;
		SunLight = W->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector::ZeroVector, FRotator(-60, 0, 0), Params);
		if (SunLight)
		{
			SunLight->Rename(TEXT("Atmosphere_Sun"));
			UDirectionalLightComponent* DirComp = Cast<UDirectionalLightComponent>(SunLight->GetLightComponent());
			if (DirComp)
			{
				DirComp->SetIntensity(SunIntensity);
				DirComp->SetLightColor(DaySunColor);
				DirComp->SetCastShadows(true);
			}
		}
	}

	if (SunLight)
	{
		if (USceneComponent* Root = SunLight->GetRootComponent()) Root->SetMobility(EComponentMobility::Movable);
		if (UDirectionalLightComponent* DirComp = Cast<UDirectionalLightComponent>(SunLight->GetLightComponent())) DirComp->SetCastShadows(true);
	}

	for (TActorIterator<ASkyAtmosphere> It(W); It; ++It)
	{
		SkyAtmosphere = *It;
		break;
	}
	if (!SkyAtmosphere)
	{
		FActorSpawnParameters Params;
		SkyAtmosphere = W->SpawnActor<ASkyAtmosphere>(ASkyAtmosphere::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
		if (SkyAtmosphere) SkyAtmosphere->Rename(TEXT("Atmosphere_SkyAtmosphere"));
	}

	for (TActorIterator<ASkyLight> It(W); It; ++It)
	{
		SkyLight = *It;
		break;
	}
	if (!SkyLight)
	{
		FActorSpawnParameters Params;
		SkyLight = W->SpawnActor<ASkyLight>(ASkyLight::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
		if (SkyLight)
		{
			SkyLight->Rename(TEXT("Atmosphere_SkyLight"));
			USkyLightComponent* SLComp = SkyLight->GetLightComponent();
			if (SLComp)
			{
				SLComp->SetIntensity(SkyIntensity);
				SLComp->bRealTimeCapture = true;
			}
		}
	}

	if (SkyLight)
	{
		if (USkyLightComponent* SLComp = SkyLight->GetLightComponent()) SLComp->bRealTimeCapture = true;
	}

	for (TActorIterator<AExponentialHeightFog> It(W); It; ++It)
	{
		HeightFog = *It;
		break;
	}
	if (!HeightFog)
	{
		FActorSpawnParameters Params;
		HeightFog = W->SpawnActor<AExponentialHeightFog>(AExponentialHeightFog::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
		if (HeightFog)
		{
			HeightFog->Rename(TEXT("Atmosphere_Fog"));
			UExponentialHeightFogComponent* FogComp = HeightFog->GetComponent();
			if (FogComp)
			{
				FogComp->SetFogDensity(BaseFogDensity);
				FogComp->SetFogHeightFalloff(FogHeightFalloff);
				FogComp->SetFogInscatteringColor(DayFogColor);
				FogComp->bEnableVolumetricFog = true;
			}
		}
	}
}

void UAlsasuaAtmosphereController::Tick(float DeltaTime)
{
	UTimeOfDayManager* TimeMgr = GetWorld() ? GetWorld()->GetSubsystem<UTimeOfDayManager>() : nullptr;
	if (!TimeMgr) return;

	const float Hour = TimeMgr->CurrentTime;
	UpdateSunVisuals(Hour);
	UpdateFogVisuals(Hour);
	UpdateSkyVisuals(Hour);
	UpdateCloudVisuals(Hour);
	UpdateMoonVisuals(Hour);
}

void UAlsasuaAtmosphereController::SetTimeOfDay(float Hour)
{
	UTimeOfDayManager* TimeMgr = GetWorld() ? GetWorld()->GetSubsystem<UTimeOfDayManager>() : nullptr;
	if (TimeMgr)
	{
		TimeMgr->SetTime(Hour);
	}
	UpdateSunVisuals(Hour);
	UpdateFogVisuals(Hour);
	UpdateSkyVisuals(Hour);
	UpdateCloudVisuals(Hour);
	UpdateMoonVisuals(Hour);
}

void UAlsasuaAtmosphereController::SetSunAngle(float AngleDeg)
{
	if (SunLight)
	{
		SunLight->SetActorRotation(FRotator(-AngleDeg, 30.f, 0.f));
	}
}

void UAlsasuaAtmosphereController::SetCloudDensity(float Density)
{
	CurrentCloudDensity = Density;
}

void UAlsasuaAtmosphereController::SetFogDensity(float Density)
{
	CurrentFogDensity = Density;
}

float UAlsasuaAtmosphereController::GetSunElevation(float Hour) const
{
	constexpr float Dawn = 6.f;
	constexpr float Noon = 12.f;
	constexpr float Dusk = 20.f;

	if (Hour < Dawn || Hour > Dusk)
	{
		return -30.f;
	}

	if (Hour <= Noon)
	{
		const float T = (Hour - Dawn) / (Noon - Dawn);
		return FMath::Lerp(-5.f, 85.f, T);
	}
	else
	{
		const float T = (Hour - Noon) / (Dusk - Noon);
		return FMath::Lerp(85.f, -5.f, T);
	}
}

void UAlsasuaAtmosphereController::UpdateSunVisuals(float Hour)
{
	const float Elevation = GetSunElevation(Hour);
	SetSunAngle(Elevation);

	if (!SunLight || !SunLight->GetLightComponent()) return;

	UDirectionalLightComponent* DirComp = Cast<UDirectionalLightComponent>(SunLight->GetLightComponent());

	float Intensity;
	FLinearColor Color;

	if (Hour >= 5.5f && Hour < 7.5f)
	{
		const float T = (Hour - 5.5f) / 2.0f;
		Color = FLinearColor::LerpUsingHSV(DawnSunColor, DaySunColor, T);
		Intensity = FMath::Lerp(DawnIntensity, SunIntensity, T);
	}
	else if (Hour >= 7.5f && Hour < 17.0f)
	{
		Color = DaySunColor;
		Intensity = SunIntensity;
	}
	else if (Hour >= 17.0f && Hour < 20.0f)
	{
		const float T = (Hour - 17.0f) / 3.0f;
		Color = FLinearColor::LerpUsingHSV(DaySunColor, DuskSunColor, T);
		Intensity = FMath::Lerp(SunIntensity, DawnIntensity, T);
	}
	else if (Hour >= 20.0f && Hour < 21.0f)
	{
		const float T = (Hour - 20.0f);
		Color = FLinearColor::LerpUsingHSV(DuskSunColor, FLinearColor(0.1f, 0.1f, 0.2f), T);
		Intensity = FMath::Lerp(DawnIntensity, NightIntensity, T);
	}
	else
	{
		Color = FLinearColor(0.05f, 0.05f, 0.1f);
		Intensity = NightIntensity;
	}

	DirComp->SetLightColor(Color);
	DirComp->SetIntensity(Intensity);
	CurrentSunColor = Color;
	CurrentSunIntensity = Intensity;
}

void UAlsasuaAtmosphereController::UpdateFogVisuals(float Hour)
{
	if (!HeightFog || !HeightFog->GetComponent()) return;

	UExponentialHeightFogComponent* FogComp = HeightFog->GetComponent();

	UWeatherSubsystem* Weather = GetWorld() ? GetWorld()->GetSubsystem<UWeatherSubsystem>() : nullptr;
	const bool bRaining = Weather && (Weather->CurrentWeather == EWeatherSubsystemState::Rainy || Weather->CurrentWeather == EWeatherSubsystemState::Thunderstorm);
	const bool bFoggy = Weather && Weather->CurrentWeather == EWeatherSubsystemState::HeavyFog;

	float TargetFogDensity = BaseFogDensity;
	FLinearColor TargetFogColor = DayFogColor;

	if (Hour < 6.f || Hour > 20.f)
	{
		TargetFogDensity = NightFogDensity;
		TargetFogColor = NightFogColor;
	}
	else if (Hour >= 5.5f && Hour < 7.5f)
	{
		const float T = (Hour - 5.5f) / 2.0f;
		TargetFogColor = FLinearColor::LerpUsingHSV(DawnFogColor, DayFogColor, T);
		TargetFogDensity = FMath::Lerp(NightFogDensity, BaseFogDensity, T);
	}
	else if (Hour >= 17.0f && Hour < 20.5f)
	{
		const float T = (Hour - 17.0f) / 3.5f;
		TargetFogColor = FLinearColor::LerpUsingHSV(DayFogColor, DawnFogColor, T);
	}

	if (bRaining) TargetFogDensity *= 2.5f;
	if (bFoggy) TargetFogDensity *= 5.f;

	CurrentFogDensity = FMath::FInterpTo(CurrentFogDensity, TargetFogDensity, GetWorld()->GetDeltaSeconds(), 2.f);
	CurrentFogColor = FLinearColor::LerpUsingHSV(CurrentFogColor, TargetFogColor, GetWorld()->GetDeltaSeconds() * 0.5f);

	FogComp->SetFogDensity(CurrentFogDensity);
	FogComp->SetFogInscatteringColor(CurrentFogColor);
}

void UAlsasuaAtmosphereController::UpdateSkyVisuals(float Hour)
{
	if (!SkyLight || !SkyLight->GetLightComponent()) return;

	USkyLightComponent* SLComp = SkyLight->GetLightComponent();

	float TargetIntensity = SkyIntensity;
	if (Hour < 6.f || Hour > 20.f)
	{
		TargetIntensity = NightSkyIntensity;
	}
	else if (Hour >= 5.5f && Hour < 7.5f)
	{
		const float T = (Hour - 5.5f) / 2.0f;
		TargetIntensity = FMath::Lerp(NightSkyIntensity, SkyIntensity, T);
	}
	else if (Hour >= 17.0f && Hour < 20.5f)
	{
		const float T = (Hour - 17.0f) / 3.5f;
		TargetIntensity = FMath::Lerp(SkyIntensity, NightSkyIntensity, T);
	}

	CurrentSkyIntensity = FMath::FInterpTo(CurrentSkyIntensity, TargetIntensity, GetWorld()->GetDeltaSeconds(), 2.f);
	SLComp->SetIntensity(CurrentSkyIntensity);
}

void UAlsasuaAtmosphereController::UpdateCloudVisuals(float Hour)
{
	CurrentCloudDensity = BaseCloudDensity;

	UWeatherSubsystem* Weather = GetWorld() ? GetWorld()->GetSubsystem<UWeatherSubsystem>() : nullptr;
	if (Weather)
	{
		switch (Weather->CurrentWeather)
		{
		case EWeatherSubsystemState::Clear:
			CurrentCloudDensity *= 0.4f;
			break;
		case EWeatherSubsystemState::Rainy:
			CurrentCloudDensity *= 1.5f;
			break;
		case EWeatherSubsystemState::Thunderstorm:
			CurrentCloudDensity *= 2.0f;
			break;
		case EWeatherSubsystemState::HeavyFog:
			CurrentCloudDensity *= 0.2f;
			break;
		}
	}

	if (Hour < 6.f || Hour > 20.f)
	{
		CurrentCloudDensity *= 0.6f;
	}
}

void UAlsasuaAtmosphereController::UpdateMoonVisuals(float Hour)
{
	if (!SunLight) return;

	if (Hour >= 21.f || Hour < 5.f)
	{
		if (USkyLightComponent* SLComp = SkyLight ? SkyLight->GetLightComponent() : nullptr)
		{
			const float MoonPhase = FMath::Abs(FMath::Sin(Hour * 0.26f));
			const float MoonBright = MoonBrightness * MoonPhase;
			SLComp->SetLightColor(FLinearColor::LerpUsingHSV(SLComp->GetLightColor(), MoonColor, 0.3f));
		}
	}
}
