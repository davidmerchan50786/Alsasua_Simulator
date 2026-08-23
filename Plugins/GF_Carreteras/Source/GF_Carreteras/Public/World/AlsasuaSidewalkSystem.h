// AlsasuaSidewalkSystem.h (capa MANIFA)
// Acera de bordillo a los dos lados de cada calzada de roads_unity.json.
//
// Son ~6360 losas: 3184 segmentos de vía por dos lados. Estaba escrito con un
// AStaticMeshActor por losa, o sea del orden de 6400 draw calls sobre los ~819
// de referencia del RESUMEN_TECNICO — y encima con dos LoadObject de material
// por losa, dentro del bucle. Es el ejemplar más caro de la regla 0: nada de un
// actor por pieza cuando las piezas se cuentan por miles.
//
// Ahora van a HierarchicalInstancedStaticMesh, una capa por acabado (piedra en
// Herriko y Harrobieta, hormigón en el resto): las 6360 losas en dos draw calls,
// con culling y LOD de serie. La losa es un plano escalado por instancia, que es
// exactamente lo que un ISM sabe hacer.
//
// No pisa a UCargadorVias: aquello construye las 206 aceras que OSM tiene
// trazadas como footway, que son las de verdad. Esto es el bordillo sintético a
// ambos lados de la calzada, que OSM no traza.
#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "AlsasuaSidewalkSystem.generated.h"

USTRUCT(BlueprintType)
struct FSidewalkSegment
{
    GENERATED_BODY()
    FVector Inicio = FVector::ZeroVector;
    FVector Fin = FVector::ZeroVector;
    float Ancho = 200.0f;
    FString Calle;
    FString Barrio;
    float Altura = 0.0f;
};

UCLASS()
class GF_CARRETERAS_API UAlsasuaSidewalkSystem : public UGameInstanceSubsystem, public IAlsasuaPilarArranque
{
    GENERATED_BODY()
	public:
	virtual int32 EjecutarArranque() override { return GenerarAceras(); }
	virtual FString EtiquetaArranque() const override { return TEXT("segmentos de acera"); }
	virtual int32 OrdenArranque() const override { return 370; }

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Sidewalks")
    int32 GenerarAceras();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Sidewalks")
    float AnchoAceras = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Sidewalks")
    float AlturaBordillo = 15.0f;

    const TArray<FSidewalkSegment>& GetAcera() const { return Acera; }

private:
    TArray<FSidewalkSegment> Acera;

    /** Actor que aloja las capas instanciadas. Uno para todo el pueblo. */
    UPROPERTY() TObjectPtr<AActor> Host = nullptr;
};
