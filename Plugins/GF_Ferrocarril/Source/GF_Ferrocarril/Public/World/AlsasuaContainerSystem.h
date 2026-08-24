// AlsasuaContainerSystem.h (capa MANIFA)
// Papeleras y contenedores de reciclaje de street_furniture.json.
//
// Cuatro cosas iban mal a la vez, y ninguna daba un error:
//
//  - Los cien se convertían con RelLocalToUE5, pero street_furniture.json mezcla
//    marcos y las 3 papelera_reciclaje están en el absoluto: acababan a 8,6 km
//    del pueblo. Es la trampa que documenta CLAUDE.md §4; MobiliarioAUE5 lo
//    decide por pieza.
//  - El tipo se asignaba con "Placed % 5", así que a una papelera de calle le
//    tocaba ser el contenedor del vidrio, y el tipo real —organico, envases,
//    papel— que el dato trae en las de reciclaje no lo leía nadie.
//  - La tabla de colores por tipo se calculaba y no se aplicaba a nada: el
//    material que se ponía era DefaultMaterial. Los cien salían iguales.
//  - La malla era /Game/CitySample/..., que no está en el repo ni se baja con
//    él, y no había fallback. Cien actores sin malla: invisibles. Ahora pasa por
//    AlsasuaMallaFab, que cae a la forma básica del motor.
//
// Y eran cien AStaticMeshActor. Ahora una capa instanciada por tipo (regla 0).
#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "Arranque/AlsasuaPilarArranque.h"
#include "AlsasuaContainerSystem.generated.h"

USTRUCT(BlueprintType)
struct FContainer
{
    GENERATED_BODY()
    FString Tipo;
    FVector Posicion = FVector::ZeroVector;
    float Rotacion = 0.0f;
    FString Barrio;
    FString Calle;
};

UCLASS()
class GF_FERROCARRIL_API UAlsasuaContainerSystem : public UGameInstanceSubsystem, public IAlsasuaPilarArranque
{
    GENERATED_BODY()
	public:
	virtual int32 EjecutarArranque() override { return ColocarContenedores(); }
	virtual FString EtiquetaArranque() const override { return TEXT("contenedores de residuos"); }
	virtual int32 OrdenArranque() const override { return 440; }

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Containers")
    int32 ColocarContenedores();

    /** Sólo para el fallback procedural, cuando no hay street_furniture.json.
     *  Con el fichero salen las 100 piezas que trae el dato. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Containers")
    int32 MaxContenedores = 40;

    const TArray<FContainer>& GetContenedores() const { return Contenedores; }

private:
    TArray<FContainer> Contenedores;
    int32 ColocarContenedoresFallback();

    /** Actor que aloja las capas instanciadas, una por tipo de contenedor. */
    UPROPERTY() TObjectPtr<AActor> Host = nullptr;

    /** Capas ya creadas, por tipo. Se crean al necesitarlas: si el pueblo no
     *  tiene contenedor de vidrio, no hay componente de vidrio. */
    UPROPERTY() TMap<FString, TObjectPtr<class UHierarchicalInstancedStaticMeshComponent>> Capas;

    bool PrepararHost();
    class UHierarchicalInstancedStaticMeshComponent* CapaDe(const FString& Tipo);
};
