// AlsasuaTelemetria.h
// svc-telemetria del plan §7, versión in-process mínima: cada segundo envía
// una línea JSON {fps, ms, mb, pilares} por UDP a localhost. El "servicio
// fuera de proceso" es Tools/DashboardTelemetria.py, que solo escucha.
//
// Apagada por defecto (cero coste): se activa con -AlsasuaTelemetria o
// TelemetriaActivada=True en [/Script/AlsasuaKernel.AlsasuaTelemetria].
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AlsasuaTelemetria.generated.h"

class FSocket;

UCLASS(Config = Game)
class ALSASUAKERNEL_API UAlsasuaTelemetria : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

private:
	UPROPERTY(Config)
	bool TelemetriaActivada = false;

	UPROPERTY(Config)
	int32 Puerto = 7777;

	FTimerHandle MangoLatido;
	FSocket* Socket = nullptr;
	TSharedPtr<FInternetAddr> Destino;

	void Latido();
};
