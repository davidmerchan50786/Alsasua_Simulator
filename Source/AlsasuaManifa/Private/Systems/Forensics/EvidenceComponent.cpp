#include "Systems/Forensics/EvidenceComponent.h"
#include "Kismet/GameplayStatics.h"

UEvidenceComponent::UEvidenceComponent() { PrimaryComponentTick.bCanEverTick = false; }

void UEvidenceComponent::BeginPlay() {
    Super::BeginPlay();
    SpawnTime = GetWorld()->GetTimeSeconds();
    GetWorld()->GetTimerManager().SetTimer(TimerHandle_Decay, this, &UEvidenceComponent::UpdateDecay, 10.f, true);
}

void UEvidenceComponent::UpdateDecay() {
    float Age = GetWorld()->GetTimeSeconds() - SpawnTime;
    if(ForensicState == EForensicState::Pristine && Age > TimeToDecay) {
        ForensicState = EForensicState::Contaminated;
    }
}

void UEvidenceComponent::CollectEvidence(AActor* Collector) {
    if(ForensicState == EForensicState::Collected) return;
    ForensicState = EForensicState::Collected;
    OnEvidenceCollected.Broadcast(GetOwner(), EvidenceTag);
}

void UEvidenceComponent::ContaminateEvidence(float Amount) {
    // Amount could reduce quality; for now, instantly set to contaminated
    ForensicState = EForensicState::Contaminated;
}
