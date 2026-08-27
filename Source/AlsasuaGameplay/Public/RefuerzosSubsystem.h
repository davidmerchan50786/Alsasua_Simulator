// RefuerzosSubsystem.h (capa GAMEPLAY)
// Spawnea oleadas de policía cuando sube el nivel de búsqueda. Puerto de
// ISpawnService.SolicitarRefuerzosPolicia + el flujo de wanted.
// UWorldSubsystem porque necesita mundo para spawnear actores.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RefuerzosSubsystem.generated.h"

UCLASS()
class ALSASUAGAMEPLAY_API URefuerzosSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	UPROPERTY(EditAnywhere, Category="Refuerzos") float Cooldown = 30.f;   // s entre oleadas
	UPROPERTY(EditAnywhere, Category="Refuerzos") float RadioSpawn = 2500.f;

	/** Spawn police vans at wanted level >= this threshold */
	UPROPERTY(EditAnywhere, Category="Refuerzos") int32 NivelMinimoVan = 3;

	/** Extra cops spawned per 0.1 tension above 0.5 */
	UPROPERTY(EditAnywhere, Category="Refuerzos") float EscalacionTension = 2.f;

private:
	float UltimaOleada = -1000.f;

	UFUNCTION()
	void OnWanted(int32 Nivel);

	void Despachar(int32 Cantidad);
	void SpawnPoliceVan();
};
