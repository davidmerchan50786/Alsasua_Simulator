// CargadorPoligonos.h (capa WORLD)
// Carga plazas (5) y zonas verdes (273) desde sus *_unity.json (poly = [x,z,...]
// plano, marco ABSOLUTO) como superficies planas drapeadas (APoligonoSuelo).
//
// Lo llama ADirectorArranque en la fase 1b, pegado al terreno y antes de árboles,
// calles y edificios: Construir() muestrea Z con un LineTrace por ECC_Visibility
// que no filtra por actor, así que solo drapea sobre el terreno si es lo único
// que hay debajo.
//
// Dos cosas a tener en cuenta si se toca:
//  - Son 278 actores, uno por polígono, y cada uno es una sección de
//    ProceduralMesh, o sea 278 draw calls sobre los ~819 de referencia del
//    RESUMEN_TECNICO. Si pesa, lo que toca es fusionarlos por tipo, no crear más.
//  - Pinta color plano (verde 58,122,53 y adoquín 150,145,135) sobre la ortofoto
//    PNOA del terreno. Si se quiere ver la foto aérea debajo, el camino es teñir
//    la ortofoto por zona en el material, no subir el epsilon.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CargadorPoligonos.generated.h"

USTRUCT()
struct FTrabajoSuelo
{
	GENERATED_BODY()
	TArray<FVector2D> Anillo;   // cm
	FColor Color = FColor(120, 120, 120);
	FString Tipo;
	float EpsilonCm = 6.f;
};

UCLASS()
class ALSASUAWORLD_API UCargadorPoligonos : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Mundo") bool bAutoCargar = true;

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	UFUNCTION(BlueprintCallable, Category="Mundo") int32 Cargar();
	void PrepararCarga();
	bool PasoPresupuesto(double PresupuestoMs);
	bool Terminado() const { return bPreparado && Idx >= Trabajos.Num(); }
	int32 Construidos = 0;

private:
	bool bHecho = false;
	bool bPreparado = false;
	int32 Idx = 0;
	TArray<FTrabajoSuelo> Trabajos;
	void Encolar(const FString& RutaRel, FColor ColorDefecto, float EpsilonCm);
};
