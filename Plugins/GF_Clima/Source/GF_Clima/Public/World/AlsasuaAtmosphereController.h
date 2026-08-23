#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "ContratosClima.h"
#include "AlsasuaAtmosphereController.generated.h"

class UExponentialHeightFogComponent;
class UDirectionalLightComponent;
class USkyLightComponent;
class USkyAtmosphereComponent;
class ADirectionalLight;
class ASkyAtmosphere;
class ASkyLight;
class AExponentialHeightFog;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimeOfDayVisualChanged, float, SunAngle);

UCLASS()
class GF_CLIMA_API UAlsasuaAtmosphereController : public UWorldSubsystem, public FTickableGameObject, public ITimeOfDayService
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsAllowedToTick() const override { return true; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(AlsasuaAtmosphereController, STATGROUP_Game); }
	virtual bool IsTickable() const override { return !IsTemplate(); }   // no ticar el CDO (CLAUDE.md §9)

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

	// ── Contrato ITimeOfDayService (publicado como "Clima.TiempoDelDia") ──
	virtual float GetSunPitch() const override { return GetSunElevationDeg(); }
	virtual float GetHour() const override { return CurrentHour; }
	virtual bool IsNight() const override { return CurrentSunElevation <= 0.f; }
	virtual FVector GetSunDirection() const override;
	virtual FLinearColor GetSunColor() const override { return CurrentSunColor; }
	virtual float GetSunIntensity() const override { return CurrentSunIntensity; }

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sun")
	FLinearColor DawnSunColor = FLinearColor(1.0f, 0.6f, 0.3f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sun")
	FLinearColor DaySunColor = FLinearColor(1.0f, 0.95f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sun")
	FLinearColor DuskSunColor = FLinearColor(1.0f, 0.4f, 0.15f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sun")
	float SunIntensity = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sun")
	float DawnIntensity = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sun")
	float NightIntensity = 0.1f;

	/** Distancia (cm) de sombras dinámicas nítidas. 300 m cubre el casco urbano. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sun")
	float ShadowDistance = 30000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sun", meta = (ClampMin = "1", ClampMax = "6"))
	int32 ShadowCascades = 4;

	/** Contact shadows: asientan bordillos y mobiliario que las cascadas no ven. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sun")
	float ContactShadowLength = 0.02f;

	/** Intensidad del sol dentro de la niebla volumétrica (rayos de luz). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sun")
	float SunVolumetricScattering = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Fog")
	float BaseFogDensity = 0.005f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Fog")
	float NightFogDensity = 0.01f;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Clouds")
	float CloudSpeed = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Clouds")
	float BaseCloudDensity = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Clouds")
	float NightCloudDensity = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sky")
	FLinearColor DaySkyColor = FLinearColor(0.1f, 0.3f, 0.8f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sky")
	FLinearColor NightSkyColor = FLinearColor(0.005f, 0.01f, 0.03f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sky")
	float SkyIntensity = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Sky")
	float NightSkyIntensity = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Moon")
	float MoonBrightness = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Atmosphere|Moon")
	FLinearColor MoonColor = FLinearColor(0.7f, 0.75f, 0.9f);

private:
	void FindOrCreateAtmosphereActors();
	void ApplyLightSetup();
	void UpdateAtmosphere(float Hour, float DeltaTime);
	void UpdateSunVisuals(float Hour, float DeltaTime);
	void UpdateFogVisuals(float DeltaTime);
	void UpdateSkyVisuals(float DeltaTime);
	void UpdateCloudVisuals();

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

	float TimeToUpdate = 0.f;

	/** Hora simulada actual (0-24); la escribe UpdateAtmosphere cada tick. */
	float CurrentHour = 12.f;

	FLinearColor CurrentSunColor = FLinearColor::White;
	FLinearColor CurrentFogColor = FLinearColor(0.7f, 0.75f, 0.85f);
	float CurrentFogDensity = 0.005f;
	float CurrentCloudDensity = 0.5f;
	float CurrentSkyIntensity = 1.f;
	float CurrentSunIntensity = 10.f;
	float CurrentSunElevation = 45.f;
	float CurrentSunAzimuth = 180.f;
	float CurrentDaylight = 1.f;
};
