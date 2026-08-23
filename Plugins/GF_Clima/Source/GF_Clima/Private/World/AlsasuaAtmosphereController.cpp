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
#include "AlsasuaServiceRegistry.h"
#include "Engine/World.h"

namespace
{
	/** Suavizado exponencial independiente del framerate. */
	FORCEINLINE float SmoothAlpha(float Speed, float DeltaTime)
	{
		return 1.f - FMath::Exp(-FMath::Max(Speed, 0.f) * FMath::Max(DeltaTime, 0.f));
	}

	/** Fin del crepúsculo civil: por debajo el sol ya no aporta luz direccional. */
	constexpr float CivilTwilightDeg = -6.f;
}

void UAlsasuaAtmosphereController::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<UTimeOfDayManager>();

	FindOrCreateAtmosphereActors();
	ApplyLightSetup();

	// Publicacion en el tablon: los consumidores preguntan por el contrato,
	// no por esta clase. Si el plugin duerme, PedirComo<> devuelve nullptr.
	if (UAlsasuaServiceRegistry* Registro = UAlsasuaServiceRegistry::Get(this))
	{
		Registro->Publicar(TEXT("Clima.TiempoDelDia"), this);
	}
}

void UAlsasuaAtmosphereController::Deinitialize()
{
	if (UAlsasuaServiceRegistry* Registro = UAlsasuaServiceRegistry::Get(this))
	{
		Registro->Retirar(TEXT("Clima.TiempoDelDia"));
	}
	Super::Deinitialize();
}

