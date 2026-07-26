#include "AlsasuaGameplayHUD.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "PlayerMappableInputConfig.h"

void AAlsasuaGameplayHUD::DrawHUD()
{
    Super::DrawHUD();

    if (!Canvas) return;

    const float ScreenW = Canvas->ClipX;
    const float ScreenH = Canvas->ClipY;

    // ── Wanted Level stars ──
    APawn* Pawn = GetOwningPawn();
    if (Pawn)
    {
        const FString DebugStr = FString::Printf(TEXT("HUD: %s @ (%.0f, %.0f)"),
            *Pawn->GetName(), Pawn->GetActorLocation().X, Pawn->GetActorLocation().Y);
    }

    // ── Retícula simple ──
    const float CrossSize = 8.0f;
    const float CX = ScreenW * 0.5f;
    const float CY = ScreenH * 0.5f;

    FLinearColor CrossColor = FLinearColor::White;
    CrossColor.A = 0.6f;

    DrawLine(CX - CrossSize, CY, CX + CrossSize, CY, CrossColor, 1.0f);
    DrawLine(CX, CY - CrossSize, CX, CY + CrossSize, CrossColor, 1.0f);

    // ── Minimal top bar: mission time ──
    const float BarHeight = 4.0f;
    DrawRect(FLinearColor(0.1f, 0.1f, 0.1f, 0.5f), 0.0f, 0.0f, ScreenW, BarHeight);

    // ── Bottom-left HUD info ──
    const float Margin = 20.0f;
    const float LineH = 18.0f;
    float Y = ScreenH - Margin - LineH;

    FLinearColor TextColor = FLinearColor::White;

    if (APlayerController* PC = GetOwningPlayerController())
    {
        if (APawn* P = PC->GetPawn())
        {
            const FString PosText = FString::Printf(TEXT("POS: %.0f, %.0f, %.0f"),
                P->GetActorLocation().X, P->GetActorLocation().Y, P->GetActorLocation().Z);
            DrawText(PosText, TextColor, Margin, Y);
            Y -= LineH;
        }
    }
}
