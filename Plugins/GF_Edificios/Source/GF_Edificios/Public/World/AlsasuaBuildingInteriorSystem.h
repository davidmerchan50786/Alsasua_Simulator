#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaBuildingInteriorSystem.generated.h"

USTRUCT(BlueprintType)
struct FBuildingInterior
{
    GENERATED_BODY()
    int32 BuildingId = 0;
    FString Barrio;
    FString Uso;
    int32 Plantas = 3;
    bool bTieneLuz = true;
    float IntensidadLuz = 1.0f;
    FLinearColor ColorLuz = FLinearColor(1.0f, 0.95f, 0.85f);
};

UCLASS()
class GF_EDIFICIOS_API UAlsasuaBuildingInteriorSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Interiors")
    int32 GenerarInteriores();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Interiors")
    float ProbabilidadLuz = 0.4f;

    const TArray<FBuildingInterior>& GetInteriores() const { return Interiores; }

private:
    TArray<FBuildingInterior> Interiores;
};
