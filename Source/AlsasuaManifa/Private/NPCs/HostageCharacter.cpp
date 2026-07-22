#include "NPCs/HostageCharacter.h"
#include "AIController.h"

AHostageCharacter::AHostageCharacter() {
    PrimaryActorTick.bCanEverTick = true;
}

void AHostageCharacter::SetHostageState(EHostageState NewState) {
    CurrentState = NewState;
    if(CurrentState == EHostageState::Safe) {
        // Lógica de desaparición o agradecimiento
    }
}

void AHostageCharacter::FollowPlayer(APawn* PlayerPawn) {
    TargetPlayer = PlayerPawn;
    CurrentState = EHostageState::Following;
    if(AAIController* AIC = Cast<AAIController>(GetController())) {
        AIC->MoveToActor(TargetPlayer, 150.0f);
    }
}
