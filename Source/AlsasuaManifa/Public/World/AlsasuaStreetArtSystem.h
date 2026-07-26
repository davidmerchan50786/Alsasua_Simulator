#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaStreetArtSystem.generated.h"

USTRUCT(BlueprintType)
struct FStreetArt
{
    GENERATED_BODY()
    FString Tipo;
    FString Mensaje;
    FVector Posicion = FVector::ZeroVector;
    float Rotacion = 0.0f;
    float Ancho = 300.0f;
    float Altura = 200.0f;
    FString Barrio;
    FString Color;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaStreetArtSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|StreetArt")
    int32 ColocarArteCallejero();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|StreetArt")
    int32 MaxMurales = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|StreetArt")
    int32 MaxGrafitis = 15;

    const TArray<FStreetArt>& GetArte() const { return Arte; }

private:
    TArray<FStreetArt> Arte;
};
