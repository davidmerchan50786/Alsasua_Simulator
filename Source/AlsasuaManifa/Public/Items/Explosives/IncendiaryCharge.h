#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Items/Explosives/ExplosiveDeviceData.h"
#include "IncendiaryCharge.generated.h"

UCLASS()
class ALSASUAMANIFA_API AIncendiaryCharge : public AActor {
    GENERATED_BODY()

public:
    AIncendiaryCharge();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Armar y detonar
    UFUNCTION(BlueprintCallable, Category="Explosive")
    void ArmCharge(float FuseTime);

    UFUNCTION(BlueprintCallable, Category="Explosive")
    void Detonate();

    // Colocación en vehículo (anchoring)
    UFUNCTION(BlueprintCallable, Category="Explosive")
    bool AttachToAnchor(AActor* TargetActor, FName AnchorSocket);

    // Data
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Explosive")
    UExplosiveDeviceData* DeviceData;

protected:
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* MeshComp;

    UPROPERTY(VisibleAnywhere)
    USceneComponent* Root;

private:
    FTimerHandle FuseTimerHandle;
    bool bArmed = false;

    void Explode_Internal();

    FTimerHandle SlowMoHandle;
};