#include "Core/Manifa/ManifaManager.h"
#include "AI/AlsasuaCrowdAgentComponent.h"
#include "Kismet/GameplayStatics.h"

void UManifaManager::TriggerManifestation(FVector CenterLocation) {
    TArray<AActor*> NPCs;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Civilian"), NPCs);

    ActiveProtesters = 0;
    for(AActor* Actor : NPCs) {
        if(UAlsasuaCrowdAgentComponent* Crowd = Actor->FindComponentByClass<UAlsasuaCrowdAgentComponent>()) {
            Crowd->CurrentState = ECrowdAgentState::Following;
            ActiveProtesters++;
        }
    }
    OnManifaStateChanged.Broadcast(true);
}

void UManifaManager::UpdateManifestationStrength(float DeltaPopularSupport) {
    Momentum = FMath::Clamp(Momentum + DeltaPopularSupport, 0.f, 100.f);
}
