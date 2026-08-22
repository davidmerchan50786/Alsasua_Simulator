#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaLODConfigComponent.generated.h"

UENUM(BlueprintType)
enum class ELODLevel : uint8
{
	High,
	Medium,
	Low,
	UltraLow
};

USTRUCT(BlueprintType)
struct FLODThresholds
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) float HighDistance = 2000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MediumDistance = 5000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float LowDistance = 10000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Culldistance = 15000.f;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GF_WORLD_API UAlsasuaLODConfigComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaLODConfigComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Alsasua|LOD")
	void SetGlobalLODThresholds(const FLODThresholds& Thresholds);

	UFUNCTION(BlueprintPure, Category = "Alsasua|LOD")
	ELODLevel GetCurrentLODLevel() const;

	UFUNCTION(BlueprintPure, Category = "Alsasua|LOD")
	float GetLODBias() const { return LODBias; }

	UFUNCTION(BlueprintCallable, Category = "Alsasua|LOD")
	void SetScreenSizeOverride(float ScreenSize);

	UFUNCTION(BlueprintCallable, Category = "Alsasua|LOD")
	static void ApplyGlobalNaniteSettings(bool bEnableNanite, int32 MaxPixelsPerEdge = 1);

	UFUNCTION(BlueprintCallable, Category = "Alsasua|LOD")
	static void ApplyGlobalHLODSettings(bool bEnableHLOD, float HLODScreenSize = 0.1f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|LOD")
	FLODThresholds Thresholds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|LOD")
	float LODBias = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|LOD")
	float DistanceUpdateInterval = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|LOD")
	bool bAutoUpdateLOD = true;

	UPROPERTY(BlueprintReadOnly, Category = "Alsasua|LOD")
	ELODLevel CurrentLOD = ELODLevel::High;

private:
	void UpdateLODFromDistance();

	float TimeSinceLastUpdate = 0.f;
	float LastDistanceToCamera = 0.f;
};
