#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "AlsasuaGameInstance.generated.h"

UCLASS()
class ALSASUAKERNEL_API UAlsasuaGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    UAlsasuaGameInstance();

    UFUNCTION(BlueprintCallable, Category = "Alsasua|GameInstance")
    void SaveGame(FString SlotName);

    UFUNCTION(BlueprintCallable, Category = "Alsasua|GameInstance")
    void LoadGame(FString SlotName);

    UFUNCTION(BlueprintCallable, Category = "Alsasua|GameInstance")
    bool HasSaveData(FString SlotName) const;
};
