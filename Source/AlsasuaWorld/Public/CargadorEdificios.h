// CargadorEdificios.h (capa WORLD)
// Lee Content/Datos/buildings_unity.json y puebla el mundo con edificios
// procedurales (AEdificioGenerado) apoyados en el Landscape. Reutiliza los
// *_unity.json de Unity vía UAlsasuaGeoData::UnityaUnreal. Puerto de
// GeneradorMundoOSM / ConstructorCiudadAssets. Soporta carga incremental
// (por presupuesto) para el director de arranque.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "CargadorEdificios.generated.h"

UCLASS()
class ALSASUAWORLD_API UCargadorEdificios : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Mundo") FString RutaRelativa = TEXT("Datos/buildings_final.json");
	UPROPERTY(EditAnywhere, Category="Mundo") bool bAutoCargar = true;

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	// Carga completa de una vez (devuelve nº de edificios creados).
	UFUNCTION(BlueprintCallable, Category="Mundo")
	int32 Cargar();

	UFUNCTION(BlueprintCallable, Category="Mundo")
	FString GetDebugSummary() const;

	// --- Carga incremental (la usa el director de arranque) ---
	void PrepararCarga();                       // parsea el JSON
	bool PasoPresupuesto(double PresupuestoMs); // construye hasta agotar ms; true si terminó
	bool Terminado() const { return bPreparado && Idx >= Items.Num(); }
	int32 Construidos = 0;

private:
	bool bHecho = false;
	bool bPreparado = false;
	int32 Idx = 0;
	TArray<TSharedPtr<FJsonValue>> Items;

	void ConstruirUno(const TSharedPtr<class FJsonObject>& O);
	float AlturaSuelo(const FVector2D& MundoXY) const;

	/** Fachada real del edificio (building_facades.json), o null. */
	const struct FBuildingFacadeEntry* FachadaDe(int32 IdEdificio) const;

	mutable const class UAlsasuaFacadeGenerator* GenFachadas = nullptr;
	mutable bool bFachadasBuscadas = false;
};
