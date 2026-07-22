#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Systems/Events/EventManagerSubsystem.h"
#include "AlsasuaEventHUD.generated.h"

USTRUCT(BlueprintType)
struct FWorldEventDataV2 {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName EventID;

    UPROPERTY(BlueprintReadOnly)
    FText EventAnnounceMessage;
};

UCLASS()
class ALSASUAMANIFA_API AAlsasuaEventHUD : public AHUD {
    GENERATED_BODY()

public:
    AAlsasuaEventHUD();

    virtual void BeginPlay() override;

    // Actualiza los valores del HUD para que Blueprints los lea
    UFUNCTION(BlueprintCallable, Category="AAA|HUD")
    void GetSocialStatus(float& OutFollowers, float& OutViralImpact, float& OutPopularSupport);

    // Eventos de interfaz disparados desde C++
    UFUNCTION(BlueprintImplementableEvent, Category="AAA|HUD")
    void OnNewGlobalEvent(const FWorldEventDataV2& EventData);

    UFUNCTION(BlueprintImplementableEvent, Category="AAA|HUD")
    void OnEvidenceUploaded(float Impact);

protected:
    UFUNCTION()
    void HandleWorldEvent(FText EventDescription);
};