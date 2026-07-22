#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AlsasuaOutcomeSystem.generated.h"

USTRUCT(BlueprintType)
struct FAlsasuaMissionResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bSuccess = false;

    UPROPERTY(BlueprintReadOnly)
    FText FinalVerdict;

    UPROPERTY(BlueprintReadOnly)
    int32 BonusCredits = 0;

    UPROPERTY(BlueprintReadOnly)
    TArray<FString> AccomplishedFeats;
};

/** Sistema que calcula el impacto histórico de la manifestación */
UCLASS()
class ALSASUAMANIFA_API UAlsasuaOutcomeSystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // Evalúa la sesión actual y genera un informe de consecuencias
    UFUNCTION(BlueprintCallable, Category = "AAA|Gameplay")
    FAlsasuaMissionResult EvaluateManifestation();

    // Dispara el evento de fin de juego y guarda el progreso
    UFUNCTION(BlueprintCallable, Category = "AAA|Gameplay")
    void FinalizeSession();

private:
    void ProcessFeats(FAlsasuaMissionResult& OutResult);
};
