#include "Gameplay/Narco/NarcoSyndicateComponent.h"
#include "Politics/FactionSubsystem.h"

UNarcoSyndicateComponent::UNarcoSyndicateComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UNarcoSyndicateComponent::SabotageNode(float Power)
{
    Value -= Power;
    Alertness += Power*0.5f;
    OnNodeSabotaged.Broadcast(NodeId);

    // Affect factions
    if (UWorld* W = GetWorld())
    {
        if (UFactionSubsystem* Sub = W->GetSubsystem<UFactionSubsystem>())
        {
            Sub->RecordPoliticalEvent(FName("LaAsamblea"), FName("ElCentro"), Power*0.2f);
            Sub->RecordPoliticalEvent(FName("ElCentro"), FName("ElGremio"), Power*0.5f);
        }
    }
}

void UNarcoSyndicateComponent::InterceptShipment(float Effectiveness)
{
    Value -= Effectiveness*10.f;
    Alertness += Effectiveness*2.f;

    if (UWorld* W = GetWorld())
    {
        if (UFactionSubsystem* Sub = W->GetSubsystem<UFactionSubsystem>())
        {
            Sub->RecordPoliticalEvent(FName("LaAsamblea"), FName("ElGremio"), Effectiveness*0.3f);
        }
    }
}