FVector UAlsasuaAtmosphereController::GetSunDirection() const
{
	// Misma orientacion que la luz direccional (UpdateSunVisuals); el forward
	// del actor apunta DESDE el sol, asi que hacia el sol es el inverso.
	const FRotator OrientacionLuz(-CurrentSunElevation, CurrentSunAzimuth - 180.f, 0.f);
	return -OrientacionLuz.Vector();
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
		if (SunLight) SunLight->Rename(TEXT("Atmosphere_Sun"));
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
		if (SkyLight) SkyLight->Rename(TEXT("Atmosphere_SkyLight"));
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
		if (HeightFog) HeightFog->Rename(TEXT("Atmosphere_Fog"));
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  ApplyLightSetup: configuración que no depende de la hora. Se aplica también
//  a los actores que ya venían en el mapa, no sólo a los que creamos nosotros
//  (antes un mapa con su propia niebla se quedaba sin niebla volumétrica).
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaAtmosphereController::ApplyLightSetup()
{
	if (SunLight)
	{
		if (USceneComponent* Root = SunLight->GetRootComponent()) Root->SetMobility(EComponentMobility::Movable);

		if (UDirectionalLightComponent* DirComp = Cast<UDirectionalLightComponent>(SunLight->GetLightComponent()))
		{
			DirComp->SetIntensity(SunIntensity);
			DirComp->SetLightColor(DaySunColor);
			DirComp->SetCastShadows(true);

			// Sin esto la SkyAtmosphere no sigue al sol: el cielo se queda con un
			// color fijo mientras el sol se mueve, y no hay amanecer ni atardecer.
			DirComp->bAtmosphereSunLight = true;

			DirComp->SetDynamicShadowCascades(FMath::Clamp(ShadowCascades, 1, 6));
			DirComp->SetDynamicShadowDistanceMovableLight(ShadowDistance);
			DirComp->SetCascadeDistributionExponent(3.f);
			// UE 5.8 retiró el setter: la longitud de sombra de contacto es
			// propiedad pública y el MarkRenderStateDirty de abajo la aplica.
			DirComp->ContactShadowLength = ContactShadowLength;
			DirComp->SetVolumetricScatteringIntensity(SunVolumetricScattering);
			DirComp->MarkRenderStateDirty();
		}
	}

	if (SkyLight)
	{
		if (USceneComponent* Root = SkyLight->GetRootComponent()) Root->SetMobility(EComponentMobility::Movable);

		if (USkyLightComponent* SLComp = SkyLight->GetLightComponent())
		{
			// La captura en tiempo real toma el color real del cielo de la
			// SkyAtmosphere, así que el tinte debe quedarse neutro: cualquier
			// color aquí se multiplica encima y falsea la luz ambiental.
			SLComp->bRealTimeCapture = true;
			SLComp->SetLightColor(FLinearColor::White);
			SLComp->SetIntensity(SkyIntensity);
			SLComp->MarkRenderStateDirty();
		}
	}

	if (HeightFog)
	{
		if (UExponentialHeightFogComponent* FogComp = HeightFog->GetComponent())
		{
			FogComp->SetFogDensity(BaseFogDensity);
			FogComp->SetFogHeightFalloff(FogHeightFalloff);
			FogComp->SetFogInscatteringColor(DayFogColor);
			FogComp->SetVolumetricFogScatteringDistribution(VolumetricFogScattering);
			FogComp->SetVolumetricFogAlbedo(FColor::White);
			FogComp->SetVolumetricFogDistance(VolumetricFogDistance);
			FogComp->bEnableVolumetricFog = true;
			FogComp->MarkRenderStateDirty();
		}
	}

	CurrentSunColor = DaySunColor;
	CurrentFogColor = DayFogColor;
}

void UAlsasuaAtmosphereController::Tick(float DeltaTime)
{
	const UWorld* W = GetWorld();
	if (!W || W->WorldType == EWorldType::Editor) return;

	UTimeOfDayManager* TimeMgr = W->GetSubsystem<UTimeOfDayManager>();
	if (!TimeMgr) return;

	TimeToUpdate -= DeltaTime;
	if (TimeToUpdate > 0.f) return;

	const float Elapsed = FMath::Max(UpdateInterval, DeltaTime);
	TimeToUpdate = UpdateInterval;

	UpdateAtmosphere(TimeMgr->CurrentTime, Elapsed);
}

void UAlsasuaAtmosphereController::SetTimeOfDay(float Hour)
{
	UTimeOfDayManager* TimeMgr = GetWorld() ? GetWorld()->GetSubsystem<UTimeOfDayManager>() : nullptr;
	if (TimeMgr)
	{
		TimeMgr->SetTime(Hour);
	}

	// Salto explícito de hora (misión, cinemática): sin interpolación, el
	// resultado debe verse ya en el frame siguiente.
	UpdateAtmosphere(Hour, 1000.f);
}

void UAlsasuaAtmosphereController::SetSunAngle(float AngleDeg)
{
	if (SunLight)
	{
		SunLight->SetActorRotation(FRotator(-AngleDeg, CurrentSunAzimuth - 180.f, 0.f));
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

// ─────────────────────────────────────────────────────────────────────────────
//  ComputeCelestialPosition
//  Declinación (Cooper) + ecuación del tiempo (Spencer) + ángulo horario.
//  Para Alsasua en el solsticio de verano da mediodía solar a 70.6° de altura,
//  salida ~06:35 y puesta ~21:45 (CEST), que son los valores reales.
//  bAntiSolar devuelve el punto opuesto al sol: donde está la luna llena.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaAtmosphereController::ComputeCelestialPosition(float Hour, bool bAntiSolar, float& OutElevationDeg, float& OutAzimuthDeg) const
{
	const float N = (float)FMath::Clamp(DayOfYear, 1, 366);

	float DeclRad = FMath::DegreesToRadians(23.44f) * FMath::Sin(2.f * PI * (284.f + N) / 365.f);

	const float B = 2.f * PI * (N - 81.f) / 364.f;
	const float EqTimeMin = 9.87f * FMath::Sin(2.f * B) - 7.53f * FMath::Cos(B) - 1.5f * FMath::Sin(B);

	// Corrección del reloj civil: desvío respecto al meridiano del huso + ecuación del tiempo.
	const float TimeCorrMin = 4.f * (Longitude - 15.f * TimeZoneHours) + EqTimeMin;
	const float SolarHour = Hour + TimeCorrMin / 60.f;

	float HourAngleRad = FMath::DegreesToRadians(15.f * (SolarHour - 12.f));

	if (bAntiSolar)
	{
		DeclRad = -DeclRad;
		HourAngleRad += PI;
	}

	const float LatRad = FMath::DegreesToRadians(Latitude);
	const float SinDecl = FMath::Sin(DeclRad);
	const float CosDecl = FMath::Cos(DeclRad);
	const float SinLat = FMath::Sin(LatRad);
	const float CosLat = FMath::Cos(LatRad);
	const float CosHour = FMath::Cos(HourAngleRad);

	const float ElevRad = FMath::Asin(FMath::Clamp(SinDecl * SinLat + CosDecl * CosLat * CosHour, -1.f, 1.f));

	const float CosAz = (SinDecl * CosLat - CosDecl * SinLat * CosHour) /
		FMath::Max(FMath::Cos(ElevRad), KINDA_SMALL_NUMBER);
	float AzDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(CosAz, -1.f, 1.f)));

	// Ángulo horario positivo = después del mediodía solar = astro al oeste.
	if (FMath::Sin(HourAngleRad) > 0.f) AzDeg = 360.f - AzDeg;

	OutElevationDeg = FMath::RadiansToDegrees(ElevRad);
	OutAzimuthDeg = AzDeg;
}

float UAlsasuaAtmosphereController::ComputeMoonPhase() const
{
	constexpr float SynodicMonth = 29.53f;
	const float Cycle = FMath::Fmod((float)DayOfYear, SynodicMonth) / SynodicMonth;
	return 0.5f * (1.f - FMath::Cos(2.f * PI * Cycle));
}

float UAlsasuaAtmosphereController::GetCloudAttenuation() const
{
	const UWeatherSubsystem* Weather = GetWorld() ? GetWorld()->GetSubsystem<UWeatherSubsystem>() : nullptr;
	if (!Weather) return 1.f;

	switch (Weather->CurrentWeather)
	{
	case EAlsasuaWeatherState::Rainy:        return 0.35f;
	case EAlsasuaWeatherState::Thunderstorm: return 0.20f;
	case EAlsasuaWeatherState::HeavyFog:     return 0.45f;
	case EAlsasuaWeatherState::Clear:
	default:                                   return 1.f;
	}
}

void UAlsasuaAtmosphereController::UpdateAtmosphere(float Hour, float DeltaTime)
{
	CurrentHour = Hour;
	ComputeCelestialPosition(Hour, /*bAntiSolar*/ false, CurrentSunElevation, CurrentSunAzimuth);

	// Iluminancia horizontal ∝ seno de la elevación: es la razón física de que
	// el mediodía sea ~10 veces más luminoso que la hora dorada.
	CurrentDaylight = FMath::Clamp(FMath::Sin(FMath::DegreesToRadians(CurrentSunElevation)), 0.f, 1.f);

	UpdateSunVisuals(Hour, DeltaTime);
	UpdateSkyVisuals(DeltaTime);
	UpdateFogVisuals(DeltaTime);
	UpdateCloudVisuals();

	OnTimeOfDayVisualChanged.Broadcast(CurrentSunElevation);
}

// ─────────────────────────────────────────────────────────────────────────────
//  UpdateSunVisuals
//  Todo sale de la elevación, no de tramos horarios: no hay saltos de color al
//  cruzar una hora concreta ni luz direccional entrando desde debajo del suelo.
//  Por debajo del crepúsculo civil la luz pasa a ser la luna, con su posición y
//  su fase reales, en vez de un tinte azul apuntando a un ángulo fijo.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaAtmosphereController::UpdateSunVisuals(float Hour, float DeltaTime)
{
	if (!SunLight) return;

	UDirectionalLightComponent* DirComp = Cast<UDirectionalLightComponent>(SunLight->GetLightComponent());
	if (!DirComp) return;

	const float CloudAtten = GetCloudAttenuation();

	FLinearColor TargetSunColor = DaySunColor;
	float TargetSunIntensity = NightIntensity;
	FRotator TargetLightRotation = SunLight->GetActorRotation();

	if (CurrentSunElevation > CivilTwilightDeg)
	{
		// Enrojecimiento por masa de aire: cuanto más rasante, más cálido.
		const float HighT = FMath::Clamp(CurrentSunElevation / 12.f, 0.f, 1.f);
		const FLinearColor HorizonColor = (CurrentSunAzimuth < 180.f) ? DawnSunColor : DuskSunColor;
		TargetSunColor = FLinearColor::LerpUsingHSV(HorizonColor, DaySunColor, HighT);

		// DawnIntensity actúa de suelo en el crepúsculo, donde el seno ya es 0.
		const float Twilight = FMath::Clamp((CurrentSunElevation - CivilTwilightDeg) / -CivilTwilightDeg, 0.f, 1.f);
		TargetSunIntensity = FMath::Max(SunIntensity * CurrentDaylight, DawnIntensity * Twilight) * CloudAtten;

		TargetLightRotation = FRotator(-CurrentSunElevation, CurrentSunAzimuth - 180.f, 0.f);
	}
	else
	{
		float MoonElev, MoonAz;
		ComputeCelestialPosition(Hour, /*bAntiSolar*/ true, MoonElev, MoonAz);

		const float MoonUp = FMath::Clamp(FMath::Sin(FMath::DegreesToRadians(MoonElev)), 0.f, 1.f);
		const float Phase = ComputeMoonPhase();

		TargetSunColor = MoonColor;
		TargetSunIntensity = FMath::Max(MoonBrightness * MoonUp * Phase * CloudAtten, NightIntensity);

		// Con la luna bajo el horizonte se mantiene una inclinación baja para que
		// la luz residual de estrellas siga entrando de lado y no desde arriba.
		TargetLightRotation = FRotator(-FMath::Max(MoonElev, 8.f), MoonAz - 180.f, 0.f);
	}

	const float Alpha = SmoothAlpha(2.f, DeltaTime);
	CurrentSunColor = FLinearColor::LerpUsingHSV(CurrentSunColor, TargetSunColor, Alpha);
	CurrentSunIntensity = FMath::Lerp(CurrentSunIntensity, TargetSunIntensity, Alpha);

	SunLight->SetActorRotation(FMath::RInterpTo(SunLight->GetActorRotation(), TargetLightRotation, DeltaTime, 4.f));
	DirComp->SetLightColor(CurrentSunColor);
	DirComp->SetIntensity(CurrentSunIntensity);
}

// ─────────────────────────────────────────────────────────────────────────────
//  UpdateSkyVisuals: sólo intensidad. El color lo pone la captura en tiempo
//  real desde la SkyAtmosphere, que ya sabe dónde está el sol.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaAtmosphereController::UpdateSkyVisuals(float DeltaTime)
{
	if (!SkyLight) return;

	USkyLightComponent* SLComp = SkyLight->GetLightComponent();
	if (!SLComp) return;

	const float Twilight = FMath::Clamp((CurrentSunElevation - CivilTwilightDeg) / -CivilTwilightDeg, 0.f, 1.f);

	float TargetSkyIntensity = FMath::Lerp(NightSkyIntensity, SkyIntensity, FMath::Max(CurrentDaylight, Twilight));

	// Rebote lunar: de noche el ambiente no es negro puro.
	TargetSkyIntensity += MoonBrightness * ComputeMoonPhase() * 0.2f * (1.f - Twilight);
	TargetSkyIntensity *= FMath::Lerp(1.f, 0.7f, 1.f - GetCloudAttenuation());

	CurrentSkyIntensity = FMath::Lerp(CurrentSkyIntensity, TargetSkyIntensity, SmoothAlpha(2.f, DeltaTime));
	SLComp->SetIntensity(CurrentSkyIntensity);
}

// ─────────────────────────────────────────────────────────────────────────────
//  UpdateFogVisuals: densidad y color de la niebla también por elevación, y el
//  halo direccional toma el color actual del sol (niebla atravesada por luz).
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaAtmosphereController::UpdateFogVisuals(float DeltaTime)
{
	if (!HeightFog) return;

	UExponentialHeightFogComponent* FogComp = HeightFog->GetComponent();
	if (!FogComp) return;

	const UWeatherSubsystem* Weather = GetWorld() ? GetWorld()->GetSubsystem<UWeatherSubsystem>() : nullptr;
	const bool bRaining = Weather && (Weather->CurrentWeather == EAlsasuaWeatherState::Rainy || Weather->CurrentWeather == EAlsasuaWeatherState::Thunderstorm);
	const bool bFoggy = Weather && Weather->CurrentWeather == EAlsasuaWeatherState::HeavyFog;

	// De noche la inversión térmica del valle deja más niebla; al alba es cálida.
	const float DayT = FMath::Clamp(CurrentSunElevation / 10.f, 0.f, 1.f);
	const float HorizonT = 1.f - FMath::Clamp(FMath::Abs(CurrentSunElevation) / 10.f, 0.f, 1.f);

	float TargetFogDensity = FMath::Lerp(NightFogDensity, BaseFogDensity, DayT);
	FLinearColor TargetFogColor = FLinearColor::LerpUsingHSV(NightFogColor, DayFogColor, DayT);

	// Cerca del horizonte la niebla se tiñe del color del sol rasante.
	TargetFogColor = FLinearColor::LerpUsingHSV(TargetFogColor, DawnFogColor, HorizonT * DayT);

	if (bRaining) TargetFogDensity = FMath::Max(TargetFogDensity * 2.5f, RainFogDensity);
	if (bFoggy) TargetFogDensity *= 5.f;

	const float Alpha = SmoothAlpha(2.f, DeltaTime);
	CurrentFogDensity = FMath::Lerp(CurrentFogDensity, TargetFogDensity, Alpha);
	CurrentFogColor = FLinearColor::LerpUsingHSV(CurrentFogColor, TargetFogColor, Alpha);

	FogComp->SetFogDensity(CurrentFogDensity);
	FogComp->SetFogInscatteringColor(CurrentFogColor);
	FogComp->SetDirectionalInscatteringColor(CurrentSunColor);
	FogComp->SetDirectionalInscatteringExponent(FMath::Lerp(2.f, 16.f, CurrentDaylight));
}

void UAlsasuaAtmosphereController::UpdateCloudVisuals()
{
	CurrentCloudDensity = BaseCloudDensity;

	const UWeatherSubsystem* Weather = GetWorld() ? GetWorld()->GetSubsystem<UWeatherSubsystem>() : nullptr;
	if (Weather)
	{
		switch (Weather->CurrentWeather)
		{
		case EAlsasuaWeatherState::Clear:        CurrentCloudDensity *= 0.4f; break;
		case EAlsasuaWeatherState::Rainy:        CurrentCloudDensity *= 1.5f; break;
		case EAlsasuaWeatherState::Thunderstorm: CurrentCloudDensity *= 2.0f; break;
		case EAlsasuaWeatherState::HeavyFog:     CurrentCloudDensity *= 0.2f; break;
		}
	}

	// De noche la convección cesa y la cobertura baja hacia NightCloudDensity.
	if (CurrentDaylight <= 0.f)
	{
		CurrentCloudDensity = FMath::Min(CurrentCloudDensity, NightCloudDensity);
	}
}
