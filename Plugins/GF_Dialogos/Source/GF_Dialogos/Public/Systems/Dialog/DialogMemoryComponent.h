#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "DialogMemoryComponent.generated.h"

USTRUCT(BlueprintType)
struct FDialogMemoryEntry {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGameplayTag MemoryTag;

    UPROPERTY(BlueprintReadWrite)
    float Value = 0.f;

    UPROPERTY(BlueprintReadWrite)
    float ExpirationTime = -1.f; // -1 significa persistente
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GF_DIALOGOS_API UDialogMemoryComponent : public UActorComponent {
    GENERATED_BODY()
public:
    UDialogMemoryComponent();

    UFUNCTION(BlueprintCallable, Category="AAA|Dialog")
    void AddMemory(FGameplayTag Tag, float Value, float Duration = -1.f);

    UFUNCTION(BlueprintPure, Category="AAA|Dialog")
    bool HasMemory(FGameplayTag Tag) const;

    UFUNCTION(BlueprintPure, Category="AAA|Dialog")
    float GetMemoryValue(FGameplayTag Tag) const;

private:
    UPROPERTY()
    TMap<FGameplayTag, FDialogMemoryEntry> MemoryBank;
};
