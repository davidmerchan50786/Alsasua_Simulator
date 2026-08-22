#include "UI/AlsasuaEventHUD.h"

AAlsasuaEventHUD::AAlsasuaEventHUD() {}

void AAlsasuaEventHUD::BroadcastWorldEvent(FText EventDescription) {
    FWorldEventDataV2 EventData;
    EventData.EventID = FName(TEXT("DirectorAlert"));
    EventData.EventAnnounceMessage = EventDescription;
    OnNewGlobalEvent(EventData);
}

void AAlsasuaEventHUD::GetSocialStatus(float& OutFollowers, float& OutViralImpact, float& OutPopularSupport) {
    OutFollowers = Followers;
    OutViralImpact = 0.f;
    OutPopularSupport = PopularSupport;
}
