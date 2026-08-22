#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "AlsasuaCore.h"
#include "AlsasuaTacticManager.generated.h"

UENUM(BlueprintType)
enum class EAlsasuaTactic : uint8 { March, Blockade, Scatter, SitIn };

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTacticChanged, EAlsasuaTactic, NewTactic);

UCLASS()
class GF_AI_API UAlsasuaTacticManager : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override { return !IsTemplate(); }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UAlsasuaTacticManager, STATGROUP_Game); }

    // Delegado para que los Agentes se suscriban una sola vez (AAA Performance)
    UPROPERTY(BlueprintAssignable, Category = "AAA|Tactics")
    FOnTacticChanged OnTacticChanged;

    UFUNCTION(BlueprintCallable, Category = "AAA|Tactics")
    void SetGlobalTactic(EAlsasuaTactic NewTactic);

private:
    EAlsasuaTactic CurrentTactic = EAlsasuaTactic::March;
    float AccumulatedTime = 0.f;
    float EvaluationInterval = 2.f;
};
