#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AlsasuaMainMenuWidget.generated.h"

UCLASS()
class ALSASUAUI_API UAlsasuaMainMenuWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "Alsasua|UI")
    void OpenMenu();
    UFUNCTION(BlueprintCallable, Category = "Alsasua|UI")
    void CloseMenu();
};
