#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "AlsasuaVegetationSpawner.generated.h"

UCLASS()
class ALSASUAWORLD_API UAlsasuaVegetationSpawner : public UWorldSubsystem, public IAlsasuaRespaldoVegetacion
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	UFUNCTION(BlueprintCallable, Category="Alsasua|Vegetacion")
	int32 SembrarVegetacion() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Alsasua|Vegetacion")
	float DensidadHierba = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Alsasua|Vegetacion")
	float DensidadArbusto = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Alsasua|Vegetacion")
	int32 MaxHierba = 12000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Alsasua|Vegetacion")
	int32 MaxArbustos = 1500;
};
