#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaFoliageWindComponent.generated.h"

/**
 * Componente de viento para foliage. Se coloca en actores de vegetation
 * y actualiza parámetros de material para animar hierba/árboles/arbustos.
 * Usa世界空间 sinéus de viento + ráfagas.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UAlsasuaFoliageWindComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaFoliageWindComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- Tree wind ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind|Tree")
	float TreeSwayAmplitude = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind|Tree")
	float TreeSwayFrequency = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind|Tree")
	float TreeLean = 1.5f;

	// --- Grass wind ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind|Grass")
	float GrassSwayAmplitude = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind|Grass")
	float GrassSwayFrequency = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind|Grass")
	float GrassBendAmount = 12.0f;

	// --- Bush wind ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind|Bush")
	float BushSwayAmplitude = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind|Bush")
	float BushSwayFrequency = 1.2f;

	// --- Gusts ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind|Gust")
	float GustFrequency = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind|Gust")
	float GustStrength = 0.5f;

private:
	void UpdateWindParameters(float DeltaTime);

	float TimeAccum = 0.f;
	float CurrentGust = 0.f;
};
