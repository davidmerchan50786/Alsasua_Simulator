#include "AlsasuaPauseMenuWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "GuardadoSubsystem.h"

void UAlsasuaPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildMenuLayout();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UAlsasuaPauseMenuWidget::TogglePause()
{
	bIsPaused = !bIsPaused;

	if (bIsPaused)
	{
		SetVisibility(ESlateVisibility::Visible);
		SetInputModeUI();
		APlayerController* PC = GetOwningPlayer();
		if (PC) PC->SetShowMouseCursor(true);

		UGameplayStatics::SetGamePaused(GetWorld(), true);
	}
	else
	{
		SetVisibility(ESlateVisibility::Collapsed);
		SetInputModeGame();
		APlayerController* PC = GetOwningPlayer();
		if (PC) PC->SetShowMouseCursor(false);

		UGameplayStatics::SetGamePaused(GetWorld(), false);
	}
}

void UAlsasuaPauseMenuWidget::BuildMenuLayout()
{
}

void UAlsasuaPauseMenuWidget::OnResumeClicked()
{
	TogglePause();
}

void UAlsasuaPauseMenuWidget::OnSaveClicked()
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	UGuardadoSubsystem* SaveSys = GI->GetSubsystem<UGuardadoSubsystem>();
	if (SaveSys)
	{
		const bool bSaved = SaveSys->GuardarEnSlot(0);
		UE_LOG(LogTemp, Log, TEXT("[Pausa] Guardado %s"), bSaved ? TEXT("exitoso") : TEXT("FALLO"));
	}
}

void UAlsasuaPauseMenuWidget::OnSettingsClicked()
{
	bSettingsOpen = !bSettingsOpen;
}

void UAlsasuaPauseMenuWidget::OnQuitClicked()
{
	UGameplayStatics::SetGamePaused(GetWorld(), false);

	APlayerController* PC = GetOwningPlayer();
	if (PC)
	{
		UWorld* W = PC->GetWorld();
		if (W)
		{
			UGameplayStatics::OpenLevel(W, FName(TEXT("MainMenu")));
		}
	}
}

void UAlsasuaPauseMenuWidget::SetInputModeGame()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	FInputModeGameOnly InputMode;
	PC->SetInputMode(InputMode);
}

void UAlsasuaPauseMenuWidget::SetInputModeUI()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(TakeWidget());
	PC->SetInputMode(InputMode);
}

FReply UAlsasuaPauseMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::P)
	{
		TogglePause();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

int32 UAlsasuaPauseMenuWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (!bIsPaused) return LayerId;

	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const float PanelWidth = 340.f;
	const float PanelHeight = 380.f;
	const float PanelX = (Size.X - PanelWidth) * 0.5f;
	const float PanelY = (Size.Y - PanelHeight) * 0.5f;

	FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(FVector2D(0, 0), Size),
		&FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox")),
		ESlateDrawEffect::None, BackgroundColor);

	FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(FVector2D(PanelX, PanelY),
			FVector2D(PanelWidth, PanelHeight)),
		&FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox")),
		ESlateDrawEffect::None, FLinearColor(0.08f, 0.08f, 0.12f, 0.95f));

	FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(FVector2D(PanelX, PanelY),
			FVector2D(PanelWidth, 2.f)),
		&FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox")),
		ESlateDrawEffect::None, ButtonColor);

	FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 18);
	FSlateDrawElement::MakeText(OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(FVector2D(PanelX + (PanelWidth - 120.f) * 0.5f, PanelY + 16.f),
			FVector2D(120.f, 26.f)),
		FText::FromString(TEXT("PAUSA")),
		TitleFont, ESlateDrawEffect::None, TextColor);

	const FString Items[] = {
		TEXT("Continuar"),
		TEXT("Guardar Partida"),
		TEXT("Opciones"),
		TEXT("Salir al Menú")
	};

	FSlateFontInfo ItemFont = FCoreStyle::GetDefaultFontStyle("Regular", 13);
	const float ItemHeight = 44.f;
	const float ItemPad = 8.f;
	const float StartY = PanelY + 64.f;

	for (int32 i = 0; i < 4; ++i)
	{
		const float Y = StartY + i * (ItemHeight + ItemPad);
		const FLinearColor Hovered = (i == 0) ? ButtonHoverColor : ButtonColor;

		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(FVector2D(PanelX + 16.f, Y),
				FVector2D(PanelWidth - 32.f, ItemHeight)),
			&FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox")),
			ESlateDrawEffect::None, Hovered);

		FSlateDrawElement::MakeText(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(FVector2D(PanelX + 32.f, Y + 10.f),
				FVector2D(PanelWidth - 64.f, ItemHeight - 20.f)),
			FText::FromString(Items[i]),
			ItemFont, ESlateDrawEffect::None, TextColor);
	}

	return LayerId + 1;
}
