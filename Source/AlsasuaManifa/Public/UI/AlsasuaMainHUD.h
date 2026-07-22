#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AlsasuaMainHUD.generated.h"

/** HUD AAA que centraliza la visualización de la tensión y apoyo social */
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

private:
    void DrawSocialBars();
    void DrawCrowdTelemetry();
};
