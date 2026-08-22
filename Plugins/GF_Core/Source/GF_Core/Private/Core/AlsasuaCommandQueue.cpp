#include "Core/AlsasuaCommandQueue.h"
#include "Core/AlsasuaBudgetManager.h"
#include "Optimization/AlsasuaActorPoolSubsystem.h"
#include "Engine/StaticMeshActor.h"

void UAlsasuaCommandQueue::Tick(float DeltaTime)
{
    if (CommandBuffer.Num() == 0) return;

    UWorld* W = GetWorld();
    if (!W) return;

    UAlsasuaBudgetManager* Budget = W->GetSubsystem<UAlsasuaBudgetManager>();
    UAlsasuaActorPoolSubsystem* Pool = W->GetSubsystem<UAlsasuaActorPoolSubsystem>();

    int32 ProcessedCount = 0;
    while (CommandBuffer.Num() > 0 && ProcessedCount < MaxCommandsPerFrame)
    {
        // Si nos quedamos sin presupuesto para simulación, paramos este frame
        if (Budget && !Budget->CanExecute(EBudgetCategory::Simulation)) break;

        FAlsasuaDeferredCommand Cmd = CommandBuffer[0];
        CommandBuffer.RemoveAt(0);

        switch (Cmd.Type)
        {
            case EAlsasuaCommandType::AcquireActor:
                if (Pool) Pool->AcquireActor(AStaticMeshActor::StaticClass(), Cmd.Location, FRotator::ZeroRotator);
                break;
            case EAlsasuaCommandType::ReleaseActor:
                if (Pool && Cmd.TargetActor) Pool->ReleaseActor(Cmd.TargetActor);
                break;
        }
        ProcessedCount++;
    }
}

void UAlsasuaCommandQueue::EnqueueCommand(FAlsasuaDeferredCommand NewCommand)
{
    CommandBuffer.Add(NewCommand);
}
