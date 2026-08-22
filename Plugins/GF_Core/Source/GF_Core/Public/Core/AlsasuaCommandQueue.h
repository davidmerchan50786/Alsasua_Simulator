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

    // Los cuatro bajo UPROPERTY y con valor inicial. Ninguno lo tenía, y en un
    // USTRUCT eso significa que el GC no ve TargetActor: la cola es DIFERIDA
    // —quince comandos por frame como mucho—, así que el actor se queda esperando
    // turno en el buffer, que es exactamente la ventana en la que puede
    // recolectarse. Cuando le toca, el ReleaseActor(Cmd.TargetActor) del Tick
    // desreferencia un puntero muerto. Y sin inicializar, un comando construido
    // por defecto traía basura en el puntero y en el índice.
    UPROPERTY() EAlsasuaCommandType Type = EAlsasuaCommandType::AcquireActor;
    UPROPERTY() FVector Location = FVector::ZeroVector;
    UPROPERTY() TObjectPtr<AActor> TargetActor = nullptr;
    UPROPERTY() int32 Index = 0;
};

UCLASS()
class GF_CORE_API UAlsasuaCommandQueue : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual bool IsAllowedToTick() const override { return true; }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(AlsasuaCommandQueue, STATGROUP_Game); }
    virtual bool IsTickable() const override { return !IsTemplate(); }   // no ticar el CDO (CLAUDE.md §9)

    // Encola un comando para ser procesado cuando haya presupuesto
    void EnqueueCommand(FAlsasuaDeferredCommand NewCommand);

private:
    // También UPROPERTY: la cadena de reflexión tiene que estar entera. Con los
    // campos del struct marcados pero el TArray sin marcar, el GC sigue sin
    // llegar a TargetActor.
    UPROPERTY()
    TArray<FAlsasuaDeferredCommand> CommandBuffer;
    const int32 MaxCommandsPerFrame = 15;
};
