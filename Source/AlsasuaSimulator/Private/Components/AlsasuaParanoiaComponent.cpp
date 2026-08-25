#include "Components/AlsasuaParanoiaComponent.h"
#include "Effects/AlsasuaVisualGlitch.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

UAlsasuaParanoiaComponent::UAlsasuaParanoiaComponent() { PrimaryComponentTick.bCanEverTick = true; }

void UAlsasuaParanoiaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (CurrentParanoia > 0.0f) {
        CurrentParanoia = FMath::Max(0.0f, CurrentParanoia - (DecayRate * DeltaTime));
        OnParanoiaChanged.Broadcast(CurrentParanoia);
        ApplyVisualEffect();
    }
}

void UAlsasuaParanoiaComponent::AddStress(float Amount) {
    CurrentParanoia = FMath::Clamp(CurrentParanoia + Amount, 0.0f, 100.0f);
    OnParanoiaChanged.Broadcast(CurrentParanoia);
    ApplyVisualEffect();
}

void UAlsasuaParanoiaComponent::ApplyVisualEffect() {
    if (AActor* Owner = GetOwner()) {
        if (ACharacter* Char = Cast<ACharacter>(Owner)) {
            if (USkeletalMeshComponent* Mesh = Char->GetMesh()) {
                UAlsasuaVisualGlitch::UpdateNPCParanoiaEffect(Mesh, CurrentParanoia);
            }
        }
    }
}
