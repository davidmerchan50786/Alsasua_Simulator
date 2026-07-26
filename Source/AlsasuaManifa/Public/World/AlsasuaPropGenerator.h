#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AlsasuaPropGenerator.generated.h"

UCLASS()
class ALSASUAMANIFA_API UAlsasuaPropGenerator : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "AAA|Props")
    void AssignPropToActor(AActor* Actor);

private:
    TArray<UStaticMesh*> BannerMeshes;
    TArray<UMaterialInterface*> BannerMaterials;

    void LoadAssets();
};
