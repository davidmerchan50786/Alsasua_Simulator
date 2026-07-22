#include "UI/AlsasuaDebugHUD.h"
#include "Core/AlsasuaBudgetManager.h"
#include "AI/AlsasuaCrowdSentiment.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"

void AAlsasuaDebugHUD::DrawHUD()
{
	Super::DrawHUD();
	DrawTelemetry();
}

void AAlsasuaDebugHUD::DrawTelemetry()
{
	UAlsasuaBudgetManager* Budget = GetWorld()->GetSubsystem<UAlsasuaBudgetManager>();
	UAlsasuaCrowdSentiment* Sentiment = GetWorld()->GetSubsystem<UAlsasuaCrowdSentiment>();

	float YStart = 50.f;

	FCanvasTextItem TextItem(FVector2D(50.f, YStart), FText::FromString(TEXT("ALSASUA MANIFA - AAA+++ ENGINE")), GEngine->GetSmallFont(), FLinearColor::White);
	TextItem.Scale = FVector2D(1.5f, 1.5f);
	Canvas->DrawItem(TextItem);

	if (Budget)
	{
		FCanvasTextItem BudgetText(FVector2D(50.f, YStart + 30.f), FText::FromString(FString::Printf(TEXT("Frame Budget: 16.6ms | Global Tension: %.2f"), Sentiment ? Sentiment->GlobalTension : 0.f)), GEngine->GetSmallFont(), FLinearColor::White);
		Canvas->DrawItem(BudgetText);
	}

	if (Sentiment && Sentiment->GlobalTension > 0.7f)
	{
		FCanvasTextItem StateText(FVector2D(50.f, YStart + 60.f), FText::FromString(TEXT("STATE: HOSTILE / PANIC")), GEngine->GetSmallFont(), FLinearColor::Red);
		StateText.Scale = FVector2D(2.0f, 2.0f);
		Canvas->DrawItem(StateText);
	}
}
