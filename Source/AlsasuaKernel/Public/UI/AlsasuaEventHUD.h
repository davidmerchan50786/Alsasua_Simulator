#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "WorldEventTypes.h"
#include "AlsasuaEventHUD.generated.h"

UCLASS()
class ALSASUAKERNEL_API AAlsasuaEventHUD : public AHUD {
    GENERATED_BODY()

public:
    AAlsasuaEventHUD();

    UFUNCTION(BlueprintCallable, Category="AAA|HUD")
    void BroadcastWorldEvent(FText EventDescription);

    UFUNCTION(BlueprintCallable, Category="AAA|HUD")
    void GetSocialStatus(float& OutFollowers, float& OutViralImpact, float& OutPopularSupport);

    UFUNCTION(BlueprintImplementableEvent, Category="AAA|HUD")
    void OnNewGlobalEvent(const FWorldEventDataV2& EventData);

    UFUNCTION(BlueprintImplementableEvent, Category="AAA|HUD")
    void OnEvidenceUploaded(float Impact);

    float Followers = 0.f;
    float PopularSupport = 0.f;
};
