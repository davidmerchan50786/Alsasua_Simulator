#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
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
class ALSASUAMANIFA_API UAlsasuaAtmosphereController : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsAllowedToTick() const override { return true; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(AlsasuaAtmosphereController, STATGROUP_Game); }

	UFUNCTION(BlueprintCallable, Category = "Alsasua|Atmosphere")
	void SetSunAngle(float AngleDeg);

	UFUNCTION(BlueprintCallable, Category = "Alsasua|Atmosphere")
	void SetCloudDensity(float Density);

	UFUNCTION(BlueprintCallable, Category = "Alsasua|Atmosphere")
	void SetFogDensity(float Density);

	UFUNCTION(BlueprintCallable, Category = "Alsasua|Atmosphere")
	void SetTimeOfDay(float Hour);

	UPROPERTY(BlueprintAssignable, Category = "Alsasua|Atmosphere")
	FOnTimeOfDayVisualChanged OnTimeOfDayVisualChanged;

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
	void UpdateSunVisuals(float Hour);
	void UpdateFogVisuals(float Hour);
	void UpdateSkyVisuals(float Hour);
	void UpdateCloudVisuals(float Hour);
	void UpdateMoonVisuals(float Hour);
	float GetSunElevation(float Hour) const;

	UPROPERTY()
	TObjectPtr<ADirectionalLight> SunLight;

	UPROPERTY()
	TObjectPtr<ASkyAtmosphere> SkyAtmosphere;

	UPROPERTY()
	TObjectPtr<ASkyLight> SkyLight;

	UPROPERTY()
	TObjectPtr<AExponentialHeightFog> HeightFog;

	FLinearColor CurrentSunColor = FLinearColor::White;
	FLinearColor CurrentFogColor = FLinearColor(0.7f, 0.75f, 0.85f);
	float CurrentFogDensity = 0.005f;
	float CurrentCloudDensity = 0.5f;
	float CurrentSkyIntensity = 1.f;
	float CurrentSunIntensity = 10.f;
};
