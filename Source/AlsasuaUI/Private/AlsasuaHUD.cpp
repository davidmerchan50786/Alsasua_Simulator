// AlsasuaHUD.cpp
#include "AlsasuaHUD.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Engine/Engine.h"
#include "AlsasuaCharacter.h"
#include "ApoyoPopularSubsystem.h"
#include "EconomiaSubsystem.h"
#include "WantedSubsystem.h"
#include "DrogasSubsystem.h"
#include "DialogoSubsystem.h"
#include "MisionesSubsystem.h"
#include "ManifestacionSubsystem.h"
#include "MenuSubsystem.h"
#include "LocalizacionSubsystem.h"
#include "ArmasComponent.h"
#include "VehiculoJugable.h"
#include "PoliciaActor.h"
#include "ManifestanteActor.h"
#include "GeoDataAlsasua.h"
#include "ArranqueMundo.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Systems/Social/SocialMediaSubsystem.h"
#include "Systemics/Events/EventManagerSubsystem.h"
#include "AlsasuaAttributeSet.h"
#include "Kismet/GameplayStatics.h"

void AAlsasuaHUD::BeginPlay()
{
    Super::BeginPlay();
    if (UWorld* World = GetWorld())
        if (UEventManagerSubsystem* EventSS = World->GetSubsystem<UEventManagerSubsystem>())
            EventSS->OnDirectorAction.AddDynamic(this, &AAlsasuaHUD::HandleWorldEvent);
}

void AAlsasuaHUD::GetSocialStatus(float& OutFollowers, float& OutViralImpact, float& OutPopularSupport)
{
    OutFollowers = 0.f;
    OutViralImpact = 0.f;
    OutPopularSupport = 0.f;
    if (USocialMediaSubsystem* SocialSS = GetWorld()->GetSubsystem<USocialMediaSubsystem>())
        OutFollowers = SocialSS->GlobalFollowers;
    if (APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
        if (AAlsasuaCharacter* Char = Cast<AAlsasuaCharacter>(Player))
            if (const UAlsasuaAttributeSet* Attr = Char->GetAttributeSet())
                OutPopularSupport = Attr->GetPopularSupport();
}

void AAlsasuaHUD::HandleWorldEvent(FText EventDescription)
{
    FWorldEventDataV2 EventData;
    EventData.EventID = FName(TEXT("DirectorAlert"));
    EventData.EventAnnounceMessage = EventDescription;
    OnNewGlobalEvent(EventData);
}

void AAlsasuaHUD::Linea(const FString& Texto, float X, float& Y, const FLinearColor& Color)
{
	if (!Canvas) return;
	FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(Texto), GEngine->GetMediumFont(), Color);
	Item.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(Item);
	Y += 26.f;
}

bool AAlsasuaHUD::PantallaCarga()
{
	if (!Canvas) return false;

	const float W = Canvas->SizeX, H = Canvas->SizeY;
	const float P = FMath::Clamp(ArranqueMundo::Progreso, 0.f, 1.f);
	const bool  Tapando = !ArranqueMundo::BaselineListo;   // gate hasta el baseline

	if (Tapando)
	{
		// Fondo opaco a pantalla completa.
		DrawRect(FLinearColor(0.03f, 0.03f, 0.05f, 1.f), 0, 0, W, H);

		const FString Titulo = TEXT("Cargando Altsasu...");
		FCanvasTextItem T(FVector2D(W * 0.5f - 130.f, H * 0.40f), FText::FromString(Titulo),
			GEngine->GetLargeFont(), FLinearColor(0.9f, 0.9f, 0.95f));
		T.EnableShadow(FLinearColor::Black);
		Canvas->DrawItem(T);
	}

	// Barra de progreso (full mientras tapa; pequeña en esquina si ya juega y queda streaming).
	const float bw = Tapando ? W * 0.4f : 220.f;
	const float bh = Tapando ? 18.f : 8.f;
	const float bx = Tapando ? (W - bw) * 0.5f : 24.f;
	const float by = Tapando ? H * 0.5f : H - 28.f;

	if (Tapando || P < 1.f)
	{
		DrawRect(FLinearColor(0.12f, 0.12f, 0.14f, Tapando ? 1.f : 0.6f), bx, by, bw, bh);
		DrawRect(FLinearColor(0.85f, 0.45f, 0.2f, Tapando ? 1.f : 0.8f), bx, by, bw * P, bh);
		if (Tapando)
		{
			FCanvasTextItem Pct(FVector2D(bx + bw + 12.f, by - 2.f),
				FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(P * 100.f))),
				GEngine->GetMediumFont(), FLinearColor::White);
			Canvas->DrawItem(Pct);
		}
	}
	return Tapando;
}

void AAlsasuaHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) return;

	// Pantalla de carga: tapa el HUD hasta que el director marque el baseline.
	if (PantallaCarga()) return;

	// Menú de pausa: tapa el HUD mientras está abierto.
	if (DibujarMenu()) return;

	// ── Overlay de drogas (tinte a pantalla completa) ───────────────────────
	if (const UGameInstance* GID = GetGameInstance())
		if (const UDrogasSubsystem* Dr = GID->GetSubsystem<UDrogasSubsystem>())
			if (Dr->Activa != ESustancia::Ninguna)
			{
				const float T = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
				FLinearColor C; float A;
				switch (Dr->Activa)
				{
				case ESustancia::Porro: C = FLinearColor(0.9f, 0.55f, 0.2f); A = 0.10f + FMath::Sin(T * 1.5f) * 0.03f; break;
				case ESustancia::Speed: C = FLinearColor(0.7f, 0.85f, 1.f);  A = (FMath::Sin(T * 12.f) > 0.85f) ? 0.18f : 0.05f; break;
				case ESustancia::Chute: C = FLinearColor(0.05f, 0.02f, 0.08f); A = 0.30f + FMath::Sin(T * 0.8f) * 0.06f; break;
				case ESustancia::Tripi: C = FLinearColor::MakeFromHSV8((uint8)(FMath::Fmod(T * 40.f, 255.f)), 180, 255); A = 0.16f + FMath::Sin(T * 2.3f) * 0.05f; break;
				default: C = FLinearColor::Black; A = 0.f; break;
				}
				C.A = A;
				DrawRect(C, 0.f, 0.f, Canvas->SizeX, Canvas->SizeY);
			}

	DibujarSaludFeedback();

	float Y = 24.f;
	const float X = 24.f;

	const UGameInstance* GI = GetGameInstance();
	if (!GI) return;
	const ULocalizacionSubsystem* L = GI->GetSubsystem<ULocalizacionSubsystem>();
	auto Tx = [&](const TCHAR* K, const TCHAR* Def) { return L ? L->Texto(FName(K)) : FString(Def); };

	// Vida del jugador
	if (const AAlsasuaCharacter* P = Cast<AAlsasuaCharacter>(GetOwningPawn()))
		Linea(FString::Printf(TEXT("%s  %d / %d"), *Tx(TEXT("hud.salud"), TEXT("Salud")), P->GetVida(), P->GetVidaMax()), X, Y, FLinearColor(0.6f, 1.f, 0.6f));

	if (const UEconomiaSubsystem* Eco = GI->GetSubsystem<UEconomiaSubsystem>())
		Linea(FString::Printf(TEXT("%s  %d EUR"), *Tx(TEXT("hud.dinero"), TEXT("Dinero")), Eco->Dinero), X, Y, FLinearColor(0.95f, 0.85f, 0.4f));

	if (const UApoyoPopularSubsystem* Ap = GI->GetSubsystem<UApoyoPopularSubsystem>())
		Linea(FString::Printf(TEXT("%s  %.0f%%"), *Tx(TEXT("hud.apoyo"), TEXT("Apoyo popular")), Ap->Apoyo), X, Y, FLinearColor(0.6f, 0.8f, 1.f));

	if (const UWantedSubsystem* Wn = GI->GetSubsystem<UWantedSubsystem>())
	{
		FString Estrellas;
		for (int32 i = 0; i < 5; ++i) Estrellas += (i < Wn->NivelBusqueda) ? TEXT("*") : TEXT("-");
		Linea(FString::Printf(TEXT("%s  [%s]"), *Tx(TEXT("hud.busqueda"), TEXT("Busqueda")), *Estrellas), X, Y, FLinearColor(1.f, 0.5f, 0.4f));
	}

	if (const UManifestacionSubsystem* Mf = GI->GetSubsystem<UManifestacionSubsystem>())
		if (Mf->Activa())
		{
			const TCHAR* Est = TEXT("");
			switch (Mf->EstadoActual())
			{
			case EEstadoManifestacion::Concentracion: Est = TEXT("Concentracion"); break;
			case EEstadoManifestacion::Marcha:        Est = TEXT("Marcha"); break;
			case EEstadoManifestacion::Dispersando:   Est = TEXT("Dispersando"); break;
			default: break;
			}
			Linea(FString::Printf(TEXT("Manifa  %s  (%d)"), Est, Mf->NumManifestantes()), X, Y, FLinearColor(1.f, 0.75f, 0.3f));
		}

	// Conduciendo: velocímetro. A pie: arma + munición (abajo a la izquierda).
	if (const AVehiculoJugable* Coche = Cast<AVehiculoJugable>(GetOwningPawn()))
	{
		const int32 KmH = FMath::Abs(FMath::RoundToInt(Coche->Velocidad() * 0.036f));
		FCanvasTextItem V(FVector2D(Canvas->SizeX - 200.f, Canvas->SizeY - 56.f), FText::FromString(FString::Printf(TEXT("%d km/h"), KmH)),
			GEngine->GetLargeFont(), FLinearColor(0.9f, 0.95f, 1.f));
		V.Scale = FVector2D(1.4f, 1.4f); V.EnableShadow(FLinearColor::Black);
		Canvas->DrawItem(V);

		// Barra de salud del coche.
		const float frac = FMath::Clamp((float)Coche->GetVida() / FMath::Max(1, Coche->GetVidaMax()), 0.f, 1.f);
		const float bw = 180.f, bh = 12.f, bx = Canvas->SizeX - 200.f, by = Canvas->SizeY - 80.f;
		DrawRect(FLinearColor(0.1f, 0.1f, 0.12f, 0.7f), bx, by, bw, bh);
		const FLinearColor c = FMath::Lerp(FLinearColor(0.85f, 0.2f, 0.15f), FLinearColor(0.4f, 0.85f, 0.4f), frac);
		DrawRect(c, bx, by, bw * frac, bh);
	}
	else if (const APawn* Pw = GetOwningPawn())
		if (const UArmasComponent* Ar = Pw->FindComponentByClass<UArmasComponent>())
		{
			FString S = Ar->NombreArma();
			if (!Ar->EsCuerpoACuerpo()) S += FString::Printf(TEXT("   %d"), Ar->MunicionActual());
			FCanvasTextItem A(FVector2D(24.f, Canvas->SizeY - 44.f), FText::FromString(S), GEngine->GetLargeFont(), FLinearColor(0.92f, 0.92f, 0.85f));
			A.EnableShadow(FLinearColor::Black);
			Canvas->DrawItem(A);
		}

	DibujarMisiones();
	DibujarRadar();
	DibujarMarcador();
	DibujarMira();
	DibujarDialogo();
}

