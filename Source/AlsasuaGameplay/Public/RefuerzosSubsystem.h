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

	UPROPERTY(EditAnywhere, Category="Refuerzos") float Cooldown = 30.f;
	UPROPERTY(EditAnywhere, Category="Refuerzos") float RadioSpawn = 2500.f;

	/** Clase de refuerzo vehicular (wanted 3+). SoftClassPath para no depender
	 *  de GF_Vehiculos en compile-time. */
	UPROPERTY(EditAnywhere, Category="Refuerzos") FSoftClassPath ClaseVehiculoPolicia;

private:
	float UltimaOleada = -1000.f;

	UFUNCTION()
	void OnWanted(int32 Nivel);

	UFUNCTION()
	void OnGuardCombat(AActor* Guard);

	void Despachar(int32 Cantidad);
	void SpawnVehiculoPolicia(FVector Centro, FRotator Rotacion);
};
