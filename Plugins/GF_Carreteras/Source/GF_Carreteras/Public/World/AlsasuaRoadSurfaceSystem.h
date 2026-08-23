// AlsasuaRoadSurfaceSystem.h (capa MANIFA)
// Clasifica el firme de cada vía —asfalto, adoquín, asfalto gastado, grava—
// según su tipo, su ancho y el barrio por el que pasa.
//
// Clasifica, no construye. Las cintas de calzada son de UCargadorCalles (fase 4),
// que ya las tiene drapeadas sobre el terreno como ACalleGenerada, una sección
// de ProceduralMesh por calle. Este sistema publica el firme por id de vía y es
// ADirectorArranque quien se lo aplica a esas cintas, porque la capa MANIFA no
// puede ver a WORLD.
//
// Antes lo hacía por su cuenta: un AStaticMeshActor por calle, un cubo aplastado
// de ancho×ancho×0,1 colocado en el primer punto del trazado. Ni seguía la calle
// ni era una superficie: era una mancha cuadrada en el arranque de cada una, 489
// de ellas, 489 draw calls sobre los ~819 de referencia, encima de la cinta que
// ya estaba puesta.
#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "AlsasuaRoadSurfaceSystem.generated.h"

USTRUCT(BlueprintType)
struct FRoadSurfaceEntry
{
    GENERATED_BODY()
    /** id de roads_unity.json: es la llave contra ACalleGenerada::Id. */
    int32 Id = -1;
    FString Nombre;
    FString Calle;
    float X = 0.0f;
    float Z = 0.0f;
    float Ancho = 8.0f;
    FString Material;
    FString Barrio;
    FString Tipo;
};

UCLASS()
class GF_CARRETERAS_API UAlsasuaRoadSurfaceSystem : public UGameInstanceSubsystem, public IAlsasuaPilarArranque
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Roads")
    bool CargarSuperficies();

    /** Firme y color de una vía por su id, para que los aplique quien tenga la
     *  cinta. Devuelve false si esa vía no está clasificada. */
    bool FirmeDe(int32 Id, FString& OutMaterial, FLinearColor& OutColor) const;

    const TArray<FRoadSurfaceEntry>& GetSuperficies() const { return Superficies; }

private:
    TArray<FRoadSurfaceEntry> Superficies;
    /** id de vía -> índice en Superficies, para no barrer 489 por consulta. */
    TMap<int32, int32> PorId;
    bool bCargado = false;
};
