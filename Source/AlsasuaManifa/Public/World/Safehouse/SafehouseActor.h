#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SafehouseActor.generated.h"

UCLASS()
class ALSASUAMANIFA_API ASafehouseActor : public AActor
{
    GENERATED_BODY()

public:
    ASafehouseActor();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Safehouse")
    bool bIsUnlocked = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Safehouse")
    FString SafehouseName;

    UFUNCTION(BlueprintCallable, Category="AAA|Safehouse")
    void EnterSafehouse(AActor* PlayerActor);

    UFUNCTION(BlueprintCallable, Category="AAA|Safehouse")
    void ChangeDisguise(AActor* PlayerActor, FName NewOutfitId);

    UFUNCTION(BlueprintCallable, Category="AAA|Safehouse")
    void DepositEvidence();

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere)
    class UBoxComponent* InteractionZone;
};
