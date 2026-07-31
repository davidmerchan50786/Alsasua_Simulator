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

private:
    TArray<FFoliageTypeData> Tipos;
    bool bCargado = false;
    void InicializarTipos();
};
