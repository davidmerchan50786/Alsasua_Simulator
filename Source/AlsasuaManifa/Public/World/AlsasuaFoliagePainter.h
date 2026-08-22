// AlsasuaFoliagePainter.h (capa MANIFA)
// Siembra las mallas reales del pack Naturaleza (hierba, setos, rocas, maleza,
// hiedra) dentro de los 273 polígonos de zona verde de greenspaces_unity.json.
//
// Es el pintor primario de las zonas verdes. El antiguo AlsasuaVegetationSpawner
// (quads procedurales cosidos en una sola sección de ProceduralMesh) queda sólo
// como respaldo mientras este pack no esté importado: DirectorArranque le pasa
// la mano a aquél sólo si aquí no hay ni una malla.
//
// Va todo a HierarchicalInstancedStaticMesh, un componente por tipo. Una
// instancia no es un actor: doce mil matas cuestan del orden de ocho draw calls
// y traen culling y LOD de serie. Con AStaticMeshActor por pieza —que es como
// estaba escrito— la misma siembra habrían sido decenas de miles de actores, y
// el RESUMEN_TECNICO ya cuenta qué pasa en este proyecto cuando se reparten draw
// calls a ese ritmo.
#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaFoliagePainter.generated.h"

USTRUCT(BlueprintType)
struct FFoliageTypeData
{
    GENERATED_BODY()
    FString Nombre;
    FString AssetPath;
    float EscalaMin = 0.8f;
    float EscalaMax = 1.2f;
    float Densidad = 1.0f;
    FString Tipo;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaFoliagePainter : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Foliage")
    bool CargarTipos();

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Foliage")
    int32 PintarFoliageEnZonasVerdes();

    const TArray<FFoliageTypeData>& GetTipos() const { return Tipos; }

    /** Tope de instancias en todo el pueblo. Cada una lleva un trazo vertical
     *  para apoyarse, y ése es el coste que hay que acotar, no el de dibujarlas. */
    UPROPERTY(EditAnywhere, Category = "Alsasua|Foliage")
    int32 MaxInstancias = 12000;

    /** Instancias por cada 100 m² de zona verde, antes de repartir por tipo. */
    UPROPERTY(EditAnywhere, Category = "Alsasua|Foliage")
    float DensidadPor100m2 = 6.f;

private:
    TArray<FFoliageTypeData> Tipos;
    bool bCargado = false;
    void InicializarTipos();
};
