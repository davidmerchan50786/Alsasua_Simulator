#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "AlsasuaCommandQueue.generated.h"

UENUM()
enum class EAlsasuaCommandType : uint8
{
    AcquireActor,
    ReleaseActor,
    UpdateHISM,
    PreloadAsset
};

USTRUCT()
struct FAlsasuaDeferredCommand
{
    GENERATED_BODY()

    EAlsasuaCommandType Type;
    FVector Location;
    AActor* TargetActor;
    int32 Index;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaCommandQueue : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual bool IsAllowedToTick() const { return true; }
    virtual TStatId GetStatId() const { return TStatId(); }

    // Encola un comando para ser procesado cuando haya presupuesto
    void EnqueueCommand(FAlsasuaDeferredCommand NewCommand);

private:
    TArray<FAlsasuaDeferredCommand> CommandBuffer;
    const int32 MaxCommandsPerFrame = 15;
};
