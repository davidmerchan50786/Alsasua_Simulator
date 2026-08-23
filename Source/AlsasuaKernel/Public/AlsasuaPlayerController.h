// AlsasuaPlayerController.h
// ═══════════════════════════════════════════════════════════════════════════
//  PlayerController del jugador (UE 5.4). Gestiona la sensibilidad del ratón,
//  el modo de entrada (solo juego) y la pausa.
// ═══════════════════════════════════════════════════════════════════════════

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AlsasuaPlayerController.generated.h"

class UAlsasuaMinimapWidget;
class UUserWidget;

UCLASS()
class ALSASUAKERNEL_API AAlsasuaPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AAlsasuaPlayerController();

    /** Sensibilidad horizontal del ratón (multiplicador del yaw). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raton", meta = (ClampMin = "0.05"))
    float MouseSensitivityX = 1.0f;

    /** Sensibilidad vertical del ratón (multiplicador del pitch). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raton", meta = (ClampMin = "0.05"))
    float MouseSensitivityY = 1.0f;

    /** Ajusta la sensibilidad del ratón en tiempo de ejecución (menú de opciones). */
    UFUNCTION(BlueprintCallable, Category = "Raton")
    void SetMouseSensitivity(float X, float Y);

    /** Alterna el estado de pausa del juego. */
    UFUNCTION(BlueprintCallable, Category = "Juego")
    void TogglePause();

    // --- UI Widgets ---
    UFUNCTION(BlueprintCallable, Category = "Alsasua|UI")
    void ShowMinimap(bool bShow);

    UFUNCTION(BlueprintCallable, Category = "Alsasua|UI")
    void ToggleMinimap();

    UFUNCTION(BlueprintCallable, Category = "Alsasua|UI")
    void OpenSettings();

    UFUNCTION(BlueprintCallable, Category = "Alsasua|UI")
    void SetWaypointOnMinimap(const FVector& WorldPos);

    UFUNCTION(BlueprintPure, Category = "Alsasua|UI")
    UAlsasuaMinimapWidget* GetMinimapWidget() const { return MinimapWidget; }

    UFUNCTION(BlueprintPure, Category = "Alsasua|UI")
    UUserWidget* GetPauseMenuWidget() const { return PauseMenuWidget; }

    UFUNCTION(BlueprintPure, Category = "Alsasua|UI")
    UUserWidget* GetSettingsWidget() const { return SettingsWidget; }

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

    /** true mientras el juego está en pausa (control interno de TogglePause). */
    UPROPERTY(BlueprintReadOnly, Category = "Juego")
    bool bJuegoEnPausa = false;

private:
    void CreateUIWidgets();
    void OnPausePressed();
    void OnMinimapTogglePressed();
    void OnSettingsPressed();

    UPROPERTY()
    TObjectPtr<UAlsasuaMinimapWidget> MinimapWidget;

    // Los dos menús viven en AlsasuaUI, que ya depende de este módulo. Guardarlos
    // como UUserWidget y cargar la clase por ruta rompe el ciclo Manifa->UI: el
    // controller deja de necesitar sus cabeceras.
    UPROPERTY()
    TObjectPtr<UUserWidget> PauseMenuWidget;

    UPROPERTY()
    TObjectPtr<UUserWidget> SettingsWidget;

    UPROPERTY(EditDefaultsOnly, Category = "Alsasua|UI|Widgets")
    TSubclassOf<UAlsasuaMinimapWidget> MinimapWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "Alsasua|UI|Widgets")
    TSubclassOf<UUserWidget> PauseMenuWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "Alsasua|UI|Widgets")
    TSubclassOf<UUserWidget> SettingsWidgetClass;
};
