#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "AlsasuaSignPlacer.generated.h"

USTRUCT(BlueprintType)
struct FSignEntry
{
    GENERATED_BODY()
    FString Tipo;
    FString Texto;
    FString Barrio;
    FString Idioma;
    float X = 0.0f;
    float Z = 0.0f;
    float AnchoM = 1.0f;
    float AltoM = 0.4f;
    float AlturaM = 2.5f;
    FString Material;
    bool bBilingue = false;
    bool bConLuz = false;
    FString TipoNegocio;
};

UCLASS()
class GF_CARRETERAS_API UAlsasuaSignPlacer : public UGameInstanceSubsystem, public IAlsasuaPilarArranque
{
    GENERATED_BODY()
	public:
	virtual int32 EjecutarArranque() override { return ColocarSenalesEnMundo(); }
	virtual FString EtiquetaArranque() const override { return TEXT("senales reales"); }
	virtual int32 OrdenArranque() const override { return 220; }

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Signs")
    bool CargarSenales();

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Signs")
    int32 ColocarSenalesEnMundo();

    const TArray<FSignEntry>& GetSenales() const { return Senales; }

private:
    TArray<FSignEntry> Senales;
    bool bCargado = false;
};
