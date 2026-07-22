#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "BaseVehicle.generated.h"

UCLASS()
class ALSASUAMANIFA_API ABaseVehicle : public APawn
{
    GENERATED_BODY()

public:
    ABaseVehicle();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    class UBoxComponent* CollisionBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    class UStaticMeshComponent* VehicleMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    class UFloatingPawnMovement* MovementComponent;

    // Estadisticas de conduccion
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Physics")
    float MaxSpeed = 2500.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Physics")
    float AccelerationExp = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Physics")
    float TurnSpeed = 45.f;

    UPROPERTY(BlueprintReadOnly, Category="AAA|Status")
    float CurrentSpeedKmh = 0.f;

    UFUNCTION(BlueprintCallable, Category="AAA|Drive")
    virtual void Drive(float ForwardValue, float RightValue);

    UFUNCTION(BlueprintCallable, Category="AAA|Drive")
    virtual void ToggleEngine(bool bOn);

protected:
    virtual void Tick(float DeltaTime) override;

    bool bEngineActive = true;
};
