#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaGuardrailSystem.generated.h"

USTRUCT(BlueprintType)
struct FGuardrail
{
    GENERATED_BODY()
    FVector Inicio = FVector::ZeroVector;
    FVector Fin = FVector::ZeroVector;
    FString Tipo;
    FString Barrio;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaGuardrailSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Guardrails")
    int32 ColocarBarandillas();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Guardrails")
    float AlturaBarandilla = 110.0f;

    const TArray<FGuardrail>& GetBarandillas() const { return Barandillas; }

private:
    TArray<FGuardrail> Barandillas;
};
