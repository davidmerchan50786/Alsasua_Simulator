#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "Mass/AlsasuaMassProxies.h"
#include "AlsasuaMassParallelManager.generated.h"

/** Estructura para el procesamiento de datos en paralelo (Thread Safe) */
struct FMassUpdateBatch
{
    int32 StartIndex;
    int32 EndIndex;
    float DeltaTime;
    FVector PlayerLocation;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaMassParallelManager : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual bool IsAllowedToTick() const { return true; }
    virtual TStatId GetStatId() const { return TStatId(); }

    // Ejecuta la actualización de miles de proxies dividiéndolos entre los núcleos de la CPU
    void ExecuteParallelUpdate(float DeltaTime);

    UPROPERTY()
    TArray<FMassProtesterProxy> Proxies;

private:
};
