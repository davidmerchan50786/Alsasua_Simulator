#include "Systems/Media/RadioSubsystem.h"

void URadioSubsystem::TriggerUrgentNews(FText Headline, FText Body, USoundBase* Voice) {
    FRadioNewsClip News;
    News.NewsHeadline = Headline;
    News.NewsBody = Body;
    News.VoiceClip = Voice;

    OnRadioUpdate.Broadcast(News);
}

void URadioSubsystem::AddClandestineMessage(FText Sender, FText Message) {
    FRadioNewsClip Msg;
    Msg.NewsHeadline = Sender;
    Msg.NewsBody = Message;

    OnTelegramUpdate.Broadcast(Msg);
}
