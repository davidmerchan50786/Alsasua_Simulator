#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AlsasuaMainHUD.generated.h"

/** HUD AAA que centraliza la visualización de la tensión, apoyo social, multitud y disfraz */
UCLASS()
class ALSASUAMANIFA_API AAlsasuaMainHUD : public AHUD
{
    GENERATED_BODY()

public:
    AAlsasuaMainHUD();

    virtual void DrawHUD() override;

    // Colores de la marca "Altsasu Manifa"
    UPROPERTY(EditAnywhere, Category = "AAA|Style")
    FColor BrandRed = FColor(200, 30, 30);

    UPROPERTY(EditAnywhere, Category = "AAA|Style")
    FColor SupportBlue = FColor(30, 150, 250);

    UPROPERTY(EditAnywhere, Category = "AAA|Style")
    FColor DurabilityGreen = FColor(30, 200, 80);

    UPROPERTY(EditAnywhere, Category = "AAA|Style")
    FColor DurabilityYellow = FColor(220, 200, 40);

    UPROPERTY(EditAnywhere, Category = "AAA|Style")
    FColor DurabilityRed = FColor(220, 40, 40);

private:
    void DrawSocialBars();
    void DrawCrowdTelemetry();
    void DrawDisguiseBar();
};
