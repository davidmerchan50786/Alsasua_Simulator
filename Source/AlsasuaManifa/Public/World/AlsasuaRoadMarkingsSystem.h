// AlsasuaRoadMarkingsSystem.h (capa MANIFA)
// Marcas viales: línea central, pasos de cebra y líneas de stop, en tres capas
// instanciadas sobre las cintas de calzada.
//
// La línea central se pintaba con el criterio `RoadWidth >= 6`, y mirando los
// anchos que hay en roads_unity.json eso son exactamente la autovía (11 m) y sus
// 50 enlaces (6 m): se pintaba **sólo en la A-10** y en ninguna calle del
// pueblo. Ahora va por tipo, en las de doble sentido —las 75 tertiary y la
// autovía—; un enlace es de sentido único y ahí tampoco.
//
// Y el paso de cebra no miraba el tipo: se pintaba en el arranque de cualquiera
// de las 489 vías, la autovía incluida.
#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaRoadMarkingsSystem.generated.h"

USTRUCT(BlueprintType)
struct FRoadMarking
{
    GENERATED_BODY()
    FString Tipo;
    FVector Posicion = FVector::ZeroVector;
    float Rotacion = 0.0f;
    float Ancho = 200.0f;
    float Largo = 400.0f;
    FString Calle;
    FString Barrio;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaRoadMarkingsSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|RoadMarkings")
    int32 GenerarMarcas();

    /** Tope de pasos de cebra, repartidos por las 269 calles candidatas. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|RoadMarkings")
    int32 MaxCrucesPeatonales = 40;

    /** Tope de tramos de línea central.
     *
     *  Las 108 vías de doble sentido dan 828 tramos de más de 3 m, y estaba en
     *  40: se pintaba el 5% y el resto se quedaba sin línea, además cortando por
     *  orden de fichero. Van todos: es una capa instanciada, o sea un draw call,
     *  y una carretera con la línea a trozos se ve peor que sin ella. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|RoadMarkings")
    int32 MaxLineasCentrales = 900;

    /** Tope de líneas de stop, repartidas por las 194 calles residenciales. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|RoadMarkings")
    int32 MaxLineasStop = 60;

    const TArray<FRoadMarking>& GetMarcas() const { return Marcas; }

private:
    TArray<FRoadMarking> Marcas;
};
