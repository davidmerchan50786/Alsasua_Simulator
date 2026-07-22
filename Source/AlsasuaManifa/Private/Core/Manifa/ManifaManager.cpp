#include "Core/Manifa/ManifaManager.h"
#include "CrowdAgentComponent.h"
#include "Kismet/GameplayStatics.h"

void UManifaManager::TriggerManifestation(FVector CenterLocation) {
    TArray<AActor*> NPCs;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Civilian"), NPCs);

    ActiveProtesters = 0;
    for(AActor* Actor : NPCs) {
        if(UCrowdAgentComponent* Crowd = Actor->FindComponentByClass<UCrowdAgentComponent>()) {
            // Ponemos a los civiles en estado de protesta y los mandamos al centro
            Crowd->CurrentState = ECrowdState::FollowingProtest;
            ActiveProtesters++;
        }
    }
    OnManifaStateChanged.Broadcast(true);
}

void UManifaManager::UpdateManifestationStrength(float DeltaPopularSupport) {
    Momentum = FMath::Clamp(Momentum + DeltaPopularSupport, 0.f, 100.f);
}
