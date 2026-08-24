#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Delegates/Delegate.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "Arranque/AlsasuaPilarArranque.h"
#include "Systemics/Urban/UrbanStateSubsystem.h"
#include "EventManagerSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FWorldEventData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName EventID;

    UPROPERTY(BlueprintReadOnly)
    FText EventAnnounceMessage;

    UPROPERTY(BlueprintReadOnly)
    float Probability = 1.0f;
};

UCLASS()
class GF_SYSTEMS_API UEventManagerSubsystem : public UWorldSubsystem, public IAlsasuaFuenteEventosMundo {
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // Evalúa el estado del mundo y dispara eventos si se cumplen las condiciones
    UFUNCTION(BlueprintCallable, Category="AAA|Director")
    void TickDirector(float DeltaTime);

    UFUNCTION(BlueprintImplementableEvent, Category="AAA|Director")
    void OnEventTriggered(const FWorldEventData& EventData);

    UFUNCTION(BlueprintCallable, Category="AAA|Director")
    void HandleEvidenceCollected(AActor* Owner, FName Tag);

    // UHT no resuelve typedefs en UPROPERTY: tipo real del contrato kernel.
    UPROPERTY(BlueprintAssignable)
    FAlsasuaEventoMundo OnDirectorAction;

    //~ IAlsasuaFuenteEventosMundo
    virtual FAlsasuaEventoMundo& EventoMundo() override { return OnDirectorAction; }

private:
    float CheckTimer = 0.f;
    const float CheckInterval = 5.0f; // Evalúa cada 5 segundos para ahorrar CPU

    void EvaluateWorldState();
};