// MenuPrincipalHUD.cpp
#include "MenuPrincipalHUD.h"
#include "MenuPrincipalController.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void AMenuPrincipalHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) return;

	const float W = Canvas->SizeX, H = Canvas->SizeY;
	DrawRect(FLinearColor(0.015f, 0.02f, 0.03f, 1.f), 0, 0, W, H);

	// Título
	FCanvasTextItem Tit(FVector2D(W * 0.5f - 220.f, H * 0.22f), FText::FromString(TEXT("ALTSASU  MANIFA")),
		GEngine->GetLargeFont(), FLinearColor(0.95f, 0.82f, 0.4f));
	Tit.Scale = FVector2D(2.2f, 2.2f);
	Tit.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(Tit);

	FCanvasTextItem Sub(FVector2D(W * 0.5f - 150.f, H * 0.32f), FText::FromString(TEXT("Alsasua Simulator")),
		GEngine->GetMediumFont(), FLinearColor(0.6f, 0.65f, 0.72f));
	Canvas->DrawItem(Sub);

	const AMenuPrincipalController* PC = Cast<AMenuPrincipalController>(GetOwningPlayerController());
	if (!PC) return;

	const TArray<FString> Ops = PC->Opciones();
	float y = H * 0.48f;
	for (int32 i = 0; i < Ops.Num(); ++i)
	{
		const bool sel = (i == PC->Seleccion);
		const FString L = (sel ? TEXT("> ") : TEXT("  ")) + Ops[i];
		FCanvasTextItem It(FVector2D(W * 0.5f - 120.f, y), FText::FromString(L), GEngine->GetMediumFont(),
			sel ? FLinearColor(1.f, 0.9f, 0.5f) : FLinearColor(0.78f, 0.78f, 0.83f));
		It.EnableShadow(FLinearColor::Black);
		Canvas->DrawItem(It);
		y += 38.f;
	}

	FCanvasTextItem Ayuda(FVector2D(W * 0.5f - 150.f, H * 0.85f), FText::FromString(TEXT("Flechas: mover    Enter: elegir")),
		GEngine->GetMediumFont(), FLinearColor(0.5f, 0.55f, 0.65f));
	Canvas->DrawItem(Ayuda);
}
