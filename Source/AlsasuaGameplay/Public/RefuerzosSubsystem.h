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

	/** Roadblock (spike strip) class — deployed at wanted 3+. SoftClassPath. */
	UPROPERTY(EditAnywhere, Category="Refuerzos|Roadblock") FSoftClassPath ClaseSpikeStrip;
	/** Number of spike strips per roadblock deployment. */
	UPROPERTY(EditAnywhere, Category="Refuerzos|Roadblock") int32 SpikeStripsPerBlock = 3;

	/** Helicopter class — deployed at wanted 4+. SoftClassPath. */
	UPROPERTY(EditAnywhere, Category="Refuerzos|Helicopter") FSoftClassPath ClaseHelicoptero;

	/** Set by external tension systems before wanted fires. Reset after each wave. */
	UPROPERTY(BlueprintReadWrite, Category="Refuerzos|Zones") float SpawnCountMultiplier = 1.f;
	UPROPERTY(BlueprintReadWrite, Category="Refuerzos|Zones") float SpawnRadiusMultiplier = 1.f;

private:
	float UltimaOleada = -1000.f;

	UFUNCTION()
	void OnWanted(int32 Nivel);

	UFUNCTION()
	void OnGuardCombat(AActor* Guard);

	void Despachar(int32 Cantidad);
	void SpawnVehiculoPolicia(FVector Centro, FRotator Rotacion);
	void DesplegarReten(FVector Centro);
	void DesplegarHelicoptero(FVector Centro);
};
