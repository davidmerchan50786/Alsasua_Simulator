#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
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

USTRUCT()
struct FFacadeBuildingInfo
{
    GENERATED_BODY()
    FVector Centro = FVector::ZeroVector;
    float AlturaM = 6.0f;
    FFacadeBuildingInfo() = default;
    FFacadeBuildingInfo(FVector InCentro, float InAltura) : Centro(InCentro), AlturaM(InAltura) {}
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaFacadeGenerator : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Facade")
    bool CargarFachadas();

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Facade")
    bool CargarEdificios();

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Facade")
    int32 GenerarFachadasEnMundo();

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Facade")
    int32 ColocarLandmarksReales();

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Facade")
    int32 ColocarParadasTransporte();

    const TArray<FBuildingFacadeEntry>& GetFachadas() const { return Fachadas; }

private:
    TArray<FBuildingFacadeEntry> Fachadas;
    TMap<int32, FFacadeBuildingInfo> EdificiosCentros;
    bool bCargado = false;

    void CrearVentanaProcedural(AActor* Owner, const FWindowData& Ventana,
        const FVector& Pos, const FRotator& Rot, float Escala);
    void CrearBalconProcedural(AActor* Owner, const FBalconData& Balcon,
        const FVector& Pos, const FRotator& Rot);
    void CrearTiendaProcedural(AActor* Owner, const FTiendaData& Tienda,
        const FVector& Pos, const FRotator& Rot);
};