void AAlsasuaHUD::DibujarMira()
{
	if (!Canvas) return;
	const AAlsasuaCharacter* P = Cast<AAlsasuaCharacter>(GetOwningPawn());
	if (!P || P->EstaMuerto()) return;

	const float cx = Canvas->SizeX * 0.5f, cy = Canvas->SizeY * 0.5f;
	const float gap = P->EstaApuntando() ? 5.f : 13.f;   // se cierra al apuntar
	const float len = 8.f;
	const FLinearColor C = P->EstaApuntando() ? FLinearColor(1.f, 0.95f, 0.7f, 0.95f) : FLinearColor(1.f, 1.f, 1.f, 0.7f);
	DrawLine(cx, cy - gap - len, cx, cy - gap, C, 1.4f);
	DrawLine(cx, cy + gap, cx, cy + gap + len, C, 1.4f);
	DrawLine(cx - gap - len, cy, cx - gap, cy, C, 1.4f);
	DrawLine(cx + gap, cy, cx + gap + len, cy, C, 1.4f);
}

void AAlsasuaHUD::DibujarMarcador()
{
	const UGameInstance* GI = GetGameInstance();
	APlayerController* PC = GetOwningPlayerController();
	APawn* Jug = GetOwningPawn();
	if (!GI || !PC || !Jug || !Canvas) return;
	const UMisionesSubsystem* Mi = GI->GetSubsystem<UMisionesSubsystem>();
	if (!Mi || !Mi->HayMarcador()) return;

	const FVector Obj = Mi->PosMarcador() + FVector(0, 0, 150.f);
	const float Dist = FVector::Dist(Jug->GetActorLocation(), Obj) / 100.f;   // m
	const FString Etq = FString::Printf(TEXT("Objetivo  %.0f m"), Dist);

	FVector2D Pant;
	const bool bEnPantalla = PC->ProjectWorldLocationToScreen(Obj, Pant, false);
	const float W = Canvas->SizeX, H = Canvas->SizeY;
	const FLinearColor Col(1.f, 0.85f, 0.35f);

	// ProjectWorldLocationToScreen da pantalla en píxeles; detecta "detrás" si fuera de rango.
	const bool bDelante = bEnPantalla && Pant.X >= 0 && Pant.X <= W && Pant.Y >= 0 && Pant.Y <= H;

	if (bDelante)
	{
		DrawRect(Col, Pant.X - 5.f, Pant.Y - 5.f, 10.f, 10.f);   // diamante/punto
		FCanvasTextItem T(FVector2D(Pant.X + 10.f, Pant.Y - 8.f), FText::FromString(Etq), GEngine->GetMediumFont(), Col);
		T.EnableShadow(FLinearColor::Black); Canvas->DrawItem(T);
	}
	else
	{
		// Flecha al borde apuntando hacia el objetivo (por rumbo relativo).
		const FVector Dir = (Obj - Jug->GetActorLocation()).GetSafeNormal2D();
		const float YawJug = FMath::DegreesToRadians(PC->GetControlRotation().Yaw);
		const FVector Fwd(FMath::Cos(YawJug), FMath::Sin(YawJug), 0), Rt(-FMath::Sin(YawJug), FMath::Cos(YawJug), 0);
		const float ang = FMath::Atan2(FVector::DotProduct(Dir, Rt), FVector::DotProduct(Dir, Fwd));  // -pi..pi
		const float cx = W * 0.5f, cy = H * 0.5f, r = FMath::Min(W, H) * 0.35f;
		const float sx = cx + FMath::Sin(ang) * r, sy = cy - FMath::Cos(ang) * r * 0.7f;
		DrawRect(Col, sx - 6.f, sy - 6.f, 12.f, 12.f);
		FCanvasTextItem T(FVector2D(sx - 20.f, sy + 10.f), FText::FromString(Etq), GEngine->GetMediumFont(), Col);
		T.EnableShadow(FLinearColor::Black); Canvas->DrawItem(T);
	}
}

