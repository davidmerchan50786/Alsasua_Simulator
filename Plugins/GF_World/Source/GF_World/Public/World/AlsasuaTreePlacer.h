// AlsasuaTreePlacer.h (capa MANIFA)
// Ficha botánica de las nueve especies del arbolado de Alsasua: nombre
// científico, euskera y castellano, altura media, radio de copa, color de
// follaje y malla asociada.
//
// Quien planta es UCargadorArboles (fase 2), del mismo trees_unity.json, con la
// especie que trae el dato, escala por altura real y un HISM por especie.
// ColocarArbolesReales() sigue aquí y funciona, pero ADirectorArranque no lo
// llama: plantaría los 2783 árboles otra vez, uno por actor, encima de los que
// ya están. Úsalo sólo si vas a plantar en un mundo donde la fase 2 no ha
// corrido.
#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaTreePlacer.generated.h"

USTRUCT(BlueprintType)
struct FTreeSpecies
{
    GENERATED_BODY()
    FString NombreCientifico;
    FString NombreEu;
    FString NombreEs;
    float AlturaMedia = 15.0f;
    float RadioCopa = 5.0f;
    FString ColorFollaje;
    FString AssetPath;
};

USTRUCT(BlueprintType)
struct FTreePlacement
{
    GENERATED_BODY()
    FVector PosicionUnreal = FVector::ZeroVector;
    FString Especie;
    float Escala = 1.0f;
    float Rotacion = 0.0f;
};

UCLASS()
class GF_WORLD_API UAlsasuaTreePlacer : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Trees")
    bool CargarArboles();

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Trees")
    int32 ColocarArbolesReales();

    const TArray<FTreePlacement>& GetArboles() const { return Arboles; }

private:
    TArray<FTreeSpecies> Especies;
    TArray<FTreePlacement> Arboles;
    bool bCargado = false;

    void InicializarEspecies();
    FString AsignarEspecie(float AlturaLIDAR) const;
};
