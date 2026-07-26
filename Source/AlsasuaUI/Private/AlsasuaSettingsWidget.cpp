#include "AlsasuaSettingsWidget.h"
#include "World/AlsasuaGraphicsSettingsSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "HAL/IConsoleManager.h"
#include "Interfaces/IPluginManager.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Engine/GameUserSettings.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

FString UAlsasuaSettingsWidget::GetSettingsSavePath()
{
	return FPaths::ProjectSavedDir() / TEXT("Config/AlsasuaSettings.json");
}

void UAlsasuaSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	LoadSettings();
}

void UAlsasuaSettingsWidget::ApplySettings()
{
	UWorld* W = GetWorld();
	if (!W) return;

	UAlsasuaGraphicsSettingsSubsystem* GFX = W->GetSubsystem<UAlsasuaGraphicsSettingsSubsystem>();
	if (GraphicsQuality >= 0 && GraphicsQuality <= 3)
	{
		GFX->ApplyGraphicsProfile((EAlsasuaGraphicsProfile)GraphicsQuality);
	}

	UGameUserSettings* Settings = GEngine->GetGameUserSettings();
	if (Settings)
	{
		switch (ShadowQuality)
		{
		case 0: Settings->SetShadowQualityLevel(0); break;
		case 1: Settings->SetShadowQualityLevel(1); break;
		case 2: Settings->SetShadowQualityLevel(2); break;
		case 3: Settings->SetShadowQualityLevel(3); break;
		default: break;
		}

		switch (TextureQuality)
		{
		case 0: Settings->SetTextureQualityLevel(0); break;
		case 1: Settings->SetTextureQualityLevel(1); break;
		case 2: Settings->SetTextureQualityLevel(2); break;
		case 3: Settings->SetTextureQualityLevel(3); break;
		default: break;
		}

		switch (ViewDistance)
		{
		case 0: Settings->SetViewDistanceQuality(0); break;
		case 1: Settings->SetViewDistanceQuality(1); break;
		case 2: Settings->SetViewDistanceQuality(2); break;
		case 3: Settings->SetViewDistanceQuality(3); break;
		default: break;
		}

		Settings->SetVSyncEnabled(bVSync);
		Settings->SetFrameRateLimit(FrameRateLimit > 0 ? FrameRateLimit : 0);
		Settings->ApplySettings(false);
	}

	if (IConsoleVariable* CVarMouse = IConsoleManager::Get().FindConsoleVariable(TEXT("g.MouseSensitivity")))
	{
		CVarMouse->Set(MouseSensitivity);
	}

	if (IConsoleVariable* CVarInvert = IConsoleManager::Get().FindConsoleVariable(TEXT("g.InvertYAxis")))
	{
		CVarInvert->Set(bInvertYAxis ? 1 : 0);
	}

	if (IConsoleVariable* CVarShake = IConsoleManager::Get().FindConsoleVariable(TEXT("g.CameraShakeIntensity")))
	{
		CVarShake->Set(CameraShakeIntensity);
	}

	const FString AudioCmd = FString::Printf(TEXT("au.MasterVolume %f"), MasterVolume);
	UKismetSystemLibrary::ExecuteConsoleCommand(W, AudioCmd);

	const FString MusicCmd = FString::Printf(TEXT("au.MusicVolume %f"), MusicVolume);
	UKismetSystemLibrary::ExecuteConsoleCommand(W, MusicCmd);

	const FString SFXCmd = FString::Printf(TEXT("au.SFXVolume %f"), SFXVolume);
	UKismetSystemLibrary::ExecuteConsoleCommand(W, SFXCmd);

	const FString VoiceCmd = FString::Printf(TEXT("au.VoiceVolume %f"), VoiceVolume);
	UKismetSystemLibrary::ExecuteConsoleCommand(W, VoiceCmd);

	const FString AmbCmd = FString::Printf(TEXT("au.AmbientVolume %f"), AmbientVolume);
	UKismetSystemLibrary::ExecuteConsoleCommand(W, AmbCmd);

	SaveSettings();
	UE_LOG(LogTemp, Log, TEXT("[Settings] Configuración aplicada"));
}

