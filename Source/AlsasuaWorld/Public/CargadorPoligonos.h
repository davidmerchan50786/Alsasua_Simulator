// CargadorPoligonos.h (capa WORLD)
// Carga plazas (5) y zonas verdes (273) desde sus *_unity.json (poly = [x,z,...]
// plano, marco ABSOLUTO) como superficies planas drapeadas (APoligonoSuelo).
//
// DORMIDO: nadie llama a Cargar(). Pinta color plano (verde 58,122,53 y adoquín
// 150,145,135) a 5-7 cm sobre el terreno, y el terreno lleva desde dc8747c la
// ortofoto PNOA real: despertarlo taparía la foto aérea con 278 parches de color
// liso, además de sumar 278 draw calls sobre los ~819 de referencia. Si algún día
// se quiere el suelo poligonal, el camino es teñir la ortofoto por zona, no
// superponerle geometría opaca.
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