void AAlsasuaHUD::DibujarRadar()
{
	APawn* Jug = GetOwningPawn();
	UWorld* W = GetWorld();
	if (!Jug || !W || !Canvas) return;

	const float Tam = 180.f, R = Tam * 0.5f;
	const float cx = Canvas->SizeX - R - 24.f;
	const float cy = Canvas->SizeY - R - 24.f;
	const float MundoR = 12000.f;          // 120 m de alcance
	const float Esc = R / MundoR;

	DrawRect(FLinearColor(0.02f, 0.03f, 0.05f, 0.55f), cx - R, cy - R, Tam, Tam);   // fondo

	const FVector P = Jug->GetActorLocation();
	auto Blip = [&](const FVector& Pos, const FLinearColor& Col, float s)
	{
		const float dN = Pos.X - P.X, dE = Pos.Y - P.Y;   // X=norte, Y=este
		float sx = cx + dE * Esc, sy = cy - dN * Esc;
		// clamp al borde del radar (para objetivos fuera de alcance)
		const float dx = sx - cx, dy = sy - cy; const float d = FMath::Sqrt(dx*dx + dy*dy);
		if (d > R) { sx = cx + dx / d * R; sy = cy + dy / d * R; }
		DrawRect(Col, sx - s, sy - s, s * 2, s * 2);
	};

	// Herriko Plaza (referencia), policía y manifestantes.
	Blip(UAlsasuaGeoData::HerrikoPlaza(), FLinearColor(0.9f, 0.8f, 0.3f), 3.f);

	int32 nPol = 0;
	for (TActorIterator<APoliciaActor> It(W); It && nPol < 32; ++It)
		if (FVector::Dist(It->GetActorLocation(), P) <= MundoR) { Blip(It->GetActorLocation(), FLinearColor(1.f, 0.3f, 0.25f), 3.f); ++nPol; }

	int32 nMan = 0;
	for (TActorIterator<AManifestanteActor> It(W); It && nMan < 60; ++It)
		if (FVector::Dist(It->GetActorLocation(), P) <= MundoR) { Blip(It->GetActorLocation(), FLinearColor(1.f, 0.65f, 0.2f), 2.f); ++nMan; }

	// Jugador (centro) + línea de orientación.
	DrawRect(FLinearColor::White, cx - 2.f, cy - 2.f, 4.f, 4.f);
	const float Yaw = FMath::DegreesToRadians(GetOwningPlayerController() ? GetOwningPlayerController()->GetControlRotation().Yaw : 0.f);
	DrawLine(cx, cy, cx + FMath::Sin(Yaw) * R * 0.8f, cy - FMath::Cos(Yaw) * R * 0.8f, FLinearColor(0.9f, 0.95f, 1.f), 1.5f);

	// Norte (arriba).
	FCanvasTextItem N(FVector2D(cx - 5.f, cy - R - 16.f), FText::FromString(TEXT("N")), GEngine->GetMediumFont(), FLinearColor(0.8f, 0.85f, 0.95f));
	Canvas->DrawItem(N);
}

