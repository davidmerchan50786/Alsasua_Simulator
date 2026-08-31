#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "AlsasuaVisualEffectsManager.generated.h"

class UMaterialParameterCollection;
class UMaterialParameterCollectionInstance;

/**
 * Manager central de efectos visuales. Actualiza el MPC global cada frame
 * con wetness, timeOfDay, wind, snow, ytodos los parámetros que conectan
 * TODOS los materiales del juego.
 */
UCLASS()
class GF_WORLD_API UAlsasuaVisualEffectsManager : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsAllowedToTick() const override { return true; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(AlsasuaVisualEffectsManager, STATGROUP_Game); }
	virtual bool IsTickable() const override { return !IsTemplate(); }   // no ticar el CDO (CLAUDE.md §9)

	// --- Wetness (lluvia) ---
	UPROPERTY(BlueprintReadOnly, Category = "VFX|Wetness")
	float GlobalWetness = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Wetness")
	float WetnessRainSpeed = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Wetness")
	float WetnessDrySpeed = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Wetness")
	float PuddleFormationThreshold = 0.4f;

	UPROPERTY(BlueprintReadOnly, Category = "VFX|Wetness")
	float PuddleAmount = 0.f;

	// --- Time of Day (para materiales) ---
	UPROPERTY(BlueprintReadOnly, Category = "VFX|Time")
	float NormalizedTime = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "VFX|Time")
	float DayNightBlend = 1.f;

	// --- Wind ---
	UPROPERTY(BlueprintReadOnly, Category = "VFX|Wind")
	float WindIntensity = 0.3f;

	UPROPERTY(BlueprintReadOnly, Category = "VFX|Wind")
	float WindDirection = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Wind")
	float WindGustFrequency = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Wind")
	float WindGustAmplitude = 0.4f;

	// --- Snow ---
	UPROPERTY(BlueprintReadOnly, Category = "VFX|Snow")
	float SnowAmount = 0.f;

	// --- Fog Tint ---
	UPROPERTY(BlueprintReadOnly, Category = "VFX|Fog")
	float FogDensityMult = 1.f;

	// --- Road wear ---
	UPROPERTY(BlueprintReadOnly, Category = "VFX|Road")
	float RoadWearAmount = 0.f;

	// --- Building emissive night ---
	UPROPERTY(BlueprintReadOnly, Category = "VFX|Buildings")
	float NightEmissiveIntensity = 0.f;

	// --- API ---
	UFUNCTION(BlueprintCallable, Category = "Alsasua|VFX")
	void SetRainIntensity(float Intensity);

	UFUNCTION(BlueprintCallable, Category = "Alsasua|VFX")
	void SetSnowIntensity(float Intensity);

	UFUNCTION(BlueprintCallable, Category = "Alsasua|VFX")
	void SetWind(float Intensity, float Direction);

	UFUNCTION(BlueprintCallable, Category = "Alsasua|VFX")
	void SetRoadWear(float Wear);

private:
	void UpdateMPC(float DeltaTime);
	void UpdateWetness(float DeltaTime);
	void UpdateTimeOfDay();
	void UpdateWind(float DeltaTime);
	void UpdateNightEmissive();

	/** El GameInstanceSubsystem de partículas (lluvia/hojas), o null si no está. */
	class UAlsasuaVFXManager* GetVFXManager() const;

	UPROPERTY()
	TObjectPtr<UMaterialParameterCollection> CachedMPC;

	UPROPERTY()
	TObjectPtr<UMaterialParameterCollection> MPCWind;

	float TimeAccumulator = 0.f;
};
