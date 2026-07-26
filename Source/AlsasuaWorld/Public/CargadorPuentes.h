// CargadorPuentes.h (capa WORLD)
// Genera geometría de puentes donde carreteras cruzan el río Arakil
// y las vías férreas. Lee waterways_unity.json + roads_unity.json
// e intersecciona para encontrar cruces.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CargadorPuentes.generated.h"

class UProceduralMeshComponent;

USTRUCT(BlueprintType)
struct FPuenteData
{
	GENERATED_BODY()

	FVector PosicionMundo;
	FVector Direccion;
	float AnchoCalzada;
	float LargoPuente;
	bool bEsFerrocarril = false;
	FString NombreCalle;
};

UCLASS()
class ALSASUAWORLD_API UCargadorPuentes : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	UFUNCTION(BlueprintCallable, Category="Mundo")
	int32 GenerarPuentes();

private:
	void DetectarCruces();
	void SpawnPuente(const FPuenteData& Data);
	TArray<FPuenteData> PuentesEncontrados;

	// Datos de vías fluviales (cargados por CargadorVias).
	TArray<TArray<FVector>> Rios;
	// Datos de carreteras.
	TArray<TPair<TArray<FVector>, FString>> Calles;
};
