#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "AlsasuaTrafficAgent.generated.h"

UCLASS()
class GF_TRAFICO_API AAlsasuaTrafficAgent : public AActor
{
	GENERATED_BODY()

public:	
	AAlsasuaTrafficAgent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "Traffic")
	USplineComponent* RouteSpline;

	UPROPERTY(EditAnywhere, Category = "Traffic")
	float Speed = 800.f;

	UPROPERTY(EditAnywhere, Category = "Traffic")
	bool bIsPatrolVehicle = false;

private:
	float DistanceAlongSpline;

	// Función para detectar si hay gente u otros coches delante
	bool ScanForObstacles();
};
