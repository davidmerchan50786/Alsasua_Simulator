// CargadorVias.h (capa WORLD)
// Carga las vías lineales secundarias (aceras, ferrocarril, ríos) desde sus
// *_unity.json (marco Unity ABSOLUTO, pts = [x,y,z,...] plano) reutilizando
// ACalleGenerada como cinta drapeada. Puerto de los generadores OSM de aceras/
// vías/cauces. Carga incremental para el director.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CargadorVias.generated.h"

USTRUCT()
struct FTrabajoVia
{
	GENERATED_BODY()
	TArray<FVector2D> PuntosMundo;   // cm
	float AnchoCm = 400.f;
	float EpsilonCm = 10.f;
	FName Tag = NAME_None;
	FString Tipo;
};

UCLASS()
class ALSASUAWORLD_API UCargadorVias : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Mundo") bool bAutoCargar = true;

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	UFUNCTION(BlueprintCallable, Category="Mundo") int32 Cargar();
	UFUNCTION(BlueprintCallable, Category="Mundo") FString GetDebugSummary() const;
	void PrepararCarga();
	bool PasoPresupuesto(double PresupuestoMs);
	bool Terminado() const { return bPreparado && Idx >= Trabajos.Num(); }
	int32 Construidas = 0;

private:
	bool bHecho = false;
	bool bPreparado = false;
	int32 Idx = 0;
	TArray<FTrabajoVia> Trabajos;

	// Lee un dataset de pts planos [x,y,z,...] (absoluto) al worklist.
	void Encolar(const FString& RutaRel, FName Tag, float EpsilonCm,
	             float AnchoDefectoM, bool bAnchoPorTracks);
};
