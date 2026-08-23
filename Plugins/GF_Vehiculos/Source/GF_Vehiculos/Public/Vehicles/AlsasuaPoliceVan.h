#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AlsasuaPoliceVan.generated.h"

UCLASS()
class GF_VEHICULOS_API AAlsasuaPoliceVan : public APawn
{
    GENERATED_BODY()

public:
    AAlsasuaPoliceVan();

    // Interceptor.fbx (Police Car & Helicopter) es una malla estática sin
    // esqueleto, no un personaje: UStaticMeshComponent, no Skeletal.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA|Physics")
    class UStaticMeshComponent* Mesh;

    // Sistema de sirenas y luces
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA|Effects")
    bool bSirensActive = false;

    // Velocidad de entrada táctica
    UPROPERTY(EditAnywhere, Category = "AAA|Movement")
    float TargetSpeed = 1200.f;

    UFUNCTION(BlueprintCallable, Category = "AAA|Movement")
    void MoveToLocationTactic(FVector TargetLocation);

protected:
    virtual void Tick(float DeltaTime) override;

private:
    FVector GoalLocation;
    bool bIsMoving = false;
};
