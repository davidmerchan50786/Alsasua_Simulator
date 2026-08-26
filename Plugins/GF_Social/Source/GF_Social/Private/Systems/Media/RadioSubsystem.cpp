#include "Systems/Media/RadioSubsystem.h"
#include "WantedSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

void URadioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    if (UWorld* W = GetWorld())
    {
        if (UGameInstance* GI = W->GetGameInstance())
        {
            if (UWantedSubsystem* Wanted = GI->GetSubsystem<UWantedSubsystem>())
                Wanted->OnEstrellasCambia.AddDynamic(this, &URadioSubsystem::OnWantedLevelChange);
        }
    }
}

void URadioSubsystem::OnWantedLevelChange(int32 Nivel)
{
    if (Nivel < 2) return;
    FText Headline = FText::Format(
        NSLOCTEXT("Radio", "Wanted", "Atencion: operacion policial en curso nivel {0}"),
        FText::AsNumber(Nivel));
    TriggerUrgentNews(Headline, FText::GetEmpty());
}

void URadioSubsystem::TriggerUrgentNews(FText Headline, FText Body, USoundBase* Voice) {
    FRadioNewsClip News;
    News.NewsHeadline = Headline;
    News.NewsBody = Body;
    News.VoiceClip = Voice;

    // Play the voice clip if provided.
    if (Voice && GetWorld())
    {
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (PC)
        {
            UGameplayStatics::PlaySound2D(PC, Voice, 0.8f);
        }
    }

    OnRadioUpdate.Broadcast(News);
}

void URadioSubsystem::AddClandestineMessage(FText Sender, FText Message) {
    FRadioNewsClip Msg;
    Msg.NewsHeadline = Sender;
    Msg.NewsBody = Message;

    OnTelegramUpdate.Broadcast(Msg);
}
