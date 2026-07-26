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

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY() UStaticMeshComponent* WaterMesh;
	UPROPERTY() UMaterialInstanceDynamic* WaterMatInst;

	void CrearMeshAgua();
	void CrearMaterialDinamico();
};
