#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaAtmosphereFogComponent.generated.h"

class UExponentialHeightFogComponent;
class UStaticMeshComponent;

/**
 * Componente de niebla volumétrica localizada. Crea parches de niebla
 * en valles, riberas del río, y zonas bajas de Alsasua.
 * Reacta al clima y hora del día.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GF_CLIMA_API UAlsasuaAtmosphereFogComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaAtmosphereFogComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- Fog Volume ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Volume")
	float FogDensityDay = 0.002f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Volume")
	float FogDensityNight = 0.008f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Volume")
	float FogDensityRain = 0.015f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Volume")
	float FogHeightFalloff = 0.2f;

	// --- Fog Color ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Color")
	FLinearColor FogColorDay = FLinearColor(0.8f, 0.85f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Color")
	FLinearColor FogColorNight = FLinearColor(0.05f, 0.06f, 0.1f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Color")
	FLinearColor FogColorDawn = FLinearColor(0.9f, 0.7f, 0.5f);

	// --- Valley Fog ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Valley")
	float ValleyFogMinAltitude = 50000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Valley")
	float ValleyFogMaxAltitude = 52000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Valley")
	float ValleyFogDensity = 0.01f;

	// --- River Fog ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|River")
	float RiverFogWidth = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|River")
	float RiverFogDensity = 0.008f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|River")
	float RiverFogSpeed = 0.5f;

private:
	void UpdateFog(float DeltaTime);

	float CurrentFogDensity = 0.002f;
};
