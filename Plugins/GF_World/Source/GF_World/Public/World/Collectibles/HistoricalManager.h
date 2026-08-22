#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/Collectibles/CollectibleData.h"
#include "HistoricalManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemUnlocked, FHistoricalCollectible, Item);

UCLASS()
class GF_WORLD_API UHistoricalManager : public UWorldSubsystem {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Alsasua|History")
    void UnlockHistoricalItem(FHistoricalCollectible Item);
    UPROPERTY(BlueprintAssignable, Category="Alsasua|History")
    FOnItemUnlocked OnItemUnlocked;
    UPROPERTY(BlueprintReadOnly, Category="Alsasua|History")
    TArray<FHistoricalCollectible> UnlockedItems;
};