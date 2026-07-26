#include "UI/AlsasuaMainHUD.h"
#include "AI/AlsasuaCrowdSentiment.h"
#include "AI/Crowd/AlsasuaCrowdSubsystem.h"
#include "Systems/Disguise/DisguiseComponent.h"
#include "Engine/Canvas.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

AAlsasuaMainHUD::AAlsasuaMainHUD()
{
}

void AAlsasuaMainHUD::DrawHUD()
{
    Super::DrawHUD();

    DrawSocialBars();
    DrawCrowdTelemetry();
    DrawDisguiseBar();
}

// ═══════════════════════════════════════════════════════════════════════════
//  DrawSocialBars: barras de Tensión Social y Apoyo Popular.
// ═══════════════════════════════════════════════════════════════════════════
void AAlsasuaMainHUD::DrawSocialBars()
{
	if (!Canvas) return;

	UWorld* W = GetWorld();
	if (!W) return;

	UAlsasuaCrowdSentiment* Sentiment = W->GetSubsystem<UAlsasuaCrowdSentiment>();
	if (!Sentiment) return;

    if (!Canvas) return;

    const float Tension = FMath::Clamp(Sentiment->GlobalTension, 0.f, 1.f);
    const float Support = FMath::Clamp(Sentiment->PopularSupport / 100.f, 0.f, 1.f);

    // Barra de Tensión (cúspide de la pantalla).
    DrawRect(FColor::Black, 50, 50, 300, 20);
    DrawRect(BrandRed, 55, 53, 290 * Tension, 14);
    DrawText(TEXT("TENSIÓN SOCIAL"), FColor::White, 50, 30);

    // Barra de Apoyo Popular.
    DrawRect(FColor::Black, 50, 100, 300, 20);
    DrawRect(SupportBlue, 55, 103, 290 * Support, 14);
    DrawText(TEXT("APOYO POPULAR"), FColor::White, 50, 80);
}

// ═══════════════════════════════════════════════════════════════════════════
//  DrawCrowdTelemetry: telemetría real de la multitud.
// ═══════════════════════════════════════════════════════════════════════════
void AAlsasuaMainHUD::DrawCrowdTelemetry()
{
	if (!Canvas) return;

	UWorld* W = GetWorld();
	if (!W) return;

	UAlsasuaCrowdSubsystem* Crowd = W->GetSubsystem<UAlsasuaCrowdSubsystem>();
	if (!Crowd) return;

	const int32 Total = Crowd->GetAgentCount();
	const int32 Alive = Crowd->GetAliveAgentCount();
	const int32 Dead = Total - Alive;

	FString Telemetry = FString::Printf(
		TEXT("MULTITUD: %d vivos / %d total"), Alive, Total);

	if (Dead > 0)
	{
		Telemetry += FString::Printf(TEXT("  |  Bajas: %d"), Dead);
	}

	DrawText(Telemetry, FColor::Yellow, 50, Canvas->SizeY - 100);

	// Posición del jugador (debug).
	if (APlayerController* PC = GetOwningPlayerController())
	{
		if (APawn* P = PC->GetPawn())
		{
			const FVector Loc = P->GetActorLocation();
			DrawText(FString::Printf(TEXT("POS: %.0f, %.0f, %.0f"), Loc.X, Loc.Y, Loc.Z),
				FColor(150, 150, 150), 50, Canvas->SizeY - 70);
		}
	}
}

// ═══════════════════════════════════════════════════════════════════════════
//  DrawDisguiseBar: barra de durabilidad del disfraz del jugador.
// ═══════════════════════════════════════════════════════════════════════════
void AAlsasuaMainHUD::DrawDisguiseBar()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !PC->GetPawn()) return;

	UDisguiseComponent* Disguise = PC->GetPawn()->FindComponentByClass<UDisguiseComponent>();
	if (!Disguise || !Disguise->IsDisguised()) return;

    if (!Canvas) return;

    const float Percent = Disguise->GetDurabilityPercent();
    const float BarWidth = 200.f;
    const float BarHeight = 12.f;
    const float X = Canvas->SizeX - BarWidth - 50.f;
    const float Y = 50.f;

	// Fondo.
	DrawRect(FColor::Black, X, Y, BarWidth, BarHeight);

	// Barra de color según durabilidad.
	FColor BarColor;
	if (Percent > 0.5f)
	{
		BarColor = DurabilityGreen;
	}
	else if (Percent > 0.2f)
	{
		BarColor = DurabilityYellow;
	}
	else
	{
		BarColor = DurabilityRed;
	}

	DrawRect(BarColor, X + 1, Y + 1, (BarWidth - 2) * Percent, BarHeight - 2);

	// Texto del tipo de disfraz.
	static const TCHAR* TypeNames[] = { TEXT("NINGUNO"), TEXT("MOMOTXORRO"), TEXT("CASUAL"), TEXT("PRENSA") };
	const int32 TypeIdx = FMath::Clamp((int32)Disguise->GetCurrentDisguise(), 0, 3);

	FString Label = FString::Printf(TEXT("DISFRAZ: %s  %d%%"),
		TypeNames[TypeIdx], FMath::RoundToInt(Percent * 100.f));

	DrawText(Label, FColor::White, X, Y - 18);
}
