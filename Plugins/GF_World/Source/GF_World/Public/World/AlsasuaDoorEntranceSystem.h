// AlsasuaDoorEntranceSystem.h (capa MANIFA)
// Puerta de entrada de cada edificio de buildings_final.json, en la fachada que
// da a su calle, con el número de portal de OSM cuando lo hay.
//
// Iba a AStaticMeshActor por puerta —1030— más otro por toldo de entrada, y con
// el LoadObject del material dentro del bucle. Ahora son dos capas
// HierarchicalInstancedStaticMesh: regla 0, nada de un actor por pieza cuando
// las piezas se cuentan por miles.
//
// El número de portal es la excepción: un UTextRenderComponent no se instancia,
// así que sigue siendo un componente por puerta rotulada, pero colgado del
// actor anfitrión en vez de uno propio. Lleva CullDistance porque un portal no
// se lee a 80 m.
#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "Arranque/AlsasuaPilarArranque.h"
#include "AlsasuaDoorEntranceSystem.generated.h"

USTRUCT(BlueprintType)
struct FDoorEntry
{
    GENERATED_BODY()
    int32 BuildingId = 0;
    FVector Posicion = FVector::ZeroVector;
    float Rotacion = 0.0f;
    FString Tipo;
    FString Color;
    FString Barrio;
    /** Dirección de OSM (Datos/direcciones_osm.json); vacías si no tiene. */
    FString Calle;
    FString Portal;
};

UCLASS()
class GF_WORLD_API UAlsasuaDoorEntranceSystem : public UGameInstanceSubsystem, public IAlsasuaPilarArranque
{
    GENERATED_BODY()
	public:
	virtual int32 EjecutarArranque() override { return ColocarPuertas(); }
	virtual FString EtiquetaArranque() const override { return TEXT("puertas y entradas"); }
	virtual int32 OrdenArranque() const override { return 490; }

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Doors")
    int32 ColocarPuertas();

    const TArray<FDoorEntry>& GetPuertas() const { return Puertas; }

private:
    TArray<FDoorEntry> Puertas;

    /** Actor que aloja las capas instanciadas y los rótulos de portal. */
    UPROPERTY() TObjectPtr<AActor> Host = nullptr;
};
