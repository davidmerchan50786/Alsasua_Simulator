#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "WorldEventTypes.h"
#include "AlsasuaHUD.generated.h"

UCLASS()
class ALSASUAUI_API AAlsasuaHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
	virtual void BeginPlay() override;

	// Social / Eventos del mundo (desde AlsasuaEventHUD)
	UFUNCTION(BlueprintCallable, Category="AAA|HUD")
	void GetSocialStatus(float& OutFollowers, float& OutViralImpact, float& OutPopularSupport);

	UFUNCTION(BlueprintImplementableEvent, Category="AAA|HUD")
	void OnNewGlobalEvent(const FWorldEventDataV2& EventData);

	UFUNCTION(BlueprintImplementableEvent, Category="AAA|HUD")
	void OnEvidenceUploaded(float Impact);

protected:
	UFUNCTION()
	void HandleWorldEvent(FText EventDescription);

private:
	void Linea(const FString& Texto, float X, float& Y, const FLinearColor& Color);
	bool PantallaCarga();
	void DibujarDialogo();
	void DibujarMisiones();
	bool DibujarMenu();
	void DibujarRadar();
	void DibujarMarcador();
	void DibujarSaludFeedback();
	void DibujarMira();

	float VidaPrev = -1.f;
	float FlashDano = 0.f;
};
