#include "Core/AlsasuaEventManager.h"
#include "AI/AlsasuaCrowdSentiment.h"
#include "Audio/AlsasuaAudioManager.h"

void UAlsasuaEventManager::Tick(float DeltaTime)
{
    MissionTimer += DeltaTime;
    ProcessTimeline();
}

void UAlsasuaEventManager::AddMilestone(FName ID, float Time)
{
    FNarrativeMilestone NewMilestone;
    NewMilestone.EventIdentifier = ID;
    NewMilestone.TimeStamp = Time;
    Timeline.Add(NewMilestone);
}

void UAlsasuaEventManager::ProcessTimeline()
{
    for (auto& Milestone : Timeline)
    {
        if (!Milestone.bHasTriggered && MissionTimer >= Milestone.TimeStamp)
        {
            Milestone.bHasTriggered = true;
            OnEventTriggered.Broadcast(Milestone.EventIdentifier);

            UE_LOG(LogTemp, Warning, TEXT("NARRATIVE EVENT: %s disparado al segundo %.2f"), *Milestone.EventIdentifier.ToString(), MissionTimer);

            // Efectos sistémicos inmediatos (AAA+++)
            if (Milestone.EventIdentifier == "POLICE_CHARGE")
            {
                UWorld* W = GetWorld();
                if (!W) continue;

                if (UAlsasuaCrowdSentiment* Sentiment = W->GetSubsystem<UAlsasuaCrowdSentiment>())
                {
                    Sentiment->TriggerSocialEvent(FVector(0,0,0), 0.9f, 5000.f); // Pánico masivo
                }
            }
        }
    }
}
