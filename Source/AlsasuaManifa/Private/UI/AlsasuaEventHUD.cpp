#include "UI/AlsasuaEventHUD.h"
#include "Systems/Social/SocialMediaSubsystem.h"
#include "AlsasuaCharacter.h"
#include "AlsasuaAttributeSet.h"
#include "Kismet/GameplayStatics.h"

AAlsasuaEventHUD::AAlsasuaEventHUD() {}

void AAlsasuaEventHUD::BeginPlay() {
    Super::BeginPlay();

    if (UWorld* World = GetWorld()) {
        if (UEventManagerSubsystem* EventSS = World->GetSubsystem<UEventManagerSubsystem>()) {
            EventSS->OnDirectorAction.AddDynamic(this, &AAlsasuaEventHUD::HandleWorldEvent);
        }
    }
}

void AAlsasuaEventHUD::GetSocialStatus(float& OutFollowers, float& OutViralImpact, float& OutPopularSupport) {
    OutFollowers = 0.f;
    OutViralImpact = 0.f;
    OutPopularSupport = 0.f;

    UWorld* W = GetWorld();
    if (!W) return;

    if (USocialMediaSubsystem* SocialSS = W->GetSubsystem<USocialMediaSubsystem>()) {
        OutFollowers = SocialSS->GlobalFollowers;
    }

    if (APawn* Player = UGameplayStatics::GetPlayerPawn(W, 0)) {
        if (AAlsasuaCharacter* Char = Cast<AAlsasuaCharacter>(Player)) {
            if (const UAlsasuaAttributeSet* Attr = Char->GetAttributeSet()) {
                OutPopularSupport = Attr->GetPopularSupport();
            }
        }
    }
}

void AAlsasuaEventHUD::HandleWorldEvent(FText EventDescription) {
    FWorldEventDataV2 EventData;
    EventData.EventID = FName(TEXT("DirectorAlert"));
    EventData.EventAnnounceMessage = EventDescription;
    OnNewGlobalEvent(EventData);
}
