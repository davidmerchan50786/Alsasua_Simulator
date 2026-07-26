#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaFarolaPlacer.generated.h"

USTRUCT(BlueprintType)
struct FFarolaEntry
{
    GENERATED_BODY()
    FString Calle;
    float X = 0.0f;
    float Z = 0.0f;
    float Rotacion = 0.0f;
    FString TipoFarola;
    float AlturaM = 3.5f;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaFarolaPlacer : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Farolas")
    bool CargarFarolas();

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Farolas")
    int32 ColocarFarolasEnMundo();

    const TArray<FFarolaEntry>& GetFarolas() const { return Farolas; }

private:
    TArray<FFarolaEntry> Farolas;
    bool bCargado = false;
};
