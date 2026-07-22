#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AlsasuaDebugHUD.generated.h"

UCLASS()
class ALSASUAMANIFA_API AAlsasuaDebugHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawTelemetry();
};
