#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DirectorArranque.generated.h"

class USubsystem;

/**
 * Orquestador del arranque del mundo TRONCO (terreno, calles, edificios,
 * plaza, puentes, POIs...). Las fases de los PILARES GF_* ya no se conocen
 * aqui: cada plugin activo implementa IAlsasuaPilarArranque (Kernel) y el
 * director solo las ordena y las registra. Un combo sin un pilar salta su
 * fase sin tocar las demas.
 */
UCLASS()
class ALSASUAWORLD_API ADirectorArranque : public AActor
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|World")
    void IniciarConstruccion();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Arranque")
    bool bCargarArboles = true;

    /**
     * Semáforos de la fase 46. La fase estuvo saltada con un log de "skip para
     * perfilado" desde que se midió el arranque, así que el sistema no se
     * ejecutaba nunca. Vuelve a la cadena detrás de esta bandera: para volver a
     * medir sin ellos se baja esto, no se comenta la fase.
     *
     * Coste: hasta MaxSemaforos cruces (12 por defecto), cada uno con poste,
     * caja y UNA luz puntual. Antes eran tres luces por semáforo, todas
     * encendidas a la vez.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Arranque")
    bool bSemaforos = true;

private:
    bool bConstruccionCompleta = false;

    /** Pilares que pidieron tiqueo (peatones, trafico, clima->audio...). */
    UPROPERTY()
    TArray<TObjectPtr<USubsystem>> PilaresTiqueables;
};
