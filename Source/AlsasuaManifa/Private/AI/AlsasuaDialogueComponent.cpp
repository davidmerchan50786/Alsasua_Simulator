#include "AI/AlsasuaDialogueComponent.h"
#include "AI/AlsasuaCrowdAgentComponent.h"

UAlsasuaDialogueComponent::UAlsasuaDialogueComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UAlsasuaDialogueComponent::BeginPlay()
{
    Super::BeginPlay();

    // Diálogos de ejemplo
    DialogueByMood.FindOrAdd(0).Lines.Add("¡Altsasu, Altsasu!");
    DialogueByMood.FindOrAdd(0).Lines.Add("¡No pasarán!");

    DialogueByMood.FindOrAdd(1).Lines.Add("¡Esto va a explotar!");
    DialogueByMood.FindOrAdd(1).Lines.Add("¡Salgan ya!");
}

void UAlsasuaDialogueComponent::Speak()
{
    AActor* Owner = GetOwner();
    if (!Owner) return;

    UAlsasuaCrowdAgentComponent* MoodComp = Cast<UAlsasuaCrowdAgentComponent>(Owner->GetComponentByClass(UAlsasuaCrowdAgentComponent::StaticClass()));
    if (!MoodComp) return;

    FString Name = Owner->GetName();
    uint8 Mood = (uint8)MoodComp->GetCurrentMood();

    const FDialogueMoodPool* DialogPool = DialogueByMood.Find(Mood);
    if (DialogPool && DialogPool->Lines.Num() > 0)
    {
        FString ChosenLine = DialogPool->Lines[FMath::RandRange(0, DialogPool->Lines.Num()-1)];
        OnSpoke.Broadcast(Name, ChosenLine);
    }
}
