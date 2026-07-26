#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AI/AlsasuaAIController.h"
#include "AlsasuaSquadManager.generated.h"

UENUM(BlueprintType)
enum class ESquadTactic : uint8
{
    Patrol,     // Vigilancia rutinaria dispersa
    Contain,    // Crear perímetros alrededor de la plaza
    Encircle,   // Rodear al jugador (Wanted Level alto)
    Support     // Reforzar una zona donde haya conflicto
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaSquadManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;

    // Registra una unidad en el sistema táctico
    void RegisterUnit(AAlsasuaAIController* Unit);
    void UnregisterUnit(AAlsasuaAIController* Unit);

    // Cambia la táctica global de la IA en Alsasua
    UFUNCTION(BlueprintCallable, Category = "AAA|AI")
    void SetGlobalTactic(ESquadTactic NewTactic);

private:
    TArray<TWeakObjectPtr<AAlsasuaAIController>> ActiveUnits;
    ESquadTactic CurrentTactic = ESquadTactic::Patrol;
    float TacticTimer = 0.f;

    void ExecuteEncircleTactic();
    void ExecuteContainTactic();
    void ExecutePatrolTactic();
    void ExecuteSupportTactic();
};
