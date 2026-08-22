// AlsasuaStreetArtSystem.h (capa MANIFA)
// Murales y pintadas en los muros del pueblo. Puerto de Unity.
//
// Los 23 estaban colocados en el centroide del barrio —BarrioCenter() a pelo, sin
// desplazamiento— flotando a una altura sorteada y con un giro al azar. O sea:
// los dos murales de Herriko en el mismo punto exacto, uno dentro del otro, en
// mitad del barrio y sin muro detrás. Es la misma firma que las 17537 persianas
// apiladas en el centroide del edificio.
//
// Y encima no se veían de canto: la malla era /Engine/BasicShapes/Plane, que es
// un plano en XY mirando hacia arriba, escalado (ancho, 0.05, alto). Un plano
// horizontal no se pone vertical escalándole la Z: quedaban tumbados en el suelo
// como una tira de 5 cm.
//
// Ahora van sobre la fachada de un edificio real de buildings_final.json,
// recorriendo su perímetro para encontrar un lienzo lo bastante largo, apoyados
// en el suelo de su muro y mirando afuera. Sembrado por índice: el mismo mural
// cae en el mismo muro en cada arranque.
#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaStreetArtSystem.generated.h"

USTRUCT(BlueprintType)
struct FStreetArt
{
    GENERATED_BODY()
    FString Tipo;
    FString Mensaje;
    FVector Posicion = FVector::ZeroVector;
    float Rotacion = 0.0f;
    float Ancho = 300.0f;
    float Altura = 200.0f;
    FString Barrio;
    FString Color;
    /** Edificio cuyo muro lo sostiene. -1 si no se encontró ninguno. */
    int32 EdificioId = -1;
};

UCLASS()
class GF_WORLD_API UAlsasuaStreetArtSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|StreetArt")
    int32 ColocarArteCallejero();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|StreetArt")
    int32 MaxMurales = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|StreetArt")
    int32 MaxGrafitis = 15;

    const TArray<FStreetArt>& GetArte() const { return Arte; }

private:
    TArray<FStreetArt> Arte;

    /** Actor que aloja las dos capas instanciadas (mural y pintada). */
    UPROPERTY() TObjectPtr<AActor> Host = nullptr;
};
