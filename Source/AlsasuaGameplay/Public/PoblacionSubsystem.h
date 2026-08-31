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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNoiseAtLocation, FVector, Location);

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
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UPoblacionSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }

	UFUNCTION(BlueprintCallable, Category="Poblacion")
	void HuirDe(FVector Location);

	/** Startle civilians within StartleRadius of Location (e.g., fast car passing). */
	UFUNCTION(BlueprintCallable, Category="Poblacion")
	void StartlePeds(FVector Location);

	/** Static: fires on loud noise. Pedestrians flee. */
	static FOnNoiseAtLocation OnLoudNoise;

	/** Scream sound played by a nearby civilian when panic/flee triggers. */
	UPROPERTY(EditAnywhere, Category="Poblacion|Reaction")
	class USoundBase* SScream = nullptr;

	/** Startle radius (cm): fast cars / explosions within this cause a startled hop + flee. */
	UPROPERTY(EditAnywhere, Category="Poblacion|Reaction")
	float StartleRadius = 1200.f;

private:
	float Acum = 0.f;
	bool bPanicMode = false;
	UPROPERTY() TArray<APeatonActor*> Peatones;

	UFUNCTION()
	void OnWantedChange(int32 Nivel);
	UFUNCTION()
	void HandleLoudNoise(FVector Location);
	void Mantener();
	bool PuntoEnAnillo(const FVector& Centro, FVector& OutPunto) const;
	void StartleNearFastVehicles(const FVector& PlayerLoc);
};
