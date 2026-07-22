#include "Systems/Disguise/DisguiseComponent.h"

UDisguiseComponent::UDisguiseComponent() { PrimaryComponentTick.bCanEverTick = true; }

void UDisguiseComponent::EquipDisguise(EDisguiseType Type, bool bConsumable, float InitialDurability) {
    CurrentDisguise = Type;
    bIsConsumable = bConsumable;
    Durability = InitialDurability;

    // Aquí se podrían activar efectos visuales (Mesh/Material) en el Character
}

void UDisguiseComponent::UseDisguise(float Amount) {
    if(CurrentDisguise == EDisguiseType::None) return;

    Durability -= Amount;
    if(Durability <= 0.f) {
        EDisguiseType BrokenType = CurrentDisguise;
        CurrentDisguise = EDisguiseType::None;
        OnDisguiseBroken.Broadcast(BrokenType);
    }
}

float UDisguiseComponent::GetDetectionMultiplier() const {
    if(CurrentDisguise == EDisguiseType::Momotxorro) return 0.3f; // 70% menos detección
    if(CurrentDisguise == EDisguiseType::None) return 1.0f;
    return 0.7f;
}

void UDisguiseComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Degradar ligeramente si corremos en presencia de policía (simulación)
    // Esto se conectaría con los sensores de IA en el sistema final
}