void UAlsasuaSettingsWidget::ResetToDefaults()
{
	GraphicsQuality = 2;
	ShadowQuality = 2;
	TextureQuality = 3;
	ViewDistance = 2;
	bVSync = true;
	FrameRateLimit = 0;
	MasterVolume = 1.0f;
	MusicVolume = 0.7f;
	SFXVolume = 1.0f;
	VoiceVolume = 1.0f;
	AmbientVolume = 0.6f;
	MouseSensitivity = 1.0f;
	bInvertYAxis = false;
	CameraShakeIntensity = 1.0f;
	bToggleSprint = false;
	bToggleCrouch = false;
}

void UAlsasuaSettingsWidget::SaveSettings()
{
	TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();

	Json->SetNumberField(TEXT("GraphicsQuality"), GraphicsQuality);
	Json->SetNumberField(TEXT("ShadowQuality"), ShadowQuality);
	Json->SetNumberField(TEXT("TextureQuality"), TextureQuality);
	Json->SetNumberField(TEXT("ViewDistance"), ViewDistance);
	Json->SetBoolField(TEXT("VSync"), bVSync);
	Json->SetNumberField(TEXT("FrameRateLimit"), FrameRateLimit);
	Json->SetNumberField(TEXT("MasterVolume"), MasterVolume);
	Json->SetNumberField(TEXT("MusicVolume"), MusicVolume);
	Json->SetNumberField(TEXT("SFXVolume"), SFXVolume);
	Json->SetNumberField(TEXT("VoiceVolume"), VoiceVolume);
	Json->SetNumberField(TEXT("AmbientVolume"), AmbientVolume);
	Json->SetNumberField(TEXT("MouseSensitivity"), MouseSensitivity);
	Json->SetBoolField(TEXT("InvertYAxis"), bInvertYAxis);
	Json->SetNumberField(TEXT("CameraShakeIntensity"), CameraShakeIntensity);
	Json->SetBoolField(TEXT("ToggleSprint"), bToggleSprint);
	Json->SetBoolField(TEXT("ToggleCrouch"), bToggleCrouch);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	if (FJsonSerializer::Deserialize(Writer, Json.ToSharedRef()))
	{
		const FString SavePath = GetSettingsSavePath();
		const FString Dir = FPaths::GetPath(SavePath);
		if (!FPaths::DirectoryExists(Dir))
		{
			IFileManager::Get().MakeDirectory(*Dir);
		}
		FFileHelper::SaveStringToFile(*OutputString, *SavePath);
		UE_LOG(LogTemp, Log, TEXT("[Settings] Configuración guardada en %s"), *SavePath);
	}
}

void UAlsasuaSettingsWidget::LoadSettings()
{
	const FString SavePath = GetSettingsSavePath();
	FString JsonContent;
	if (!FFileHelper::LoadFileToString(JsonContent, *SavePath)) return;

	TSharedPtr<FJsonObject> Json;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid()) return;

	if (Json->TryGetNumberField(TEXT("GraphicsQuality"), GraphicsQuality)) {}
	if (Json->TryGetNumberField(TEXT("ShadowQuality"), ShadowQuality)) {}
	if (Json->TryGetNumberField(TEXT("TextureQuality"), TextureQuality)) {}
	if (Json->TryGetNumberField(TEXT("ViewDistance"), ViewDistance)) {}
	Json->TryGetBoolField(TEXT("VSync"), bVSync);
	if (Json->TryGetNumberField(TEXT("FrameRateLimit"), FrameRateLimit)) {}
	if (Json->TryGetNumberField(TEXT("MasterVolume"), MasterVolume)) {}
	if (Json->TryGetNumberField(TEXT("MusicVolume"), MusicVolume)) {}
	if (Json->TryGetNumberField(TEXT("SFXVolume"), SFXVolume)) {}
	if (Json->TryGetNumberField(TEXT("VoiceVolume"), VoiceVolume)) {}
	if (Json->TryGetNumberField(TEXT("AmbientVolume"), AmbientVolume)) {}
	if (Json->TryGetNumberField(TEXT("MouseSensitivity"), MouseSensitivity)) {}
	Json->TryGetBoolField(TEXT("InvertYAxis"), bInvertYAxis);
	if (Json->TryGetNumberField(TEXT("CameraShakeIntensity"), CameraShakeIntensity)) {}
	Json->TryGetBoolField(TEXT("ToggleSprint"), bToggleSprint);
	Json->TryGetBoolField(TEXT("ToggleCrouch"), bToggleCrouch);

	UE_LOG(LogTemp, Log, TEXT("[Settings] Configuración cargada"));
}

