#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaDetailDressingSystem.generated.h"

USTRUCT(BlueprintType)
struct FDetailItem
{
    GENERATED_BODY()
    FString Tipo;
    FVector Posicion = FVector::ZeroVector;
    float Rotacion = 0.0f;
    float Escala = 1.0f;
    FString Barrio;
    FString Calle;
    FLinearColor Color = FLinearColor::White;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaDetailDressingSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Detail")
    int32 ColocarDetalle();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Detail")
    int32 MaxMacetas = 40;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Detail")
    int32 MaxBuzones = 15;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Detail")
    int32 MaxPapeleiras = 30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Detail")
    int32 MaxBancos = 25;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Detail")
    int32 MaxVallasVerdes = 20;

    const TArray<FDetailItem>& GetDetalles() const { return Detalles; }
    bool UsaDatosReales() const { return bUsandoDatosReales; }

private:
    bool bUsandoDatosReales = false;
    TArray<FDetailItem> Detalles;
    // Actores que coloca este sistema y que sobreviven al arranque: sin
    // UPROPERTY el GC no los ve a través de esta lista (CLAUDE.md §9).
    UPROPERTY()
    TArray<TObjectPtr<AActor>> MueblesReales;

    void CargarMueblesReales(UWorld* World);
    void ColocarMacetas(UWorld* World);
    void ColocarBuzones(UWorld* World);
    void ColocarPapeleiras(UWorld* World);
    void ColocarBancos(UWorld* World);
    void ColocarVallasVerdes(UWorld* World);
    AActor* CrearActor(UWorld* World, const FVector& Pos, float Rot, float Scale,
        const TCHAR* MeshPath, const TCHAR* MatPath, const FString& Label);
};
