#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "AlsasuaAudioPilar.generated.h"

class UAlsasuaAmbientAudioSystem;

/**
 * Fase 30 del antiguo DirectorArranque: crea el componente de audio ambiental
 * y tiquea el puente clima->audio leyendo IAlsasuaEstadoClima del pilar de
 * clima, sin acoplarse a el.
 */
UCLASS()
class GF_AUDIO_API UAlsasuaAudioPilar : public UWorldSubsystem,
	public IAlsasuaPilarArranque, public IAlsasuaPilarTiquear
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;

	virtual int32 EjecutarArranque() override;
	virtual FString EtiquetaArranque() const override { return TEXT("audio ambiental activado"); }
	virtual int32 OrdenArranque() const override { return 300; }

	virtual void TiquearPilar(float DeltaTime) override;

private:
	UPROPERTY()
	TObjectPtr<UAlsasuaAmbientAudioSystem> Componente;

	UPROPERTY()
	TWeakObjectPtr<USubsystem> FuenteClima;
};
