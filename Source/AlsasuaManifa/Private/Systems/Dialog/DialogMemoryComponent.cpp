#include "Systems/Dialog/DialogMemoryComponent.h"

UDialogMemoryComponent::UDialogMemoryComponent() { PrimaryComponentTick.bCanEverTick = false; }

void UDialogMemoryComponent::AddMemory(FGameplayTag Tag, float Value, float Duration) {
    FDialogMemoryEntry Entry;
    Entry.MemoryTag = Tag;
    Entry.Value = Value;
    Entry.ExpirationTime = (Duration > 0) ? GetWorld()->GetTimeSeconds() + Duration : -1.f;
    MemoryBank.Add(Tag, Entry);
}

bool UDialogMemoryComponent::HasMemory(FGameplayTag Tag) const {
    return MemoryBank.Contains(Tag);
}

float UDialogMemoryComponent::GetMemoryValue(FGameplayTag Tag) const {
    if(const FDialogMemoryEntry* Entry = MemoryBank.Find(Tag)) return Entry->Value;
    return 0.f;
}
