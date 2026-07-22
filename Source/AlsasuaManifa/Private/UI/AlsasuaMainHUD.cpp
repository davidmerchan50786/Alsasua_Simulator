#include "UI/AlsasuaMainHUD.h"
#include "AI/AlsasuaCrowdSentiment.h"
#include "Engine/Canvas.h"

AAlsasuaMainHUD::AAlsasuaMainHUD()
{
}

void AAlsasuaMainHUD::DrawHUD()
{
    Super::DrawHUD();

    DrawSocialBars();
    DrawCrowdTelemetry();
}

void AAlsasuaMainHUD::DrawSocialBars()
{
    UAlsasuaCrowdSentiment* Sentiment = GetWorld()->GetSubsystem<UAlsasuaCrowdSentiment>();
    if (!Sentiment) return;

    float Tension = FMath::Clamp(Sentiment->GlobalTension, 0.f, 1.f);
    float Support = FMath::Clamp(Sentiment->PopularSupport / 100.f, 0.f, 1.f);

    // Barra de Tensión (Cúspide de la pantalla)
    DrawRect(FColor::Black, 50, 50, 300, 20);
    DrawRect(BrandRed, 55, 53, 290 * Tension, 14);
    DrawText(TEXT("TENSIÓN SOCIAL"), FColor::White, 50, 30);

    // Barra de Apoyo Popular
    DrawRect(FColor::Black, 50, 100, 300, 20);
    DrawRect(SupportBlue, 55, 103, 290 * Support, 14);
    DrawText(TEXT("APOYO POPULAR"), FColor::White, 50, 80);
}

void AAlsasuaMainHUD::DrawCrowdTelemetry()
{
    // En una implementación final, aquí contaríamos los agentes en cada estado
    // Para la demo, mostramos el conteo de simulación
    FString Telemetry = FString::Printf(TEXT("SISTEMA ALTSASU: MONITORIZANDO 1.000+ AGENTES"));
    DrawText(Telemetry, FColor::Yellow, 50, Canvas->SizeY - 100);
}
