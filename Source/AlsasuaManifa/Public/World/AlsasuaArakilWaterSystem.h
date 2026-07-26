#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaArakilWaterSystem.generated.h"

USTRUCT(BlueprintType)
struct FWaterSegment
{
    GENERATED_BODY()
    FVector Centro = FVector::ZeroVector;
    float Ancho = 15.0f;
    float Largo = 50.0f;
    float Profundidad = 2.0f;
    float VelocidadFlujo = 0.5f;
    FLinearColor ColorSuperficie = FLinearColor(0.02f, 0.12f, 0.18f, 0.92f);
    FLinearColor ColorProfundo = FLinearColor(0.01f, 0.04f, 0.08f, 0.95f);
    FLinearColor ColorEspuma = FLinearColor(0.8f, 0.85f, 0.9f, 0.6f);
    float Turbidez = 0.3f;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaArakilWaterSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Water")
    bool CargarTramosRio();

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Water")
    int32 GenerarMallaAgua();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Water|Visual")
    float WaterSpeed = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Water|Visual")
    float WaveAmplitude = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Water|Visual")
    float WaveFrequency = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Water|Visual")
    FLinearColor RiverColor = FLinearColor(0.03f, 0.15f, 0.22f, 0.9f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Water|Visual")
    float FoamIntensity = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Water|Physics")
    float CurrentStrength = 200.0f;

    const TArray<FWaterSegment>& GetTramos() const { return Tramos; }

private:
    TArray<FWaterSegment> Tramos;
    bool bCargado = false;
};
