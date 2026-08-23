#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "AlsasuaFacadeGenerator.generated.h"

USTRUCT(BlueprintType)
struct FWindowData
{
    GENERATED_BODY()
    FString Tipo;
    float Ancho = 1.0f;
    float Alto = 1.5f;
    FString MaterialMarcos;
    FString ColorMarcos;
    bool bConPersiana = true;
    bool bConBalcon = false;
};

USTRUCT(BlueprintType)
struct FBalconData
{
    GENERATED_BODY()
    FString Tipo;
    float Ancho = 2.0f;
    float Profundidad = 0.8f;
    FString Barandilla;
};

USTRUCT(BlueprintType)
struct FTiendaData
{
    GENERATED_BODY()
    FString Nombre;
    FString Tipo;
    float AnchoM = 4.0f;
    float AlturaM = 3.0f;
    FString MaterialFachada;
    bool bConToldo = false;
    FString ColorToldo;
};

USTRUCT(BlueprintType)
struct FBuildingFacadeEntry
{
    GENERATED_BODY()
    int32 BuildingId = 0;
    FString Barrio;
    FString MaterialFachada;
    TArray<float> ColorFachada;
    FString Estilo;
    int32 NumNiveles = 2;
    float AlturaTotal = 6.0f;
    float AlturaPorNivel = 3.0f;
    float PerimetroAprox = 20.0f;
    float AreaAprox = 50.0f;
    TArray<FWindowData> Ventanas;
    TArray<FBalconData> Balcones;
    TArray<FTiendaData> TiendasPlantaBaja;
    FString MaterialTejado;
    TArray<float> ColorTejado;
};


UCLASS()
class GF_EDIFICIOS_API UAlsasuaFacadeGenerator : public UGameInstanceSubsystem, public IAlsasuaDatosFachadas, public IAlsasuaPilarArranque
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Facade")
    bool CargarFachadas();

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Facade")
    int32 ColocarLandmarksReales();

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Facade")
    int32 ColocarParadasTransporte();

    const TArray<FBuildingFacadeEntry>& GetFachadas() const { return Fachadas; }

    /**
     * Fachada real de un edificio, o null.
     *
     * Es de donde CargadorEdificios saca las ventanas: antes este subsistema
     * generaba su propia geometría de ventanas a 1 cm del centro de cada
     * edificio (con HalfSide calculado y sin usar), o sea dentro del edificio y
     * sin que se viera nada, mientras los muros de verdad se hacían con medidas
     * inventadas a partir del tamaño.
     */
    const FBuildingFacadeEntry* De(int32 BuildingId) const;

    //~ IAlsasuaDatosFachadas
    virtual bool MedidasDe(int32 IdEdificio, float& AlturaPorNivelM,
        float& AnchoVentanaM, float& AltoVentanaM) const override;

private:
    TArray<FBuildingFacadeEntry> Fachadas;
    bool bCargado = false;

};
