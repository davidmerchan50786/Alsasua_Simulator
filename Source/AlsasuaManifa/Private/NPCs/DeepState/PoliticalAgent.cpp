#include "NPCs/DeepState/PoliticalAgent.h"
#include "Systems/Media/RadioSubsystem.h"

APoliticalAgent::APoliticalAgent() { PrimaryActorTick.bCanEverTick = false; }

void APoliticalAgent::BeginPlay() {
    Super::BeginPlay();
}

void APoliticalAgent::PublishHeadline(const FText& Headline, const FText& Body) {
    if (UWorld* W = GetWorld()) {
        if (URadioSubsystem* Radio = W->GetSubsystem<URadioSubsystem>()) {
            Radio->TriggerUrgentNews(Headline, Body, nullptr);
        }
    }
}
