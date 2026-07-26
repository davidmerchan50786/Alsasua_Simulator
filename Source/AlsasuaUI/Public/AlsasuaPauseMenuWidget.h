#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AlsasuaPauseMenuWidget.generated.h"

class UTextBlock;
class UButton;

UCLASS()
class ALSASUAUI_API UAlsasuaPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Alsasua|Pause")
	void TogglePause();

	UFUNCTION(BlueprintCallable, Category = "Alsasua|Pause")
	bool IsPaused() const { return bIsPaused; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Pause|Style")
	FLinearColor BackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.75f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Pause|Style")
	FLinearColor ButtonColor = FLinearColor(0.15f, 0.15f, 0.2f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Pause|Style")
	FLinearColor ButtonHoverColor = FLinearColor(0.25f, 0.05f, 0.05f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Pause|Style")
	FLinearColor TextColor = FLinearColor(1.f, 1.f, 1.f, 0.95f);

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildMenuLayout();
	void OnResumeClicked();
	void OnSaveClicked();
	void OnSettingsClicked();
	void OnQuitClicked();
	void SetInputModeGame();
	void SetInputModeUI();

	bool bIsPaused = false;
	bool bSettingsOpen = false;
};
