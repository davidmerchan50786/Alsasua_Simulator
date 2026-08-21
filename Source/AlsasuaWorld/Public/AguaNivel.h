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

	// CrearMeshAgua() estaba declarada aquí y no la definía nadie: la malla la
	// crea y configura el constructor, incluido el SetStaticMesh del plano. Una
	// declaración sin definición no es un aviso, es una trampa —el primero
	// que la llame se lleva un error de ENLAZADO, que no lo canta el editor.
	void CrearMaterialDinamico();
};
