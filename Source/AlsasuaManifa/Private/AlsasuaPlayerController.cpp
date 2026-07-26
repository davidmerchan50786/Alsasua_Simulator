// AlsasuaPlayerController.cpp
#include "AlsasuaPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "AlsasuaMinimapWidget.h"
#include "AlsasuaPauseMenuWidget.h"
#include "AlsasuaSettingsWidget.h"
#include "Components/InputComponent.h"

AAlsasuaPlayerController::AAlsasuaPlayerController()
{
    MouseSensitivityX = 1.0f;
    MouseSensitivityY = 1.0f;
}

void AAlsasuaPlayerController::BeginPlay()
{
    Super::BeginPlay();

    bShowMouseCursor = false;

    FInputModeGameOnly ModoJuego;
    SetInputMode(ModoJuego);

    CreateUIWidgets();
}

void AAlsasuaPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (!InputComponent) return;

    InputComponent->BindAction(TEXT("Pause"), IE_Pressed, this, &AAlsasuaPlayerController::OnPausePressed);
    InputComponent->BindAction(TEXT("MinimapToggle"), IE_Pressed, this, &AAlsasuaPlayerController::OnMinimapTogglePressed);
    InputComponent->BindAction(TEXT("Settings"), IE_Pressed, this, &AAlsasuaPlayerController::OnSettingsPressed);
}

void AAlsasuaPlayerController::CreateUIWidgets()
{
    UWorld* W = GetWorld();
    if (!W) return;

    if (MinimapWidgetClass)
    {
        MinimapWidget = CreateWidget<UAlsasuaMinimapWidget>(this, MinimapWidgetClass);
        if (MinimapWidget)
        {
            MinimapWidget->AddToViewport(0);
            MinimapWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
    }
    else
    {
        MinimapWidget = CreateWidget<UAlsasuaMinimapWidget>(this, UAlsasuaMinimapWidget::StaticClass());
        if (MinimapWidget)
        {
            MinimapWidget->AddToViewport(0);
            MinimapWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
    }

    if (PauseMenuWidgetClass)
    {
        PauseMenuWidget = CreateWidget<UAlsasuaPauseMenuWidget>(this, PauseMenuWidgetClass);
    }
    else
    {
        PauseMenuWidget = CreateWidget<UAlsasuaPauseMenuWidget>(this, UAlsasuaPauseMenuWidget::StaticClass());
    }

    if (SettingsWidgetClass)
    {
        SettingsWidget = CreateWidget<UAlsasuaSettingsWidget>(this, SettingsWidgetClass);
        if (SettingsWidget)
        {
            SettingsWidget->AddToViewport(10);
            SettingsWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
    else
    {
        SettingsWidget = CreateWidget<UAlsasuaSettingsWidget>(this, UAlsasuaSettingsWidget::StaticClass());
        if (SettingsWidget)
        {
            SettingsWidget->AddToViewport(10);
            SettingsWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void AAlsasuaPlayerController::SetMouseSensitivity(float X, float Y)
{
    MouseSensitivityX = FMath::Max(0.05f, X);
    MouseSensitivityY = FMath::Max(0.05f, Y);
}

void AAlsasuaPlayerController::TogglePause()
{
    bJuegoEnPausa = !bJuegoEnPausa;
    UGameplayStatics::SetGamePaused(this, bJuegoEnPausa);

    if (PauseMenuWidget)
    {
        PauseMenuWidget->TogglePause();
    }
}

void AAlsasuaPlayerController::ShowMinimap(bool bShow)
{
    if (MinimapWidget)
    {
        if (bShow)
        {
            MinimapWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else
        {
            MinimapWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void AAlsasuaPlayerController::ToggleMinimap()
{
    if (MinimapWidget)
    {
        MinimapWidget->ToggleMinimap();
    }
}

void AAlsasuaPlayerController::OpenSettings()
{
    if (SettingsWidget)
    {
        SettingsWidget->SetVisibility(ESlateVisibility::Visible);

        FInputModeUIOnly InputMode;
        InputMode.SetWidgetToFocus(SettingsWidget->TakeWidget());
        SetInputMode(InputMode);
        SetShowMouseCursor(true);
    }
}

void AAlsasuaPlayerController::SetWaypointOnMinimap(const FVector& WorldPos)
{
    if (MinimapWidget)
    {
        MinimapWidget->SetWaypoint(WorldPos);
    }
}

void AAlsasuaPlayerController::OnPausePressed()
{
    TogglePause();
}

void AAlsasuaPlayerController::OnMinimapTogglePressed()
{
    ToggleMinimap();
}

void AAlsasuaPlayerController::OnSettingsPressed()
{
    OpenSettings();
}
