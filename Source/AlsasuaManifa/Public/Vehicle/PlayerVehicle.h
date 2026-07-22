#pragma once

#include "CoreMinimal.h"
#include "Vehicle/BaseVehicle.h"
#include "PlayerVehicle.generated.h"

UCLASS()
class ALSASUAMANIFA_API APlayerVehicle : public ABaseVehicle
{
    GENERATED_BODY()

public:
    APlayerVehicle();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    class USpringArmComponent* SpringArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    class UCameraComponent* Camera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    class UVehicleDamageComponent* DamageComponent;

    // Sonidos dinámicos
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    class UAudioComponent* EngineAudio;

    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
    void MoveForward(float Val);
    void MoveRight(float Val);
    virtual void Tick(float DeltaTime) override;

private:
    float TargetPitch = 1.0f;
};
