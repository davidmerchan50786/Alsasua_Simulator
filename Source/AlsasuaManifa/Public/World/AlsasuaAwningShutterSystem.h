#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
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
class ALSASUAMANIFA_API UAlsasuaAwningShutterSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
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
    TArray<FAwningEntry> Toldos;
    TArray<FShutterEntry> Persianas;
};
