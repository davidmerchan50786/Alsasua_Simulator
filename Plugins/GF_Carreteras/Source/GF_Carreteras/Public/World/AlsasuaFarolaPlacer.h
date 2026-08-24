#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "Arranque/AlsasuaPilarArranque.h"
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
class GF_CARRETERAS_API UAlsasuaFarolaPlacer : public UGameInstanceSubsystem, public IAlsasuaPilarArranque
{
    GENERATED_BODY()
	public:
	virtual int32 EjecutarArranque() override { return ColocarFarolasEnMundo(); }
	virtual FString EtiquetaArranque() const override { return TEXT("farolas reales"); }
	virtual int32 OrdenArranque() const override { return 240; }

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
