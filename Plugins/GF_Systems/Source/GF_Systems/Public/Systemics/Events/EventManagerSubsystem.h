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
class GF_SYSTEMS_API UEventManagerSubsystem : public UWorldSubsystem, public IAlsasuaFuenteEventosMundo, public IAlsasuaPilarArranque, public IAlsasuaPilarTiquear {
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // IAlsasuaPilarArranque: aparece en el log del director y se registra para ticar.
    // No ejecuta construccion propia (todo es runtime), pero queda visible en el arranque.
    virtual int32 EjecutarArranque() override { return 0; }
    virtual FString EtiquetaArranque() const override { return TEXT("director de eventos (world events)"); }
    virtual int32 OrdenArranque() const override { return 60; }

    // IAlsasuaPilarTiquear: el DirectorArranque lo avanza cada frame.
    virtual void TiquearPilar(float DeltaTime) override { TickDirector(DeltaTime); }

    // Evalúa el estado del mundo y dispara eventos si se cumplen las condiciones
    UFUNCTION(BlueprintCallable, Category="AAA|Director")
    void TickDirector(float DeltaTime);

    UFUNCTION(BlueprintImplementableEvent, Category="AAA|Director")
    void OnEventTriggered(const FWorldEventData& EventData);

    UFUNCTION(BlueprintCallable, Category="AAA|Director")
    void HandleEvidenceCollected(AActor* Owner, FName Tag);

    // Trigger un evento del mundo a peticion (demo/showcase): lo transmite por
    // el bus OnDirectorAction (HUD + radio). La consola `Alsasua.Evento N` lo llama.
    UFUNCTION(BlueprintCallable, Category="AAA|Director")
    void ForzarEvento(const FText& Mensaje);

    // UHT no resuelve typedefs en UPROPERTY: tipo real del contrato kernel.
    UPROPERTY(BlueprintAssignable)
    FAlsasuaEventoMundo OnDirectorAction;

    //~ IAlsasuaFuenteEventosMundo
    virtual FAlsasuaEventoMundo& EventoMundo() override { return OnDirectorAction; }

private:
    float CheckTimer = 0.f;
    const float CheckInterval = 5.0f; // Evalúa cada 5 segundos para ahorrar CPU

    void EvaluateWorldState();

    // ── Calendario de festividades (eventos programados) ──────────────────
    // Reloj de juego acelerado: 1 minuto real = 1 hora de juego, meses de 30
    // días. Hace avanzar el día y dispara las fiestas reales de Alsasua.
    float FestivoGameHours = 0.f;
    int32 FestivoDay = 1;
    int32 FestivoMonth = 1;

    void TickFestivals(float DeltaTime);
    void CheckFestivalDay();
    void StartFestival(const FText& Nombre, const FText& Descripcion);
};