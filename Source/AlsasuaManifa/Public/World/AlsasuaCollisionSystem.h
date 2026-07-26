#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaCollisionSystem.generated.h"

UCLASS()
class ALSASUAMANIFA_API UAlsasuaCollisionSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Collision")
    int32 GenerarColisionesEdificios();

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Collision")
    int32 GenerarColisionesCalles();
};
