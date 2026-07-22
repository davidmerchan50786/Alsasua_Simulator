#include "Core/AlsasuaCommandQueue.h"
#include "Core/AlsasuaBudgetManager.h"
#include "Optimization/AlsasuaActorPoolSubsystem.h"

void UAlsasuaCommandQueue::Tick(float DeltaTime)
{
    if (CommandBuffer.Num() == 0) return;

    UAlsasuaBudgetManager* Budget = GetWorld()->GetSubsystem<UAlsasuaBudgetManager>();
    UAlsasuaActorPoolSubsystem* Pool = GetWorld()->GetSubsystem<UAlsasuaActorPoolSubsystem>();

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
                if (Pool) Pool->AcquireActor(Cmd.Location, FRotator::ZeroRotator);
                break;
            case EAlsasuaCommandType::ReleaseActor:
                // Cast seguro para el pool
                // if (Pool) Pool->ReleaseActor(Cast<AAlsasuaCharacter>(Cmd.TargetActor));
                break;
        }
        ProcessedCount++;
    }
}

void UAlsasuaCommandQueue::EnqueueCommand(FAlsasuaDeferredCommand NewCommand)
{
    CommandBuffer.Add(NewCommand);
}
