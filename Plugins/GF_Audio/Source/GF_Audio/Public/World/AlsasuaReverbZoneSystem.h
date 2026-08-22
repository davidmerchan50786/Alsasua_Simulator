#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaReverbZoneSystem.generated.h"

/**
 * Sistema de zonas de reverberación por ubicación.
 * Añade reverb adaptativo: interior, exterior, callejón, iglesia, cueva.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GF_AUDIO_API UAlsasuaReverbZoneSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaReverbZoneSystem();

	virtual void BeginPlay() override;

	// --- Zone Types ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb|Exterior")
	float ExteriorReverbTime = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb|Exterior")
	float ExteriorVolume = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb|Interior")
	float InteriorReverbTime = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb|Interior")
	float InteriorVolume = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb|Alley")
	float AlleyReverbTime = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb|Alley")
	float AlleyVolume = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb|Church")
	float ChurchReverbTime = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb|Church")
	float ChurchVolume = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb|Cave")
	float CaveReverbTime = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb|Cave")
	float CaveVolume = 0.4f;

	// --- Zone Size ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb|Size")
	float DefaultZoneRadius = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb|Size")
	float ChurchZoneRadius = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb|Size")
	float CaveZoneRadius = 600.f;

private:
	void SpawnReverbZones();

	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedZones;
};
