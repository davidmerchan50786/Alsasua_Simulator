#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "AlsasuaMassProxiesManager.generated.h"

/**
 * Subsistema simplificado para gestión de proxies de multitud.
 * Trabaja junto con UAlsasuaMassParallelManager para el renderizado.
 */
UCLASS()
class GF_OPTIMIZATION_API UAlsasuaMassProxies : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override { return true; }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UAlsasuaMassProxies, STATGROUP_Game); }

    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override
    {
        return WorldType == EWorldType::Editor || WorldType == EWorldType::PIE || WorldType == EWorldType::Game;
    }

    /** Callback de cambio de nivel de wanted para afectar proxies. */
    void OnWantedLevelChanged(int32 NewLevel);
};
