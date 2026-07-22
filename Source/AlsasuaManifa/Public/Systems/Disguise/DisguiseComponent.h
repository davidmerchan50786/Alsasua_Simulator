#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DisguiseComponent.generated.h"

UENUM(BlueprintType)
enum class EDisguiseType : uint8 {
    None,
    Momotxorro, // Disfraz tradicional (Sakoa, Adarrak, Mozorroa)
    Casual_Infiltrator,
    Press_Pass
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDisguiseBroken, EDisguiseType, Type);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UDisguiseComponent : public UActorComponent {
    GENERATED_BODY()
public:
    UDisguiseComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Disguise")
    EDisguiseType CurrentDisguise = EDisguiseType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Disguise")
    float Durability = 100.f; // Se gasta al correr, saltar o realizar actos sospechosos

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Disguise")
    bool bIsConsumable = false; // Si es true, se destruye al agotarse o desequipar

    UPROPERTY(BlueprintAssignable)
    FOnDisguiseBroken OnDisguiseBroken;

    UFUNCTION(BlueprintCallable, Category="AAA|Disguise")
    void EquipDisguise(EDisguiseType Type, bool bConsumable, float InitialDurability = 100.f);

    UFUNCTION(BlueprintCallable, Category="AAA|Disguise")
    void UseDisguise(float Amount); // Llamar al realizar acciones (atacar, correr ante GC)

    UFUNCTION(BlueprintPure, Category="AAA|Disguise")
    float GetDetectionMultiplier() const;

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};