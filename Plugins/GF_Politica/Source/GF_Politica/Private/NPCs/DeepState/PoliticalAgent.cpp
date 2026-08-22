#include "NPCs/DeepState/PoliticalAgent.h"

APoliticalAgent::APoliticalAgent() { PrimaryActorTick.bCanEverTick = false; }

void APoliticalAgent::BeginPlay() {
    Super::BeginPlay();
}

void APoliticalAgent::PublishHeadline(const FText& Headline, const FText& Body) {
    OnHeadlinePublished.Broadcast(Headline, Body);
}
