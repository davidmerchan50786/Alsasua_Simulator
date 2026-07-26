#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaRoadSurfaceSystem.generated.h"

USTRUCT(BlueprintType)
struct FRoadSurfaceEntry
{
    GENERATED_BODY()
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
class ALSASUAMANIFA_API UAlsasuaRoadSurfaceSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Roads")
    bool CargarSuperficies();

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Roads")
    int32 AplicarSuperficiesEnMundo();

    const TArray<FRoadSurfaceEntry>& GetSuperficies() const { return Superficies; }

private:
    TArray<FRoadSurfaceEntry> Superficies;
    bool bCargado = false;
};
