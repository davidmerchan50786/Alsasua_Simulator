#include "NPCs/Underworld/NarcoDealerCharacter.h"
#include "Systems/Media/RadioSubsystem.h"
#include "Systems/Social/SocialMediaSubsystem.h"
#include "Systems/DeepState/DeepStateSubsystem.h"

ANarcoDealerCharacter::ANarcoDealerCharacter() { PrimaryActorTick.bCanEverTick = false; }

void ANarcoDealerCharacter::BeginPlay() { Super::BeginPlay(); }

void ANarcoDealerCharacter::NegotiateImmunity(bool bAccept) {
    if(bAccept) {
        // El jugador pacta con el "chivato": Inmunidad a cambio de datos
        bIsProtectedByDeepState = true;
        ImmunityLevel = 100.f;

        if (UWorld* W = GetWorld()) {
            if (URadioSubsystem* Radio = W->GetSubsystem<URadioSubsystem>()) {
                Radio->TriggerUrgentNews(
                    FText::FromString("POLÍTICA: El Ministerio insiste en la limpieza de las calles."),
                    FText::FromString("POLITIKA: Ministerioak kaleen garbiketan tematzen da."),
                    nullptr
                );
            }
        }
    }
}

void ANarcoDealerCharacter::ExposeDealer() {
    bIsProtectedByDeepState = false;
    ImmunityLevel = 0.f;

    // Impacto social: El pueblo ve la corrupción de las cloacas
    if (USocialMediaSubsystem* Social = GetWorld()->GetSubsystem<USocialMediaSubsystem>()) {
        Social->AddFollowers(2500);
    }

    if (URadioSubsystem* Radio = GetWorld()->GetSubsystem<URadioSubsystem>()) {
        Radio->TriggerUrgentNews(
            FText::FromString("¡ESCÁNDALO! Se filtra red de informantes protegidos por el Estado."),
            FText::FromString("ISLADA! Estatuak babestutako informatzaile sarea filtratu da."),
            nullptr
        );
    }
}
