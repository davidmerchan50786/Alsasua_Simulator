#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaContainerSystem.generated.h"

USTRUCT(BlueprintType)
struct FContainer
{
    GENERATED_BODY()
    FString Tipo;
    FVector Posicion = FVector::ZeroVector;
    float Rotacion = 0.0f;
    FString Barrio;
    FString Calle;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaContainerSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Containers")
    int32 ColocarContenedores();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Containers")
    int32 MaxContenedores = 40;

    const TArray<FContainer>& GetContenedores() const { return Contenedores; }

private:
    TArray<FContainer> Contenedores;
    int32 ColocarContenedoresFallback();
};
