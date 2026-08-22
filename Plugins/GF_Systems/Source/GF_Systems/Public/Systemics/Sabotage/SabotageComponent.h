#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SabotageComponent.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UENUM(BlueprintType)
enum class EDamageType : uint8 {
    None,
    Fire,
    Explosive,
    Structural,
    RoadBlock
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDamageChanged, float, NewDamage);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GF_SYSTEMS_API USabotageComponent : public UActorComponent {
    GENERATED_BODY()
public:
    USabotageComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Sabotage")
    float CurrentDamage = 0.f;

    UPROPERTY(BlueprintReadOnly, Category="AAA|Sabotage")
    EDamageType ActiveDamage = EDamageType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Sabotage")
    float RepairRate = 5.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Sabotage")
    bool bCanRepair = true;

    UPROPERTY(BlueprintAssignable, Category="AAA|Sabotage")
    FOnDamageChanged OnDamageChanged;

    UFUNCTION(BlueprintCallable, Category="AAA|Sabotage")
    void BeginSabotage(EDamageType Type, float Intensity);

    UFUNCTION(BlueprintCallable, Category="AAA|Sabotage")
    void Repair(float DeltaTime);

    UFUNCTION(BlueprintPure, Category="AAA|Sabotage")
    float GetCurrentDamage() const { return CurrentDamage; }

protected:
    void ApplyVisualDamage();
    void ClearVisualDamage();

private:
    void BroadcastIfChanged(float OldDamage);

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> CachedMesh;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> DamageMIDs;

    UPROPERTY(Transient)
    TObjectPtr<AActor> FireIndicator;

    FVector OriginalScale = FVector::OneVector;
    bool bOriginalCaptured = false;
};
