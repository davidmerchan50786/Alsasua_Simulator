#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaSidewalkSystem.generated.h"

USTRUCT(BlueprintType)
struct FSidewalkSegment
{
    GENERATED_BODY()
    FVector Inicio = FVector::ZeroVector;
    FVector Fin = FVector::ZeroVector;
    float Ancho = 200.0f;
    FString Calle;
    FString Barrio;
    float Altura = 0.0f;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaSidewalkSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Sidewalks")
    int32 GenerarAceras();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Sidewalks")
    float AnchoAceras = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Sidewalks")
    float AlturaBordillo = 15.0f;

    const TArray<FSidewalkSegment>& GetAcera() const { return Acera; }

private:
    TArray<FSidewalkSegment> Acera;
};
