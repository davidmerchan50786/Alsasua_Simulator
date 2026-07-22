#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Items/Explosives/ExplosiveDeviceData.h"
#include "VehicleTarget.generated.h"

UCLASS()
class ALSASUAMANIFA_API AVehicleTarget : public APawn {
    GENERATED_BODY()

public:
    AVehicleTarget();

    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UStaticMeshComponent* VehicleMesh;

    // Health for vehicle
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle")
    float MaxHealth = 100.f;

    UPROPERTY(BlueprintReadOnly, Category="Vehicle")
    float CurrentHealth = 100.f;

    // Anchor sockets where charges can be attached
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle")
    TArray<FName> ChargeAnchors;

    UFUNCTION(BlueprintCallable, Category="Vehicle")
    bool ReceiveRadialImpulse(const FVector& Origin, float Force, float Damage);

    UFUNCTION(BlueprintCallable, Category="Vehicle")
    void IgniteAtLocation(const FVector& Loc, UParticleSystem* FireFX, float Duration);

    UFUNCTION()
    void OnDestroyedByExplosion();
};