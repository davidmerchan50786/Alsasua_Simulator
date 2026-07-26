#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AlsasuaVegetationSpawner.generated.h"

UCLASS()
class ALSASUAWORLD_API UAlsasuaVegetationSpawner : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	UFUNCTION(BlueprintCallable, Category="Alsasua|Vegetacion")
	int32 SembrarVegetacion();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Alsasua|Vegetacion")
	float DensidadHierba = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Alsasua|Vegetacion")
	float DensidadArbusto = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Alsasua|Vegetacion")
	int32 MaxHierba = 5000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Alsasua|Vegetacion")
	int32 MaxArbustos = 500;
};
