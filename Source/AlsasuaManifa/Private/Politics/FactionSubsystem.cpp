#include "Politics/FactionSubsystem.h"
#include "Engine/World.h"

void UFactionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UFactionSubsystem::RegisterFaction(const FFactionData& Data)
{
    Factions.Add(Data.Id, Data);
}

void UFactionSubsystem::RecordPoliticalEvent(FName SubjectFaction, FName TargetFaction, float Impact)
{
    if (FFactionData* Sub = Factions.Find(SubjectFaction))
    {
        Sub->Influence = FMath::Clamp(Sub->Influence + Impact, 0.f, 100.f);
    }
    if (FFactionData* Target = Factions.Find(TargetFaction))
    {
        Target->Suspicion = FMath::Clamp(Target->Suspicion + FMath::Abs(Impact)*0.5f, 0.f, 100.f);
    }
}

void UFactionSubsystem::PublishEvidence(FName TargetFaction, float Strength)
{
    if (FFactionData* F = Factions.Find(TargetFaction))
    {
        F->Influence = FMath::Clamp(F->Influence - Strength, 0.f, 100.f);
    }
}

FFactionData UFactionSubsystem::GetFactionData(FName Id) const
{
    if (const FFactionData* Found = Factions.Find(Id))
    {
        return *Found;
    }
    return FFactionData();
}