void AAlsasuaHUD::DibujarSaludFeedback()
{
	if (!Canvas) return;
	const AAlsasuaCharacter* P = Cast<AAlsasuaCharacter>(GetOwningPawn());
	if (!P) return;

	const float dt = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.016f;
	const float Vida = (float)P->GetVida();
	const float Frac = FMath::Clamp(Vida / FMath::Max(1, P->GetVidaMax()), 0.f, 1.f);
	const float W = Canvas->SizeX, H = Canvas->SizeY;

	// Flash al recibir daño.
	if (VidaPrev >= 0.f && Vida < VidaPrev) FlashDano = 1.f;
	VidaPrev = Vida;
	if (FlashDano > 0.f)
	{
		DrawRect(FLinearColor(0.8f, 0.f, 0.f, FlashDano * 0.35f), 0, 0, W, H);
		FlashDano = FMath::Max(0.f, FlashDano - dt * 2.5f);
	}

	// Viñeta de salud baja (bordes rojos pulsantes < 30%).
	if (Frac < 0.30f && Vida > 0.f)
	{
		const float T = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		const float pulso = 0.18f + FMath::Sin(T * 4.f) * 0.07f;
		const float a = pulso * (1.f - Frac / 0.30f);
		const FLinearColor C(0.6f, 0.f, 0.f, a);
		const float g = H * 0.16f;   // grosor de borde
		DrawRect(C, 0, 0, W, g);            // arriba
		DrawRect(C, 0, H - g, W, g);        // abajo
		DrawRect(C, 0, 0, W * 0.12f, H);    // izquierda
		DrawRect(C, W - W * 0.12f, 0, W * 0.12f, H);  // derecha
	}
}

bool AAlsasuaHUD::DibujarMenu()
{
	const UGameInstance* GI = GetGameInstance();
	if (!GI || !Canvas) return false;
	const UMenuSubsystem* M = GI->GetSubsystem<UMenuSubsystem>();
	if (!M || !M->Abierto()) return false;

	const float W = Canvas->SizeX, H = Canvas->SizeY;
	DrawRect(FLinearColor(0.02f, 0.02f, 0.04f, 0.85f), 0, 0, W, H);   // oscurece la escena

	const FString Titulo = (M->Pantalla == EPantallaMenu::Opciones) ? TEXT("OPCIONES") : TEXT("ALTSASU  —  PAUSA");
	FCanvasTextItem T(FVector2D(W * 0.5f - 140.f, H * 0.22f), FText::FromString(Titulo), GEngine->GetLargeFont(), FLinearColor(0.95f, 0.85f, 0.45f));
	T.EnableShadow(FLinearColor::Black); Canvas->DrawItem(T);

	const TArray<FString> Ops = M->Opciones();
	float y = H * 0.38f;
	for (int32 i = 0; i < Ops.Num(); ++i)
	{
		const bool sel = (i == M->Seleccion);
		const FString L = (sel ? TEXT("> ") : TEXT("  ")) + Ops[i];
		FCanvasTextItem It(FVector2D(W * 0.5f - 150.f, y), FText::FromString(L), GEngine->GetMediumFont(),
			sel ? FLinearColor(1.f, 0.9f, 0.5f) : FLinearColor(0.8f, 0.8f, 0.85f));
		It.EnableShadow(FLinearColor::Black); Canvas->DrawItem(It);
		y += 34.f;
	}

	FCanvasTextItem Ayuda(FVector2D(W * 0.5f - 170.f, H * 0.82f), FText::FromString(TEXT("Flechas: mover   Enter: elegir   Esc: cerrar")),
		GEngine->GetMediumFont(), FLinearColor(0.55f, 0.6f, 0.7f));
	Canvas->DrawItem(Ayuda);
	return true;
}

