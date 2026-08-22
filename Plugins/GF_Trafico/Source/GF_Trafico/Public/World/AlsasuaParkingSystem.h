// AlsasuaParkingSystem.h (capa MANIFA)
// Plazas de aparcamiento en línea junto al bordillo y puertas de garaje.
//
// Estaba escrito a golpe de FMath::RandRange: 80 plazas y 20 garajes en puntos
// sorteados de cualquiera de las 489 vías de roads_unity.json. Eso significaba
// tres cosas, todas malas:
//
//  - El pueblo cambiaba en cada arranque, así que no se podía comparar dos
//    capturas ni razonar sobre lo que se veía.
//  - "Cualquiera de las 489" incluye la A-10, la N-1 y sus 89 enlaces, los 56
//    caminos y las 31 calles peatonales: batería de plazas pintadas sobre la
//    autovía. Ahora sólo se aparca donde se aparca, en las 194 calles
//    residenciales y las 44 de servicio.
//  - Los garajes eran un cubo de hormigón de 5x6x3 m soltado a metro y medio
//    del eje de la vía, sin mirar qué había: bloques macizos dentro de
//    edificios y en mitad de la calzada. Un garaje es una puerta en la planta
//    baja de una fachada, no una caseta exenta, así que ahora es eso: un panel
//    en la fachada que da a la calle, la misma que elige AlsasuaDirecciones
//    para la puerta de entrada.
//
// Y eran 100 AStaticMeshActor con el LoadObject de la malla y el material
// dentro del bucle. Ahora dos capas instanciadas (regla 0).
#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaParkingSystem.generated.h"

USTRUCT(BlueprintType)
struct FParkingSpot
{
    GENERATED_BODY()
    FVector Posicion = FVector::ZeroVector;
    float Rotacion = 0.0f;
    FString Tipo; // "calle", "garaje"
    bool bOcupado = false;
    FString Barrio;
};

UCLASS()
class GF_TRAFICO_API UAlsasuaParkingSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Parking")
    int32 GenerarPlazasAparcamiento();

    /** Tope de plazas de calle. Es un tope, no una cuota: si las calles dan
     *  para menos, salen menos, en vez de repetir puntos hasta llegar.
     *
     *  Las 194 calles residenciales dan 5490 plazas si se llenan enteras las dos
     *  aceras. 1800 es una de cada tres, que para un pueblo de 7500 habitantes
     *  se acerca a lo que hay, y son 1800 trazos de suelo al arrancar: el mismo
     *  orden que los 2783 árboles del LiDAR. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Parking")
    int32 MaxPlazasCalle = 1800;

    /** Tope de puertas de garaje en fachada. Quien decide cuántas salen es el
     *  8% sorteado por id entre los edificios de vivienda con 12 m o más de
     *  fachada (~65); el tope está por encima para que no recorte por orden de
     *  fichero, que no es orden geográfico. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Parking")
    int32 MaxGarajes = 90;

    /** Largo de una plaza en línea, con su hueco de maniobra. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Parking")
    float PasoPlazaCm = 550.0f;

    /** Separación del bordillo al eje de la plaza, sobre el semiancho de la vía. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Parking")
    float RetranqueoCm = 125.0f;

    const TArray<FParkingSpot>& GetPlazas() const { return Plazas; }

private:
    TArray<FParkingSpot> Plazas;

    /** Actor que aloja las dos capas instanciadas. */
    UPROPERTY() TObjectPtr<AActor> Host = nullptr;

    int32 PintarPlazasDeCalle(class UHierarchicalInstancedStaticMeshComponent* Capa);
    int32 ColocarPuertasGaraje(class UHierarchicalInstancedStaticMeshComponent* Capa);
};
