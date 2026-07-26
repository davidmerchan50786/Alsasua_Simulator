#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AlsasuaLODManager.generated.h"

UCLASS()
class ALSASUAMANIFA_API UAlsasuaLODManager : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
    float LOD0Distance = 5000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
    float LOD1Distance = 15000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
    float LOD2Distance = 30000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
    float TreeLOD0Distance = 3000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
    float TreeLOD1Distance = 8000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
    float TreeHideDistance = 20000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
    int32 MaxVisibleTrees = 2000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
    int32 MaxVisibleBuildings = 500;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|LOD")
    void ConfigureLODsForQuality(int32 QualityLevel);

private:
    TArray<AActor*> CachedTrees;
    TArray<AActor*> CachedBuildings;
    bool bCached = false;
    float TimeSinceCache = 0.0f;

    void CacheActors();
    void UpdateTreeLODs(const FVector& CameraLocation);
    void UpdateBuildingLODs(const FVector& CameraLocation);
};
