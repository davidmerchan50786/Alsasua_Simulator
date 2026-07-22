#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AlsasuaCore.h"
#include "AlsasuaTacticManager.generated.h"

UENUM(BlueprintType)
enum class EAlsasuaTactic : uint8 { March, Blockade, Scatter, SitIn };

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTacticChanged, EAlsasuaTactic, NewTactic);

UCLASS()
class ALSASUAMANIFA_API UAlsasuaTacticManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    void Tick(float DeltaTime);

    // Delegado para que los Agentes se suscriban una sola vez (AAA Performance)
    UPROPERTY(BlueprintAssignable, Category = "AAA|Tactics")
    FOnTacticChanged OnTacticChanged;

    UFUNCTION(BlueprintCallable, Category = "AAA|Tactics")
    void SetGlobalTactic(EAlsasuaTactic NewTactic);

private:
    EAlsasuaTactic CurrentTactic = EAlsasuaTactic::March;
};
