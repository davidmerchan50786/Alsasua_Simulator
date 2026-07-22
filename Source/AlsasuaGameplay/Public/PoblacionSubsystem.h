// PoblacionSubsystem.h (capa GAMEPLAY)
// Mantiene una población de peatones ambientales alrededor del jugador sobre la
// navmesh: spawnea en un anillo y recicla los lejanos (presupuesto). Puerto de
// la parte de peatones de SistemaTrafico + streaming de multitud.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "PoblacionSubsystem.generated.h"

class APeatonActor;

UCLASS()
class ALSASUAGAMEPLAY_API UPoblacionSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Poblacion") int32 MaxPeatones = 40;
	UPROPERTY(EditAnywhere, Category="Poblacion") float RadioMin = 1500.f;   // 15 m
	UPROPERTY(EditAnywhere, Category="Poblacion") float RadioMax = 6000.f;   // 60 m
	UPROPERTY(EditAnywhere, Category="Poblacion") float RadioCull = 9000.f;  // 90 m -> reciclar
	UPROPERTY(EditAnywhere, Category="Poblacion") int32 SpawnsPorTick = 3;
	UPROPERTY(EditAnywhere, Category="Poblacion") float PeriodoMantenimiento = 1.f;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UPoblacionSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }

private:
	float Acum = 0.f;
	UPROPERTY() TArray<APeatonActor*> Peatones;

	void Mantener();
	bool PuntoEnAnillo(const FVector& Centro, FVector& OutPunto) const;
};
