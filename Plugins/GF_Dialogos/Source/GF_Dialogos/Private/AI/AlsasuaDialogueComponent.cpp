#include "AI/AlsasuaDialogueComponent.h"
#include "AI/AlsasuaCrowdAgentComponent.h"

UAlsasuaDialogueComponent::UAlsasuaDialogueComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UAlsasuaDialogueComponent::BeginPlay()
{
    Super::BeginPlay();

    DialogueByMood.FindOrAdd(0).Lines = {
        TEXT("¡Altsasu, Altsasu!"),
        TEXT("¡No pasarán!"),
        TEXT("¡El pueblo unido jamás será vencido!"),
        TEXT("¡Justice pour ALSASUA!")
    };
    DialogueByMood.FindOrAdd(1).Lines = {
        TEXT("¡Esto va a explotar!"),
        TEXT("¡Salgan ya!"),
        TEXT("¡Ya basta de represión!"),
        TEXT("¡Tenemos que movernos!")
    };
    DialogueByMood.FindOrAdd(2).Lines = {
        TEXT("¡Vamos a torcerles el brazo!"),
        TEXT("¡Que se enteren de una vez!"),
        TEXT("¡Esto no para hasta conseguirlo!"),
        TEXT("¡Nadie nos calla!")
    };
    DialogueByMood.FindOrAdd(3).Lines = {
        TEXT("¡Corred! ¡Viene la policía!"),
        TEXT("¡Auxilio!"),
        TEXT("¡Me están pillando!"),
        TEXT("¡Salid por la calle de atrás!")
    };
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
