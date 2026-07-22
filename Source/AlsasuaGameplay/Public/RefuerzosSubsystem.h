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

private:
	float UltimaOleada = -1000.f;

	UFUNCTION()
	void OnWanted(int32 Nivel);

	void Despachar(int32 Cantidad);
};