void UAlsasuaSettingsWidget::CloseSettings()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

FReply UAlsasuaSettingsWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		ApplySettings();
		CloseSettings();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

int32 UAlsasuaSettingsWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const float PanelWidth = 440.f;
	const float PanelHeight = 560.f;
	const float PanelX = (Size.X - PanelWidth) * 0.5f;
	const float PanelY = (Size.Y - PanelHeight) * 0.5f;

	FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(FVector2D(0, 0), Size),
		&FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox")),
		ESlateDrawEffect::None, FLinearColor(0, 0, 0, 0.6f));

	FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(FVector2D(PanelX, PanelY), FVector2D(PanelWidth, PanelHeight)),
		&FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox")),
		ESlateDrawEffect::None, BackgroundColor);

	float Y = PanelY + 20.f;

	FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 16);
	FSlateDrawElement::MakeText(OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(FVector2D(PanelX + 16.f, Y), FVector2D(200.f, 24.f)),
		FText::FromString(TEXT("OPCIONES")),
		TitleFont, ESlateDrawEffect::None, TextColor);
	Y += 36.f;

	DrawSectionHeader(OutDrawElements, TEXT("Gráficos"), Y, AllottedGeometry);

	TArray<FString> QualityOpts = { TEXT("Bajo"), TEXT("Medio"), TEXT("Alto"), TEXT("Ultra") };
	DrawOption(OutDrawElements, TEXT("Calidad General"), GraphicsQuality, QualityOpts, Y, AllottedGeometry); Y += 32.f;
	DrawOption(OutDrawElements, TEXT("Sombras"), ShadowQuality, QualityOpts, Y, AllottedGeometry); Y += 32.f;
	DrawOption(OutDrawElements, TEXT("Texturas"), TextureQuality, QualityOpts, Y, AllottedGeometry); Y += 32.f;
	DrawOption(OutDrawElements, TEXT("Distancia"), ViewDistance, QualityOpts, Y, AllottedGeometry); Y += 32.f;
	DrawToggle(OutDrawElements, TEXT("V-Sync"), bVSync, Y, AllottedGeometry); Y += 32.f;
	DrawSlider(OutDrawElements, TEXT("FPS Máx"), (float)FrameRateLimit, 0.f, 240.f, Y, AllottedGeometry); Y += 40.f;

	DrawSectionHeader(OutDrawElements, TEXT("Audio"), Y, AllottedGeometry);
	DrawSlider(OutDrawElements, TEXT("Volumen Maestro"), MasterVolume, 0.f, 1.f, Y, AllottedGeometry); Y += 32.f;
	DrawSlider(OutDrawElements, TEXT("Música"), MusicVolume, 0.f, 1.f, Y, AllottedGeometry); Y += 32.f;
	DrawSlider(OutDrawElements, TEXT("Efectos"), SFXVolume, 0.f, 1.f, Y, AllottedGeometry); Y += 32.f;
	DrawSlider(OutDrawElements, TEXT("Voces"), VoiceVolume, 0.f, 1.f, Y, AllottedGeometry); Y += 32.f;
	DrawSlider(OutDrawElements, TEXT("Ambiente"), AmbientVolume, 0.f, 1.f, Y, AllottedGeometry); Y += 40.f;

	DrawSectionHeader(OutDrawElements, TEXT("Controles"), Y, AllottedGeometry);
	DrawSlider(OutDrawElements, TEXT("Sensibilidad Ratón"), MouseSensitivity, 0.1f, 3.f, Y, AllottedGeometry); Y += 32.f;
	DrawToggle(OutDrawElements, TEXT("Invertir Eje Y"), bInvertYAxis, Y, AllottedGeometry); Y += 32.f;
	DrawSlider(OutDrawElements, TEXT("Sacudida Cámara"), CameraShakeIntensity, 0.f, 2.f, Y, AllottedGeometry); Y += 32.f;
	DrawToggle(OutDrawElements, TEXT("Toggle Correr"), bToggleSprint, Y, AllottedGeometry); Y += 32.f;
	DrawToggle(OutDrawElements, TEXT("Toggle Agachar"), bToggleCrouch, Y, AllottedGeometry); Y += 40.f;

	FSlateFontInfo HintFont = FCoreStyle::GetDefaultFontStyle("Italic", 9);
	FSlateDrawElement::MakeText(OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(
			FVector2D(PanelX + 16.f, PanelY + PanelHeight - 28.f),
			FVector2D(PanelWidth - 32.f, 20.f)),
		FText::FromString(TEXT("ESC para aplicar y cerrar")),
		HintFont, ESlateDrawEffect::None, FLinearColor(0.5f, 0.5f, 0.5f, 0.7f));

	return LayerId + 1;
}

