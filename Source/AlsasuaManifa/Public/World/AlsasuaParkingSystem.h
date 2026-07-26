#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaParkingSystem.generated.h"

USTRUCT(BlueprintType)
struct FParkingSpot
{
    GENERATED_BODY()
    FVector Posicion = FVector::ZeroVector;
    float Rotacion = 0.0f;
    FString Tipo; // "calle", "parking", "garaje"
    bool bOcupado = false;
    FString Barrio;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaParkingSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Parking")
    int32 GenerarPlazasAparcamiento();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Parking")
    int32 MaxPlazasCalle = 80;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Parking")
    int32 MaxGarajes = 20;

    const TArray<FParkingSpot>& GetPlazas() const { return Plazas; }

private:
    TArray<FParkingSpot> Plazas;
    FVector ObtenerPuntoEnCalle(const FString& Barrio);
};
