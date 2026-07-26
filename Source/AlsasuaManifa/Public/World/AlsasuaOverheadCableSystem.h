#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaOverheadCableSystem.generated.h"

USTRUCT(BlueprintType)
struct FOverheadCable
{
    GENERATED_BODY()
    FVector Inicio = FVector::ZeroVector;
    FVector Fin = FVector::ZeroVector;
    float Caida = 30.0f;
    FString Tipo;
    FString Calle;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaOverheadCableSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Cables")
    int32 ColocarCables();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Cables")
    int32 MaxCables = 30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Cables")
    float AlturaCables = 500.0f;

    const TArray<FOverheadCable>& GetCables() const { return Cables; }

private:
    TArray<FOverheadCable> Cables;
};
