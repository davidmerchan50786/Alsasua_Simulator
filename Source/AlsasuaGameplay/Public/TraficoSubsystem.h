// TraficoSubsystem.h (capa GAMEPLAY)
// Mantiene unos pocos vehículos ambientales recorriendo los ejes de calle
// (UCargadorCalles, capa World) cercanos al jugador. Recicla los que terminan
// su ruta o se alejan. Puerto de la parte de tráfico de SistemaTrafico.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "TraficoSubsystem.generated.h"

class AVehiculoAmbiente;

UCLASS()
class ALSASUAGAMEPLAY_API UTraficoSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Trafico") int32 MaxVehiculos = 12;
	UPROPERTY(EditAnywhere, Category="Trafico") float RadioMin = 2000.f;   // 20 m
	UPROPERTY(EditAnywhere, Category="Trafico") float RadioMax = 12000.f;  // 120 m
	UPROPERTY(EditAnywhere, Category="Trafico") float RadioCull = 18000.f; // 180 m
	UPROPERTY(EditAnywhere, Category="Trafico") float VelMin = 700.f;      // ~25 km/h
	UPROPERTY(EditAnywhere, Category="Trafico") float VelMax = 1400.f;     // ~50 km/h
	UPROPERTY(EditAnywhere, Category="Trafico") float PeriodoMantenimiento = 1.5f;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UTraficoSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }

private:
	float Acum = 0.f;
	UPROPERTY() TArray<AVehiculoAmbiente*> Vehiculos;
	void Mantener();
};
