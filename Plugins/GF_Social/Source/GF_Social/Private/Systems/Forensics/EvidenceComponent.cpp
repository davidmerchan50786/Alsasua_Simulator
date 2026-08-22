#include "Systems/Forensics/EvidenceComponent.h"
#include "Kismet/GameplayStatics.h"

UEvidenceComponent::UEvidenceComponent() { PrimaryComponentTick.bCanEverTick = false; }

void UEvidenceComponent::BeginPlay() {
    Super::BeginPlay();
    UWorld* W = GetWorld();
    if (!W) return;
    SpawnTime = W->GetTimeSeconds();
    W->GetTimerManager().SetTimer(TimerHandle_Decay, this, &UEvidenceComponent::UpdateDecay, 10.f, true);
}

void UEvidenceComponent::UpdateDecay() {
    UWorld* W = GetWorld();
    if (!W) return;
    float Age = W->GetTimeSeconds() - SpawnTime;
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
    if(ForensicState == EForensicState::Collected) return;

    ContaminationLevel = FMath::Clamp(ContaminationLevel + Amount, 0.f, 100.f);

    if(ContaminationLevel >= ContaminationThreshold) {
        ForensicState = EForensicState::Contaminated;
    }
}
