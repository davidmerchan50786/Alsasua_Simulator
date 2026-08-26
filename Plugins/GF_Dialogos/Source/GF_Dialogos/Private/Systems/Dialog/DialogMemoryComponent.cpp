#include "Systems/Dialog/DialogMemoryComponent.h"

UDialogMemoryComponent::UDialogMemoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 1.f;
}

void UDialogMemoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	for (auto It = MemoryBank.CreateIterator(); It; ++It)
	{
		if (It.Value().ExpirationTime > 0.f && Now >= It.Value().ExpirationTime)
			It.RemoveCurrent();
	}
}

void UDialogMemoryComponent::AddMemory(FGameplayTag Tag, float Value, float Duration) {
    FDialogMemoryEntry Entry;
    Entry.MemoryTag = Tag;
    Entry.Value = Value;
    UWorld* W = GetWorld();
    Entry.ExpirationTime = (Duration > 0 && W) ? W->GetTimeSeconds() + Duration : -1.f;
    MemoryBank.Add(Tag, Entry);
}

bool UDialogMemoryComponent::HasMemory(FGameplayTag Tag) const {
    if (const FDialogMemoryEntry* Entry = MemoryBank.Find(Tag))
    {
        if (Entry->ExpirationTime > 0.f && GetWorld() && GetWorld()->GetTimeSeconds() >= Entry->ExpirationTime)
            return false;
        return true;
    }
    return false;
}

float UDialogMemoryComponent::GetMemoryValue(FGameplayTag Tag) const {
    if (const FDialogMemoryEntry* Entry = MemoryBank.Find(Tag))
    {
        if (Entry->ExpirationTime > 0.f && GetWorld() && GetWorld()->GetTimeSeconds() >= Entry->ExpirationTime)
            return 0.f;
        return Entry->Value;
    }
    return 0.f;
}
