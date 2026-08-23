#include "AI/AlsasuaCrowdAgentComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UAlsasuaCrowdAgentComponent::UAlsasuaCrowdAgentComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

uint8 UAlsasuaCrowdAgentComponent::GetCurrentMood() const
{
    if (CurrentState == ECrowdAgentState::Panicking || Morale < 30.f)
    {
        return 1;
    }

    if (CurrentState == ECrowdAgentState::Resisting || Morale < 60.f)
    {
        return 2;
    }

    return 0;
}

void UAlsasuaCrowdAgentComponent::ReceiveExternalPanic(float Intensity)
{
    PanicLevel = FMath::Clamp(PanicLevel + Intensity, 0.f, 1.f);

    if (PanicLevel > 0.6f)
    {
        CurrentState = ECrowdAgentState::Panicking;
    }

    // Aplicar un "impulso" físico inmediato (empujón)
    PushForce = Intensity * 500.f;
}

void UAlsasuaCrowdAgentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Decaimiento natural del pánico y la fuerza de empuje (Fricción social)
    PanicLevel = FMath::FInterpTo(PanicLevel, 0.f, DeltaTime, 0.5f);
    PushForce = FMath::FInterpTo(PushForce, 0.f, DeltaTime, 2.0f);

    if (PanicLevel < 0.2f && CurrentState == ECrowdAgentState::Panicking)
    {
        CurrentState = ECrowdAgentState::Neutral;
    }

    // Si hay fuerza de empuje, mover al personaje físicamente
    if (PushForce > 5.0f)
    {
        ACharacter* Owner = Cast<ACharacter>(GetOwner());
        if (Owner && Owner->GetCharacterMovement())
        {
            FVector PushDir = -Owner->GetActorForwardVector(); // El empuje suele venir de delante en cargas
            Owner->LaunchCharacter(PushDir * PushForce * DeltaTime, true, false);
        }
    }
}

//~ IAlsasuaCrowdAgentInterface (contrato del Kernel)
void UAlsasuaCrowdAgentComponent::AdjustMorale_Implementation(float Delta)
{
    Morale += Delta;
}
