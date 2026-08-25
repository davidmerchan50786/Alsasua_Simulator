#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaShopFrontSystem.generated.h"

USTRUCT(BlueprintType)
struct FShopFront
{
    GENERATED_BODY()
    FString Nombre;
    FString Tipo;
    FString Barrio;
    FString Calle;
    float X = 0.0f;
    float Z = 0.0f;
    float Rotacion = 0.0f;
    float AnchoM = 4.0f;
    float AlturaM = 3.0f;
    FString ColorFachada;
    bool bConToldo = false;
    FString ColorToldo;
    bool bConRotulo = true;
    FString Horario;
};

UCLASS()
class GF_EDIFICIOS_API UAlsasuaShopFrontSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Shops")
    bool CargarTiendas();

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Shops")
    int32 ColocarTiendasEnMundo();

    const TArray<FShopFront>& GetTiendas() const { return Tiendas; }

private:
    TArray<FShopFront> Tiendas;
    bool bCargado = false;
};
