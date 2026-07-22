// CargadorPoligonos.h (capa WORLD)
// Carga plazas (5) y zonas verdes (273) desde sus *_unity.json (poly = [x,z,...]
// plano, marco ABSOLUTO) como superficies planas drapeadas (APoligonoSuelo).
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
