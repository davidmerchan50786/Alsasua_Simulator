#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaRoadMarkingsSystem.generated.h"

USTRUCT(BlueprintType)
struct FRoadMarking
{
    GENERATED_BODY()
    FString Tipo;
    FVector Posicion = FVector::ZeroVector;
    float Rotacion = 0.0f;
    float Ancho = 200.0f;
    float Largo = 400.0f;
    FString Calle;
    FString Barrio;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaRoadMarkingsSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|RoadMarkings")
    int32 GenerarMarcas();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|RoadMarkings")
    int32 MaxCrucesPeatonales = 30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|RoadMarkings")
    int32 MaxLineasCentrales = 40;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|RoadMarkings")
    int32 MaxLineasStop = 20;

    const TArray<FRoadMarking>& GetMarcas() const { return Marcas; }

private:
    TArray<FRoadMarking> Marcas;
};
