#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "Services/ITimeOfDayService.h"
#include "AlsasuaAtmosphereController.generated.h"

class UExponentialHeightFogComponent;
class UDirectionalLightComponent;
class USkyLightComponent;
class USkyAtmosphereComponent;
class ADirectionalLight;
class ASkyAtmosphere;
class ASkyLight;
class AExponentialHeightFog;
class AVolumetricCloud;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimeOfDayVisualChanged, float, SunAngle);

UCLASS()
class GF_CLIMA_API UAlsasuaAtmosphereController : public UWorldSubsystem, public FTickableGameObject, public ITimeOfDayService
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsAllowedToTick() const override { return true; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(AlsasuaAtmosphereController, STATGROUP_Game); }
	virtual bool IsTickable() const override { return !IsTemplate(); }

	// ITimeOfDayService
	virtual float GetHour() const override;
	virtual float GetSunPitch() const override;
	virtual FRotator GetSunDirection() const override;
	virtual bool IsNight() const override;
	virtual FLinearColor GetSunColor() const override;
	virtual float GetSunIntensity() const override;

	UFUNCTION(BlueprintCallable, Category = "Alsasua|Atmosphere")
	void SetSunAngle(float AngleDeg);

	UFUNCTION(BlueprintCallable, Category = "Alsasua|Atmosphere")
	void SetCloudDensity(float Density);

	UFUNCTION(BlueprintCallable, Category = "Alsasua|Atmosphere")
	void SetFogDensity(float Density);

	UFUNCTION(BlueprintCallable, Category = "Alsasua|Atmosphere")
	void SetTimeOfDay(float Hour);

	/** Elevación solar real en grados (negativa de noche). */
	UFUNCTION(BlueprintPure, Category = "Alsasua|Atmosphere")
	float GetSunElevationDeg() const { return CurrentSunElevation; }

	/** Azimut solar en grados desde el norte, sentido horario. */
	UFUNCTION(BlueprintPure, Category = "Alsasua|Atmosphere")
	float GetSunAzimuthDeg() const { return CurrentSunAzimuth; }

	/** 0 = sol bajo el horizonte, 1 = sol en el cénit. */
	UFUNCTION(BlueprintPure, Category = "Alsasua|Atmosphere")
	float GetDaylightFactor() const { return CurrentDaylight; }

	UPROPERTY(BlueprintAssignable, Category = "Alsasua|Atmosphere")
	FOnTimeOfDayVisualChanged OnTimeOfDayVisualChanged;

	// ── Emplazamiento: Alsasua / Altsasu (Navarra) ─────────────────────────
	// La posición del sol se calcula de verdad a partir de estos datos, así que
	// la altura de mediodía, el azimut de salida/puesta y la duración del día
	// son los del pueblo real y no una curva inventada.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Geo")
	float Latitude = 42.8956f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Geo")
	float Longitude = -2.1697f;

	/** Huso horario en horas (CET = 1, CEST = 2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Geo")
	float TimeZoneHours = 2.f;

	/** Día del año (1-366). 172 = solsticio de verano. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Geo", meta = (ClampMin = "1", ClampMax = "366"))
	int32 DayOfYear = 172;

	/**
	 * Segundos entre actualizaciones. Mover el sol invalida la caché de draw
	 * commands del pueblo entero: hacerlo cada frame cuesta más que el resto
	 * del ciclo día/noche junto.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere", meta = (ClampMin = "0.0"))
	float UpdateInterval = 0.1f;

	// ── Sun ──────────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sun")
	FLinearColor DawnSunColor = FLinearColor(1.0f, 0.6f, 0.3f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sun")
	FLinearColor DaySunColor = FLinearColor(1.0f, 0.95f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sun")
	FLinearColor DuskSunColor = FLinearColor(1.0f, 0.4f, 0.15f);

	/** Deep red for the lowest sun (-2 to -4 deg elevation). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sun")
	FLinearColor DeepDuskSunColor = FLinearColor(0.9f, 0.2f, 0.05f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sun")
	float SunIntensity = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sun")
	float DawnIntensity = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sun")
	float NightIntensity = 0.1f;

	/** Distancia (cm) de sombras dinámicas nítidas. 300 m cubre el casco urbano. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sun")
	float ShadowDistance = 30000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sun", meta = (ClampMin = "1", ClampMax = "10"))
	int32 ShadowCascades = 6;

	/** Contact shadows: asientan bordillos y mobiliario que las cascadas no ven. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sun")
	float ContactShadowLength = 0.02f;

	/** Intensidad del sol dentro de la niebla volumétrica (rayos de luz). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sun")
	float SunVolumetricScattering = 1.f;

	/** Light shaft override direction for god rays through foliage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sun")
	FVector LightShaftDirection = FVector(-1.f, 0.f, -1.f);

	// ── Fog ──────────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Fog")
	float BaseFogDensity = 0.005f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Fog")
	float NightFogDensity = 0.01f;

	/** Dawn/dusk inversion-layer fog peak (physically: valley thermal inversion). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Fog")
	float DawnFogDensity = 0.018f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Fog")
	float RainFogDensity = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Fog")
	FLinearColor DayFogColor = FLinearColor(0.7f, 0.75f, 0.85f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Fog")
	FLinearColor NightFogColor = FLinearColor(0.05f, 0.08f, 0.15f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Fog")
	FLinearColor DawnFogColor = FLinearColor(0.9f, 0.6f, 0.4f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Fog")
	float FogHeightFalloff = 0.2f;

	/** Anisotropía de la niebla volumétrica: >0 dispersa hacia delante (halo solar). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Fog", meta = (ClampMin = "-0.9", ClampMax = "0.9"))
	float VolumetricFogScattering = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Fog")
	float VolumetricFogDistance = 25000.f;

	/** Extended golden-hour range in degrees (±). Default 15° covers Alsasua's latitude. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Fog")
	float GoldenHourRangeDeg = 15.f;

	// ── Clouds ───────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Clouds")
	float CloudSpeed = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Clouds")
	float BaseCloudDensity = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Clouds")
	float NightCloudDensity = 0.3f;

	// ── Volumetric Cloud Layer ───────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Clouds")
	float CloudLayerBottom = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Clouds")
	float CloudLayerHeight = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Clouds")
	float CloudTracingMaxDistance = 200.f;

	/** Quality: higher = more cloud samples, more GPU. 0.5-2.0 typical. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Clouds")
	float CloudSampleCountScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Clouds")
	float CloudShadowSampleCountScale = 0.5f;

	/** Cloud bottom occlusion from sky light — higher = darker undersides. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Clouds")
	float CloudBottomOcclusion = 0.3f;

	/** Transmittance threshold for cloud ray marching — lower = more opaque clouds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Clouds")
	float StopTracingTransmittanceThreshold = 0.01f;

	/** Shadow tracing distance through cloud layer — deeper = darker shadows on ground. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Clouds")
	float ShadowTracingDistance = 15.f;

	// ── Sky ──────────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sky")
	FLinearColor DaySkyColor = FLinearColor(0.1f, 0.3f, 0.8f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sky")
	FLinearColor NightSkyColor = FLinearColor(0.005f, 0.01f, 0.03f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sky")
	float SkyIntensity = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sky")
	float NightSkyIntensity = 0.05f;

	// ── Moon ─────────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Moon")
	float MoonBrightness = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Moon")
	FLinearColor MoonColor = FLinearColor(0.7f, 0.75f, 0.9f);

	/** Moon contribution multiplier to sky light (was 0.2, too dim). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Moon")
	float MoonSkyBounce = 0.5f;

	// ── Sky Atmosphere (Rayleigh / Mie) ─────────────────────────────────────

	/** Base Rayleigh scattering color. Shifts warm at golden hour. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|SkyAtmo")
	FLinearColor DayRayleighColor = FLinearColor(0.00582f, 0.01355f, 0.0331f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|SkyAtmo")
	float DayRayleighScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|SkyAtmo")
	float NightRayleighScale = 0.1f;

	/** Rayleigh color shifts toward orange/red at low sun. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|SkyAtmo")
	FLinearColor SunsetRayleighColor = FLinearColor(0.06f, 0.015f, 0.008f);

	/** Rayleigh scale drops at sunset (less blue scattering, more red path length). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|SkyAtmo")
	float SunsetRayleighScale = 0.6f;

	/** Rayleigh scale increases with rain (cleaner air scatters more blue). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|SkyAtmo")
	float RainRayleighScale = 1.3f;

	/** Rayleigh distribution altitude (km). Lower = denser low-altitude sky. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|SkyAtmo")
	float RayleighDistribution = 8.0f;

	/** Base Mie scattering (haze around sun). Increases at golden hour. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|SkyAtmo")
	FLinearColor DayMieColor = FLinearColor(0.005f, 0.005f, 0.005f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|SkyAtmo")
	float DayMieScale = 0.5f;

	/** Mie increases at golden hour (haze bands near horizon). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|SkyAtmo")
	float SunsetMieScale = 1.5f;

	/** Mie sky-high during rain/fog (uniform gray haze). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|SkyAtmo")
	float RainMieScale = 3.0f;

	/** Mie anisotropy: higher = tighter sun halo, lower = wider glow. 0.85-0.999 typical. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|SkyAtmo")
	float DayMieAnisotropy = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|SkyAtmo")
	float SunsetMieAnisotropy = 0.95f;

	/** Mie distribution altitude (km). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|SkyAtmo")
	float MieDistribution = 1.2f;

	/** Multi-scattering: 0 = single scatter only, 2 = recommended default for LUT quality. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|SkyAtmo")
	float DayMultiScattering = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|SkyAtmo")
	float NightMultiScattering = 0.5f;

	// ── Night Sky (Stars + Moon Corona) ───────────────────────────────────

	/** Star visibility: 0 = invisible, 1 = full. Fades with daylight + clouds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Stars")
	float StarIntensity = 1.0f;

	/** Star brightness scales with moon phase (full moon washes out dim stars). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Stars")
	float MoonWashoutFactor = 0.3f;

	/** Milky Way band visibility (extra diffuse glow on the sky dome). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Stars")
	float MilkyWayIntensity = 0.15f;

	/** Moon corona glow radius in degrees. Larger = more atmospheric scattering. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Moon")
	float MoonCoronaRadius = 15.f;

	/** Moon corona brightness (emissive halo). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Moon")
	float MoonCoronaIntensity = 0.5f;

private:
	void FindOrCreateAtmosphereActors();
	void ApplyLightSetup();
	void UpdateAtmosphere(float Hour, float DeltaTime);
	void UpdateSunVisuals(float Hour, float DeltaTime);
	void UpdateSkyAtmosphereVisuals(float DeltaTime);
	void UpdateFogVisuals(float DeltaTime);
	void UpdateSkyVisuals(float DeltaTime);
	void UpdateCloudVisuals();
	void UpdateCloudLayer(float DeltaTime);
	void UpdateRainShadows(float DeltaTime);
	void UpdateStarSky(float DeltaTime);

	/**
	 * Posición del sol (o del punto antisolar, donde va la luna llena) para la
	 * hora dada. NOAA simplificado: declinación de Cooper + ecuación del tiempo.
	 */
	void ComputeCelestialPosition(float Hour, bool bAntiSolar, float& OutElevationDeg, float& OutAzimuthDeg) const;

	/** Fracción iluminada de la luna (0 = nueva, 1 = llena). */
	float ComputeMoonPhase() const;

	/** Atenuación del sol y del cielo por nubosidad según el clima activo. */
	float GetCloudAttenuation() const;

	UPROPERTY()
	TObjectPtr<ADirectionalLight> SunLight;

	UPROPERTY()
	TObjectPtr<ASkyAtmosphere> SkyAtmosphere;

	UPROPERTY()
	TObjectPtr<ASkyLight> SkyLight;

	UPROPERTY()
	TObjectPtr<AExponentialHeightFog> HeightFog;

	UPROPERTY()
	AVolumetricCloud* VolumetricCloud = nullptr;

	float TimeToUpdate = 0.f;

	FLinearColor CurrentSunColor = FLinearColor::White;
	FLinearColor CurrentFogColor = FLinearColor(0.7f, 0.75f, 0.85f);
	float CurrentFogDensity = 0.005f;
	float CurrentCloudDensity = 0.5f;
	float CurrentSkyIntensity = 1.f;
	float CurrentSunIntensity = 10.f;
	float CurrentSunElevation = 45.f;
	float CurrentSunAzimuth = 180.f;
	float CurrentDaylight = 1.f;

	// SkyAtmosphere smooth state
	FLinearColor CurrentRayleighColor = FLinearColor(0.00582f, 0.01355f, 0.0331f);
	float CurrentRayleighScale = 1.0f;
	float CurrentMieScale = 0.5f;
	FLinearColor CurrentMieColor = FLinearColor(0.005f, 0.005f, 0.005f);
	float CurrentMieAnisotropy = 0.9f;
	float CurrentMultiScattering = 2.0f;
	float CurrentRainIntensity = 0.f;
};