void AAlsasuaHUD::DibujarMisiones()
{
	const UGameInstance* GI = GetGameInstance();
	if (!GI || !Canvas) return;
	const UMisionesSubsystem* Mi = GI->GetSubsystem<UMisionesSubsystem>();
	if (!Mi || !Mi->HayMision()) return;

	const float x = Canvas->SizeX - 360.f;
	float y = 24.f;

	FCanvasTextItem Tit(FVector2D(x, y), FText::FromString(Mi->TituloActual()),
		GEngine->GetMediumFont(), FLinearColor(0.95f, 0.85f, 0.45f));
	Tit.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(Tit); y += 26.f;

	for (const FString& O : Mi->ObjetivosTexto())
	{
		const bool Hecho = O.StartsWith(TEXT("[x]"));
		FCanvasTextItem It(FVector2D(x, y), FText::FromString(O), GEngine->GetMediumFont(),
			Hecho ? FLinearColor(0.55f, 0.85f, 0.55f) : FLinearColor(0.85f, 0.85f, 0.9f));
		It.EnableShadow(FLinearColor::Black);
		Canvas->DrawItem(It); y += 22.f;
	}
}

void AAlsasuaHUD::DibujarDialogo()
{
	const UGameInstance* GI = GetGameInstance();
	if (!GI || !Canvas) return;
	const UDialogoSubsystem* Di = GI->GetSubsystem<UDialogoSubsystem>();
	if (!Di || !Di->EnCurso()) return;

	const float W = Canvas->SizeX, H = Canvas->SizeY;
	const float bx = W * 0.12f, bw = W * 0.76f;
	const float bh = H * 0.26f, by = H - bh - 24.f;

	DrawRect(FLinearColor(0.02f, 0.02f, 0.04f, 0.82f), bx, by, bw, bh);
	DrawRect(FLinearColor(0.85f, 0.45f, 0.2f, 0.9f), bx, by, bw, 3.f);   // filo superior

	float y = by + 14.f;
	const float x = bx + 20.f;

	FCanvasTextItem Hab(FVector2D(x, y), FText::FromString(Di->HablanteActual()),
		GEngine->GetMediumFont(), FLinearColor(0.95f, 0.8f, 0.4f));
	Hab.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(Hab); y += 26.f;

	FCanvasTextItem Txt(FVector2D(x, y), FText::FromString(Di->TextoActual()),
		GEngine->GetMediumFont(), FLinearColor(0.92f, 0.92f, 0.95f));
	Txt.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(Txt); y += 34.f;

	const TArray<FString> Ops = Di->OpcionesActuales();
	if (Ops.Num() == 0)
	{
		FCanvasTextItem C(FVector2D(x, by + bh - 28.f), FText::FromString(TEXT("[E] continuar")),
			GEngine->GetMediumFont(), FLinearColor(0.6f, 0.7f, 0.8f));
		Canvas->DrawItem(C);
	}
	else for (int32 i = 0; i < Ops.Num(); ++i)
	{
		FCanvasTextItem O(FVector2D(x, y), FText::FromString(FString::Printf(TEXT("%d) %s"), i + 1, *Ops[i])),
			GEngine->GetMediumFont(), FLinearColor(0.7f, 0.85f, 1.f));
		O.EnableShadow(FLinearColor::Black);
		Canvas->DrawItem(O); y += 24.f;
	}
}
