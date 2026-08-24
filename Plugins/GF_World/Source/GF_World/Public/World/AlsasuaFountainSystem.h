#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "Arranque/AlsasuaPilarArranque.h"
#include "AlsasuaFountainSystem.generated.h"

USTRUCT(BlueprintType)
struct FRealFountain
{
    GENERATED_BODY()
    FString Nombre;
    FVector Posicion = FVector::ZeroVector;
    FString Barrio;
    FString Calle;
    float Radio = 150.0f;
    float AlturaCazoleta = 80.0f;
    bool bFuncional = true;
};

UCLASS()
class GF_WORLD_API UAlsasuaFountainSystem : public UGameInstanceSubsystem, public IAlsasuaPilarArranque
{
    GENERATED_BODY()
	public:
	virtual int32 EjecutarArranque() override { return ColocarFuentes(); }
	virtual FString EtiquetaArranque() const override { return TEXT("fuentes reales"); }
	virtual int32 OrdenArranque() const override { return 380; }

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Fountains")
    int32 ColocarFuentes();

    const TArray<FRealFountain>& GetFuentes() const { return Fuentes; }

private:
    TArray<FRealFountain> Fuentes;
};
