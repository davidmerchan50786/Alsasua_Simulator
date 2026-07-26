#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AlsasuaSettingsWidget.generated.h"

UCLASS()
class ALSASUAUI_API UAlsasuaSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Alsasua|Settings")
	void ApplySettings();

	UFUNCTION(BlueprintCallable, Category = "Alsasua|Settings")
	void ResetToDefaults();

	UFUNCTION(BlueprintCallable, Category = "Alsasua|Settings")
	void SaveSettings();

	UFUNCTION(BlueprintCallable, Category = "Alsasua|Settings")
	void LoadSettings();

	// --- Graphics ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Settings|Graphics")
	int32 GraphicsQuality = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Settings|Graphics")
	int32 ShadowQuality = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Settings|Graphics")
	int32 TextureQuality = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Settings|Graphics")
	int32 ViewDistance = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Settings|Graphics")
	bool bVSync = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Settings|Graphics")
	int32 FrameRateLimit = 0;

	// --- Audio ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Settings|Audio")
	float MasterVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Settings|Audio")
	float MusicVolume = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Settings|Audio")
	float SFXVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Settings|Audio")
	float VoiceVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Settings|Audio")
	float AmbientVolume = 0.6f;

	// --- Controls ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Settings|Controls")
	float MouseSensitivity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Settings|Controls")
	bool bInvertYAxis = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Settings|Controls")
	float CameraShakeIntensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Settings|Controls")
	bool bToggleSprint = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Settings|Controls")
	bool bToggleCrouch = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Settings|Style")
	FLinearColor BackgroundColor = FLinearColor(0.03f, 0.03f, 0.06f, 0.92f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Settings|Style")
	FLinearColor SectionColor = FLinearColor(0.12f, 0.03f, 0.03f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Settings|Style")
	FLinearColor TextColor = FLinearColor(0.9f, 0.9f, 0.9f, 1.f);

	UFUNCTION(BlueprintCallable, Category = "Alsasua|Settings")
	void CloseSettings();

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void DrawSectionHeader(FSlateWindowElementList& OutDrawElements, const FString& Title, float& Y,
		const FGeometry& Geom) const;
	void DrawSlider(FSlateWindowElementList& OutDrawElements, const FString& Label, float Value,
		float MinVal, float MaxVal, float Y, const FGeometry& Geom) const;
	void DrawToggle(FSlateWindowElementList& OutDrawElements, const FString& Label, bool bValue,
		float Y, const FGeometry& Geom) const;
	void DrawOption(FSlateWindowElementList& OutDrawElements, const FString& Label, int32 Current,
		const TArray<FString>& Options, float Y, const FGeometry& Geom) const;

	static FString GetSettingsSavePath();
};
