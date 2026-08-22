#include "Politics/ReputationComponent.h"
#include "Politics/FactionSubsystem.h"

void UReputationComponent::ModifyReputation(FName FactionId, float Delta)
{
    float& Val = Reputation.FindOrAdd(FactionId);
    Val = FMath::Clamp(Val + Delta, -100.f, 100.f);
    OnReputationChanged.Broadcast(FactionId, Val);

    // Notify global subsystem
    if (UWorld* W = GetWorld())
    {
        if (UFactionSubsystem* Sub = W->GetSubsystem<UFactionSubsystem>())
        {
            FFactionData Data = Sub->GetFactionData(FactionId);
            Data.Influence = FMath::Clamp(Data.Influence + Delta*0.1f, 0.f, 100.f);
            Sub->RegisterFaction(Data);
        }
    }
}
