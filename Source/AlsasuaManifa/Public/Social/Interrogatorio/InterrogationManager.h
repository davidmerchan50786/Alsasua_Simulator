#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "InterrogationManager.generated.h"

UENUM(BlueprintType)
enum class EInterrogationTactic : uint8 {
    Empathy,
    Intimidation,
    Corruption // Soborno
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInterrogationSuccess, FName, DiscoveredNodeId);

UCLASS()
class ALSASUAMANIFA_API UInterrogationManager : public UWorldSubsystem {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Alsasua|Social")
    void StartInterrogation(FName TargetNPCId, float NPCResistance);

    UFUNCTION(BlueprintCallable, Category="Alsasua|Social")
    void ApplyTactic(EInterrogationTactic Tactic);

    UPROPERTY(BlueprintAssignable)
    FOnInterrogationSuccess OnInterrogationSuccess;

private:
    float CurrentResistance;
    FName CurrentTarget;
};