// AlsasuaPlayerController.cpp
#include "AlsasuaPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UI/AlsasuaMinimapWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/InputComponent.h"

namespace
{
    // AlsasuaUI depende de este módulo, así que aquí no se puede incluir su cabecera.
    // La clase se busca en el registro de UObjects por su ruta de script: si el
    // módulo no está cargado sale nullptr y el menú simplemente no aparece.
    UClass* ClaseDeUI(const TCHAR* Ruta)
    {
        return FindObject<UClass>(nullptr, Ruta);
    }
}

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

    UClass* ClasePausa = PauseMenuWidgetClass ? PauseMenuWidgetClass.Get() : ClaseDeUI(TEXT("/Script/AlsasuaUI.AlsasuaPauseMenuWidget"));
    if (ClasePausa)
    {
        PauseMenuWidget = CreateWidget<UUserWidget>(this, ClasePausa);
    }

    UClass* ClaseAjustes = SettingsWidgetClass ? SettingsWidgetClass.Get() : ClaseDeUI(TEXT("/Script/AlsasuaUI.AlsasuaSettingsWidget"));
    if (ClaseAjustes)
    {
        SettingsWidget = CreateWidget<UUserWidget>(this, ClaseAjustes);
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

    // TogglePause del menú es UFUNCTION, así que se llama por reflexión y no hace
    // falta el tipo concreto de AlsasuaUI.
    if (PauseMenuWidget)
    {
        if (UFunction* Alternar = PauseMenuWidget->FindFunction(TEXT("TogglePause")))
        {
            PauseMenuWidget->ProcessEvent(Alternar, nullptr);
        }
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
