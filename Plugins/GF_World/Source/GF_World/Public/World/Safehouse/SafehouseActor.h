#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Systems/Disguise/DisguiseComponent.h"
#include "SafehouseActor.generated.h"

UCLASS()
class GF_WORLD_API ASafehouseActor : public AActor
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

    /**
     * Cambiar el disfraz del jugador.
     * @param PlayerActor    Actor del jugador (debe tener UDisguiseComponent).
     * @param NewOutfitId    Nombre del outfit ("Momotxorro", "Casual", "Press").
     */
    UFUNCTION(BlueprintCallable, Category="AAA|Safehouse")
    void ChangeDisguise(AActor* PlayerActor, FName NewOutfitId);

    UFUNCTION(BlueprintCallable, Category="AAA|Safehouse")
    void DepositEvidence();

    UPROPERTY(BlueprintReadOnly, Category="AAA|Safehouse")
    TArray<int32> EvidenceDeposited;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere)
    class UBoxComponent* InteractionZone;

    static EDisguiseType ParseOutfitName(FName OutfitId);
};
