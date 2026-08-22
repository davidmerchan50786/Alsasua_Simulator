#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaCore.h"
#include "AlsasuaInventoryComponent.generated.h"

// Interface para objetos usables (Megáfonos, Pancartas, Kits)
UINTERFACE(MinimalAPI)
class UAlsasuaItemUsable : public UInterface { GENERATED_BODY() };

class IAlsasuaItemUsable {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, Category = "AAA|Inventory")
    bool OnUse(AActor* User);
};

USTRUCT(BlueprintType)
struct FAlsasuaItemData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ItemID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Quantity = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxStack = 10; // Estándar AAA

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Weight = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UTexture2D* Icon = nullptr;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GF_SYSTEMS_API UAlsasuaInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAlsasuaInventoryComponent();

    UFUNCTION(BlueprintCallable, Category = "AAA|Inventory")
    bool AddItem(FAlsasuaItemData NewItem);

    UFUNCTION(BlueprintCallable, Category = "AAA|Inventory")
    bool RemoveItem(FName ItemID, int32 Amount = 1);

    UFUNCTION(BlueprintCallable, Category = "AAA|Inventory")
    void UseItem(FName ItemID);

    UPROPERTY(BlueprintAssignable, Category = "AAA|Inventory")
    FOnInventoryChanged OnInventoryChanged;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA|Inventory")
    TArray<FAlsasuaItemData> Items;

    UPROPERTY(EditAnywhere, Category = "AAA|Inventory")
    float MaxWeight = 50.0f;

    UPROPERTY(BlueprintReadOnly, Category = "AAA|Inventory")
    float CurrentWeight = 0.0f;

private:
    void RecalculateWeight();
};
