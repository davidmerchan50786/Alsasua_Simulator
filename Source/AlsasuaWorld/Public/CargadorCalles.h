// CargadorCalles.h (capa WORLD)
// Lee Content/Datos/roads_unity.json (489 vías, relativas a plaza) y construye
// las calles drapeadas sobre el terreno. Carga incremental para el director.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "CargadorCalles.generated.h"

// Eje de calzada para el tráfico: polilínea en mundo XY (cm) + ancho.
struct FEjeVial
{
	TArray<FVector2D> Puntos;
	float AnchoCm = 400.f;
};

UCLASS()
class ALSASUAWORLD_API UCargadorCalles : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Mundo") FString RutaRelativa = TEXT("Datos/roads_unity.json");
	UPROPERTY(EditAnywhere, Category="Mundo") bool bAutoCargar = true;

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	UFUNCTION(BlueprintCallable, Category="Mundo")
	int32 Cargar();

	void PrepararCarga();
	bool PasoPresupuesto(double PresupuestoMs);
	bool Terminado() const { return bPreparado && Idx >= Items.Num(); }
	int32 Construidas = 0;

	// Ejes viarios (polilínea + ancho) para el tráfico. Se rellena al construir.
	TArray<FEjeVial> EjesViarios;

private:
	bool bHecho = false;
	bool bPreparado = false;
	int32 Idx = 0;
	TArray<TSharedPtr<FJsonValue>> Items;
	void ConstruirUna(const TSharedPtr<class FJsonObject>& O);
};
