#include "Systems/Media/RadioSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

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
