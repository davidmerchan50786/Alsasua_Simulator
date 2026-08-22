#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DebugManager.generated.h"

UCLASS()
class GF_DEBUG_API UDebugManager : public UWorldSubsystem {
    GENERATED_BODY()
public:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

    UFUNCTION(Exec)
    void DumpGameState();
};