void UAlsasuaSettingsWidget::DrawSectionHeader(FSlateWindowElementList& OutDrawElements, const FString& Title, float& Y,
	const FGeometry& Geom) const
{
	FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", 11);
	const FVector2D Size = Geom.GetLocalSize();
	const float PanelWidth = 440.f;
	const float PanelX = (Size.X - PanelWidth) * 0.5f;

	FSlateDrawElement::MakeBox(OutDrawElements, 0,
		Geom.ToPaintGeometry(FVector2D(PanelX + 16.f, Y), FVector2D(PanelWidth - 32.f, 1.f)),
		&FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox")),
		ESlateDrawEffect::None, SectionColor);

	FSlateDrawElement::MakeText(OutDrawElements, 0,
		Geom.ToPaintGeometry(FVector2D(PanelX + 20.f, Y + 6.f), FVector2D(200.f, 16.f)),
		FText::FromString(Title),
		Font, ESlateDrawEffect::None, FLinearColor(0.7f, 0.3f, 0.3f, 1.f));

	Y += 28.f;
}

void UAlsasuaSettingsWidget::DrawSlider(FSlateWindowElementList& OutDrawElements, const FString& Label, float Value,
	float MinVal, float MaxVal, float Y, const FGeometry& Geom) const
{
	const FVector2D Size = Geom.GetLocalSize();
	const float PanelWidth = 440.f;
	const float PanelX = (Size.X - PanelWidth) * 0.5f;
	const float SliderWidth = 200.f;

	FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 10);
	FSlateDrawElement::MakeText(OutDrawElements, 0,
		Geom.ToPaintGeometry(FVector2D(PanelX + 20.f, Y + 6.f), FVector2D(180.f, 16.f)),
		FText::FromString(Label),
		Font, ESlateDrawEffect::None, TextColor);

	const float SliderX = PanelX + PanelWidth - SliderWidth - 50.f;
	FSlateDrawElement::MakeBox(OutDrawElements, 0,
		Geom.ToPaintGeometry(FVector2D(SliderX, Y + 10.f), FVector2D(SliderWidth, 4.f)),
		&FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox")),
		ESlateDrawEffect::None, FLinearColor(0.2f, 0.2f, 0.25f, 1.f));

	const float Normalized = (MaxVal > MinVal) ? (Value - MinVal) / (MaxVal - MinVal) : 0.f;
	FSlateDrawElement::MakeBox(OutDrawElements, 0,
		Geom.ToPaintGeometry(FVector2D(SliderX, Y + 10.f),
			FVector2D(SliderWidth * Normalized, 4.f)),
		&FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox")),
		ESlateDrawEffect::None, FLinearColor(0.8f, 0.2f, 0.2f, 1.f));

	const FString ValStr = (MaxVal <= 1.f)
		? FString::Printf(TEXT("%.0f%%"), Value * 100.f)
		: FString::Printf(TEXT("%.0f"), Value);

	FSlateDrawElement::MakeText(OutDrawElements, 0,
		Geom.ToPaintGeometry(FVector2D(SliderX + SliderWidth + 8.f, Y + 4.f), FVector2D(30.f, 16.f)),
		FText::FromString(ValStr),
		Font, ESlateDrawEffect::None, TextColor);
}

