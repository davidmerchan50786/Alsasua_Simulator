// AlsasuaCollisionSystem.h (capa MANIFA)
// Repasa la colisión de lo que se coloca por malla estática y no la trae puesta.
//
// **No toca edificios ni calles, y es correcto que no lo haga**: los ponen
// AEdificioGenerado y ACalleGenerada, que en su constructor hacen
// SetCollisionProfileName("BlockAll") y crean cada sección de ProceduralMesh con
// bCreateCollision = true. La colisión del pueblo ya está en su sitio desde que
// se construye la geometría.
//
// Lo que había aquí no arreglaba nada de eso, y tampoco fallaba: recorría
// GetAllActorsOfClass(AStaticMeshActor) filtrando por GetName() para encontrar
// "Edificio" y "Calle_". Los 1030 edificios son AEdificioGenerado y las calles
// ACalleGenerada, los dos AActor, así que la lista no traía ninguno — y aunque
// los hubiera traído, su malla es un ProceduralMeshComponent y el
// FindComponentByClass<UStaticMeshComponent> tampoco lo habría encontrado. Dos
// bucles que no daban una vuelta, con su línea de log diciendo cuántas
// colisiones habían generado.
//
// Se queda con el repaso de lo que sí es AStaticMeshActor y puede llegar sin
// colisión, que es lo único que aquí tiene sentido hacer.
#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "Arranque/AlsasuaPilarArranque.h"
#include "AlsasuaCollisionSystem.generated.h"

UCLASS()
class GF_WORLD_API UAlsasuaCollisionSystem : public UGameInstanceSubsystem, public IAlsasuaPilarArranque
{
    GENERATED_BODY()
	public:
	virtual int32 EjecutarArranque() override { return RepasarColisionesDeProps(); }
	virtual FString EtiquetaArranque() const override { return TEXT("props sin colision repasados"); }
	virtual int32 OrdenArranque() const override { return 310; }

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    /**
     * Pone colisión a los AStaticMeshActor que hayan llegado sin ella.
     *
     * Edificios y calles no entran: ya vienen con BlockAll de su constructor.
     * Devuelve cuántos actores se han repasado.
     */
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Collision")
    int32 RepasarColisionesDeProps();
};
