#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ResistanceTool.generated.h"

UENUM(BlueprintType)
enum class EToolType : uint8 {
    Slingshot,  // Tirachinas para romper cámaras/atenciones
    SmokeBomb,  // Bote de humo para huida
    Megaphone   // Megáfono para atraer masa
};

UCLASS()
class GF_SYSTEMS_API AResistanceTool : public AActor {
    GENERATED_BODY()
public:
    AResistanceTool();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Tool")
    EToolType ToolType = EToolType::Slingshot;

    UFUNCTION(BlueprintCallable, Category="AAA|Tool")
    void UseTool(FVector TargetLocation);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Tool")
    float EffectRadius = 1000.f;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY() class UNiagaraSystem* NSSlingshot = nullptr;
    UPROPERTY() class USoundBase* SSlingshot = nullptr;
    UPROPERTY() class UNiagaraSystem* NSSmoke = nullptr;
    UPROPERTY() class USoundBase* SSmoke = nullptr;
    UPROPERTY() class UNiagaraSystem* NSRally = nullptr;
    UPROPERTY() class USoundBase* SMega = nullptr;
};