void UAlsasuaSettingsWidget::DrawToggle(FSlateWindowElementList& OutDrawElements, const FString& Label, bool bValue,
	float Y, const FGeometry& Geom) const
{
	const FVector2D Size = Geom.GetLocalSize();
	const float PanelWidth = 440.f;
	const float PanelX = (Size.X - PanelWidth) * 0.5f;

	FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 10);
	FSlateDrawElement::MakeText(OutDrawElements, 0,
		Geom.ToPaintGeometry(FVector2D(PanelX + 20.f, Y + 6.f), FVector2D(200.f, 16.f)),
		FText::FromString(Label),
		Font, ESlateDrawEffect::None, TextColor);

	const float ToggleX = PanelX + PanelWidth - 70.f;
	const FLinearColor BoxColor = bValue
		? FLinearColor(0.2f, 0.7f, 0.3f, 1.f)
		: FLinearColor(0.3f, 0.3f, 0.35f, 1.f);

	FSlateDrawElement::MakeBox(OutDrawElements, 0,
		Geom.ToPaintGeometry(FVector2D(ToggleX, Y + 4.f), FVector2D(40.f, 18.f)),
		&FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox")),
		ESlateDrawEffect::None, BoxColor);

	const FString StateStr = bValue ? TEXT("ON") : TEXT("OFF");
	FSlateDrawElement::MakeText(OutDrawElements, 0,
		Geom.ToPaintGeometry(FVector2D(ToggleX + 6.f, Y + 5.f), FVector2D(28.f, 16.f)),
		FText::FromString(StateStr),
		Font, ESlateDrawEffect::None, FLinearColor::White);
}

void UAlsasuaSettingsWidget::DrawOption(FSlateWindowElementList& OutDrawElements, const FString& Label, int32 Current,
	const TArray<FString>& Options, float Y, const FGeometry& Geom) const
{
	const FVector2D Size = Geom.GetLocalSize();
	const float PanelWidth = 440.f;
	const float PanelX = (Size.X - PanelWidth) * 0.5f;

	FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 10);
	FSlateDrawElement::MakeText(OutDrawElements, 0,
		Geom.ToPaintGeometry(FVector2D(PanelX + 20.f, Y + 6.f), FVector2D(200.f, 16.f)),
		FText::FromString(Label),
		Font, ESlateDrawEffect::None, TextColor);

	const float OptWidth = 55.f;
	const float OptX = PanelX + PanelWidth - Options.Num() * OptWidth - 10.f;

	for (int32 i = 0; i < Options.Num(); ++i)
	{
		const float X = OptX + i * OptWidth;
		const bool bSelected = (i == Current);
		const FLinearColor BG = bSelected ? FLinearColor(0.6f, 0.1f, 0.1f, 1.f) : FLinearColor(0.15f, 0.15f, 0.2f, 1.f);

		FSlateDrawElement::MakeBox(OutDrawElements, 0,
			Geom.ToPaintGeometry(FVector2D(X, Y + 2.f), FVector2D(OptWidth - 2.f, 22.f)),
			&FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox")),
			ESlateDrawEffect::None, BG);

		FSlateFontInfo OptFont = FCoreStyle::GetDefaultFontStyle(bSelected ? "Bold" : "Regular", 9);
		FSlateDrawElement::MakeText(OutDrawElements, 0,
			Geom.ToPaintGeometry(FVector2D(X + 4.f, Y + 5.f), FVector2D(OptWidth - 8.f, 16.f)),
			FText::FromString(Options[i]),
			OptFont, ESlateDrawEffect::None, bSelected ? FLinearColor::White : FLinearColor(0.6f, 0.6f, 0.6f, 1.f));
	}
}
