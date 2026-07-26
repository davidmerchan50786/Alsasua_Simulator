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
class ALSASUAMANIFA_API UAlsasuaTreePlacer : public UGameInstanceSubsystem
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
