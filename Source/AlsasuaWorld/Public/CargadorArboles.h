// CargadorArboles.h (capa WORLD)
// Lee Content/Datos/trees_unity.json (2783 árboles LIDAR, marco Unity absoluto)
// y los instancia con Hierarchical Instanced Static Mesh, una malla por especie.
// Puerto de AlsasuaTreeStreamer (siembra). Soporta carga incremental.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "CargadorArboles.generated.h"

class UHierarchicalInstancedStaticMeshComponent;

UCLASS()
class ALSASUAWORLD_API UCargadorArboles : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Mundo") FString RutaRelativa = TEXT("Datos/trees_unity.json");
	UPROPERTY(EditAnywhere, Category="Mundo") bool bAutoCargar = true;
	UPROPERTY(EditAnywhere, Category="Mundo") float AlturaReferenciaMalla = 10.f;

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	UFUNCTION(BlueprintCallable, Category="Mundo")
	int32 Cargar();

	// --- Carga incremental (director de arranque) ---
	void PrepararCarga();
	bool PasoPresupuesto(double PresupuestoMs);
	bool Terminado() const { return bPreparado && Idx >= Items.Num(); }
	int32 Sembrados = 0;

private:
	bool bHecho = false;
	bool bPreparado = false;
	int32 Idx = 0;
	TArray<TSharedPtr<FJsonValue>> Items;

	UPROPERTY() AActor* Host = nullptr;
	UPROPERTY() TMap<FString, UHierarchicalInstancedStaticMeshComponent*> PorEspecie;
	UPROPERTY() UStaticMesh* MallaDefecto = nullptr;

	void SembrarUno(const TSharedPtr<class FJsonObject>& O);
	UHierarchicalInstancedStaticMeshComponent* ComponenteDe(const FString& Especie);
	float AlturaSuelo(const FVector2D& MundoXY) const;
};
