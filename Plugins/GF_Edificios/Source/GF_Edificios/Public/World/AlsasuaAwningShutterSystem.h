// AlsasuaAwningShutterSystem.h (capa MANIFA)
// Toldos y persianas sobre las fachadas, a partir de building_facades.json.
//
// Son 17537 ventanas con persiana y del orden de 700 toldos. Estaba escrito con
// un AStaticMeshActor por pieza —dieciocho mil actores— y, peor, apilándolas en
// el CENTROIDE del edificio, una cada 3 m de altura y sin tope: el edificio
// 297389260 mide 7,7 m y tiene 132 ventanas con persiana, así que le salía una
// columna de 398 m atravesando el tejado. Las que no sobresalían quedaban dentro
// del edificio, invisibles y pagándose igual.
//
// Ahora van repartidas por el perímetro y por planta, a ras de fachada y mirando
// afuera, y lo que no cabe en el edificio no se coloca (se dice cuánto). Todo a
// HierarchicalInstancedStaticMesh, una capa para toldos y otra para persianas.
#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "AlsasuaAwningShutterSystem.generated.h"

USTRUCT(BlueprintType)
struct FAwningEntry
{
    GENERATED_BODY()
    int32 BuildingId = 0;
    FVector Posicion = FVector::ZeroVector;
    float Rotacion = 0.0f;
    float Ancho = 200.0f;
    float Profundidad = 100.0f;
    FString ColorToldo;
    FString Barrio;
    bool bPlegado = false;
};

USTRUCT(BlueprintType)
struct FShutterEntry
{
    GENERATED_BODY()
    int32 BuildingId = 0;
    FVector Posicion = FVector::ZeroVector;
    float Rotacion = 0.0f;
    FString Color;
    bool bAbierto = false;
};

UCLASS()
class GF_EDIFICIOS_API UAlsasuaAwningShutterSystem : public UGameInstanceSubsystem, public IAlsasuaPilarArranque
{
    GENERATED_BODY()
	public:
	virtual int32 EjecutarArranque() override { return ColocarToldosYPersianas(); }
	virtual FString EtiquetaArranque() const override { return TEXT("toldos + persianas"); }
	virtual int32 OrdenArranque() const override { return 470; }

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Awnings")
    int32 ColocarToldosYPersianas();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Awnings")
    float ProbabilidadToldo = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Awnings")
    float ProbabilidadPersiana = 0.6f;

    const TArray<FAwningEntry>& GetToldos() const { return Toldos; }
    const TArray<FShutterEntry>& GetPersianas() const { return Persianas; }

private:
    /** Actor que aloja las dos capas instanciadas. */
    UPROPERTY() TObjectPtr<AActor> Host = nullptr;

    TArray<FAwningEntry> Toldos;
    TArray<FShutterEntry> Persianas;
};
