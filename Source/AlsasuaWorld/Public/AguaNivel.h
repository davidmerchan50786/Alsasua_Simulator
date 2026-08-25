#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AguaNivel.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS()
class ALSASUAWORLD_API AAguaNivel : public AActor
{
	GENERATED_BODY()

public:
	AAguaNivel();

	UPROPERTY(EditAnywhere, Category="Agua|Geometry")
	float AnchoCm = 800000.f;

	UPROPERTY(EditAnywhere, Category="Agua|Geometry")
	float LargoCm = 800000.f;

	UPROPERTY(EditAnywhere, Category="Agua|Geometry")
	float AlturaNivelCm = -1393.9f;

	UPROPERTY(EditAnywhere, Category="Agua|Visual")
	FLinearColor WaterColor = FLinearColor(0.01f, 0.05f, 0.15f, 0.85f);

	UPROPERTY(EditAnywhere, Category="Agua|Visual")
	float OpacityBase = 0.85f;

	UPROPERTY(EditAnywhere, Category="Agua|Visual")
	float FresnelPower = 4.0f;

	UPROPERTY(EditAnywhere, Category="Agua|Visual")
	float WaveSpeed = 0.5f;

	// Gerstner wave parameters
	UPROPERTY(EditAnywhere, Category="Agua|Waves")
	float WaveAmplitude = 15.0f;

	UPROPERTY(EditAnywhere, Category="Agua|Waves")
	float WaveFrequency = 0.02f;

	UPROPERTY(EditAnywhere, Category="Agua|Waves")
	float WaveSteepness = 0.5f;

	UPROPERTY(EditAnywhere, Category="Agua|Waves")
	int32 WaveDirections = 3;

	// Shoreline parameters
	UPROPERTY(EditAnywhere, Category="Agua|Shoreline")
	float ShorelineDepth = 200.0f;

	UPROPERTY(EditAnywhere, Category="Agua|Shoreline")
	FLinearColor ShorelineFoamColor = FLinearColor(0.8f, 0.85f, 0.9f, 0.6f);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY() UStaticMeshComponent* WaterMesh;
	UPROPERTY() UMaterialInstanceDynamic* WaterMatInst;

	void CrearMaterialDinamico();
	void ActualizarOndas(float Time);
};
