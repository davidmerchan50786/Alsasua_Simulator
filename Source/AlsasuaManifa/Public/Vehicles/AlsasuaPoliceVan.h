#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AlsasuaPoliceVan.generated.h"

UCLASS()
class ALSASUAMANIFA_API AAlsasuaPoliceVan : public APawn
{
    GENERATED_BODY()

public:
    AAlsasuaPoliceVan();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA|Physics")
    class USkeletalMeshComponent* Mesh;

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
