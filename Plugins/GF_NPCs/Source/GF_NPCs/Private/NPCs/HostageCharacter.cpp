#include "NPCs/HostageCharacter.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"

AHostageCharacter::AHostageCharacter() {
    PrimaryActorTick.bCanEverTick = true;
}

void AHostageCharacter::BeginPlay() {
    Super::BeginPlay();
    SGracias = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SC_HostageGracias.SC_HostageGracias"));
}

void AHostageCharacter::SetHostageState(EHostageState NewState) {
    CurrentState = NewState;
    if(CurrentState == EHostageState::Safe) {
        if (SGracias)
            UGameplayStatics::PlaySoundAtLocation(GetWorld(), SGracias, GetActorLocation());

        // Liberar control de IA (el rehén queda libre).
        if (AAIController* AIC = Cast<AAIController>(GetController()))
            AIC->UnPossess();

        // Desaparecer tras un breve delay.
        FTimerHandle TimerHandle;
        GetWorldTimerManager().SetTimer(TimerHandle, [this]()
        {
            if (this) SetActorHiddenInGame(true);
        }, 5.0f, false);
    }
}

void AHostageCharacter::FollowPlayer(APawn* PlayerPawn) {
    TargetPlayer = PlayerPawn;
    CurrentState = EHostageState::Following;
    if(AAIController* AIC = Cast<AAIController>(GetController())) {
        AIC->MoveToActor(TargetPlayer, 150.0f);
    }
}
