#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "Components/PostProcessComponent.h"
#include "AlsasuaZonePostProcess.generated.h"

USTRUCT(BlueprintType)
struct FZoneColorGrading
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ZoneID;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D Center = FVector2D::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Radius = 500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FLinearColor TemperatureTint = FLinearColor(1.f, 1.f, 1.f, 1.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float SaturationBoost = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float ContrastBoost = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Grain = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Bloom = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float VignetteIntensity = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FLinearColor VignetteColor = FLinearColor::Black;
};

UENUM(BlueprintType)
enum class EZoneType : uint8
{
	Exterior,
	Interior,
	InteriorOscuro,
	Sotano
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaZonePostProcess : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return !IsTemplate(); }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UAlsasuaZonePostProcess, STATGROUP_Game); }

	UFUNCTION(BlueprintCallable, Category = "Alsasua|PostProcess")
	void RegisterZone(const FZoneColorGrading& Zone);

	UFUNCTION(BlueprintCallable, Category = "Alsasua|PostProcess")
	void UnregisterZone(FName ZoneID);

	UFUNCTION(BlueprintCallable, Category = "Alsasua|PostProcess")
	void SetCurrentZoneType(EZoneType Type);

	UFUNCTION(BlueprintPure, Category = "Alsasua|PostProcess")
	EZoneType GetCurrentZoneType() const { return CurrentZoneType; }

	UFUNCTION(BlueprintPure, Category = "Alsasua|PostProcess")
	FName GetCurrentZoneID() const { return ActiveZoneID; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|PostProcess|Exterior")
	float ExteriorSaturation = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|PostProcess|Exterior")
	float ExteriorContrast = 1.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|PostProcess|Exterior")
	float ExteriorBloom = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|PostProcess|Exterior")
	float ExteriorTemperature = 6500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|PostProcess|Interior")
	float InteriorSaturation = 0.95f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|PostProcess|Interior")
	float InteriorContrast = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|PostProcess|Interior")
	float InteriorBloom = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|PostProcess|Interior")
	float InteriorTemperature = 5500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|PostProcess|Interior")
	float InteriorVignette = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|PostProcess|InteriorOscuro")
	float DarkSaturation = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|PostProcess|InteriorOscuro")
	float DarkContrast = 1.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|PostProcess|InteriorOscuro")
	float DarkGrain = 0.04f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|PostProcess|InteriorOscuro")
	float DarkVignette = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|PostProcess|InteriorOscuro")
	float DarkTemperature = 4000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|PostProcess|Sotano")
	float SotanoSaturation = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|PostProcess|Sotano")
	float SotanoGrain = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|PostProcess|Sotano")
	float SotanoVignette = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|PostProcess|Sotano")
	float SotanoTemperature = 3200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|PostProcess|Blend")
	float BlendSpeed = 4.0f;

private:
	void LoadNeighborhoodZones();
	void UpdatePostProcessVolume(float DeltaTime);
	void ApplyZoneBlending(float DeltaTime);
	float TemperatureToTint(float Kelvin) const;

	EZoneType CurrentZoneType = EZoneType::Exterior;
	EZoneType BlendedZoneType = EZoneType::Exterior;
	EZoneType PreviousZoneType = EZoneType::Exterior;

	FName ActiveZoneID;
	FName PreviousZoneID;

	UPROPERTY()
	TObjectPtr<UPostProcessComponent> ZonePPComponent;

	TArray<FZoneColorGrading> RegisteredZones;

	float BlendAlpha = 0.f;
	float TargetSaturation = 1.1f;
	float CurrentSaturation = 1.1f;
	float TargetContrast = 1.05f;
	float CurrentContrast = 1.05f;
	float TargetBloom = 0.15f;
	float CurrentBloom = 0.15f;
	float TargetVignette = 0.f;
	float CurrentVignette = 0.f;
	float TargetTemperature = 6500.f;
	float CurrentTemperature = 6500.f;
	float TargetGrain = 0.f;
	float CurrentGrain = 0.f;
};
