#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DetentionMinigameWidget.generated.h"

UCLASS()
class ALSASUAMANIFA_API UDetentionMinigameWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintImplementableEvent, Category="AAA|UI")
    void UpdateResistance(float Current, float Max);

    UFUNCTION(BlueprintImplementableEvent, Category="AAA|UI")
    void ShowQTEPrompt(float Duration);

    UFUNCTION(BlueprintImplementableEvent, Category="AAA|UI")
    void UpdateStress(float Level);
};
