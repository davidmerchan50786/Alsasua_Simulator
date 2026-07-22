#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AlsasuaGameplayHUD.generated.h"

UCLASS()
class ALSASUAGAMEPLAY_API AAlsasuaGameplayHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
};
