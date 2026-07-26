#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaPaintedStreetSignSystem.generated.h"

USTRUCT(BlueprintType)
struct FPaintedSignEntry
{
    GENERATED_BODY()
    FString NombreES;
    FString NombreEU;
    FVector Posicion = FVector::ZeroVector;
    float Rotacion = 0.0f;
    float Ancho = 300.0f;
    float Altura = 80.0f;
    FString Barrio;
    FString Color;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaPaintedStreetSignSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|PaintedSigns")
    int32 ColocarRotulosPintados();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|PaintedSigns")
    int32 MaxRotulos = 40;

    const TArray<FPaintedSignEntry>& GetRotulos() const { return Rotulos; }

private:
    TArray<FPaintedSignEntry> Rotulos;
};
