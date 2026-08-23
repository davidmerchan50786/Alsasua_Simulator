// ManifestacionSubsystem.cpp
#include "ManifestacionSubsystem.h"
#include "ManifestanteActor.h"
#include "PoliciaActor.h"
#include "ApoyoPopularSubsystem.h"
#include "WantedSubsystem.h"
#include "AI/AlsasuaCrowdSentiment.h"
#include "NavigationSystem.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

void UManifestacionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (UWorld* W = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
		if (UAlsasuaCrowdSentiment* Sent = W->GetSubsystem<UAlsasuaCrowdSentiment>())
			Sent->OnConvocarManifestacion.AddDynamic(this, &UManifestacionSubsystem::HandleConvocarDelegate);
}

int32 UManifestacionSubsystem::TamanoPorApoyo() const
{
	float Apoyo = 50.f;
	if (const UGameInstance* GI = GetGameInstance())
		if (const UApoyoPopularSubsystem* Ap = GI->GetSubsystem<UApoyoPopularSubsystem>())
			Apoyo = Ap->Apoyo;
	return FMath::RoundToInt(FMath::Lerp((float)TamMin, (float)TamMax, FMath::Clamp(Apoyo / 100.f, 0.f, 1.f)));
}

void UManifestacionSubsystem::AplicarApoyo(float Delta)
{
	if (UGameInstance* GI = GetGameInstance())
		if (UApoyoPopularSubsystem* Ap = GI->GetSubsystem<UApoyoPopularSubsystem>())
		{
			if (Delta >= 0) Ap->SumarApoyo(Delta, TEXT("manifestacion"));
			else            Ap->RestarApoyo(-Delta, TEXT("manifestacion"));
		}
}

void UManifestacionSubsystem::SubirBusqueda(int32 N)
{
	if (UGameInstance* GI = GetGameInstance())
		if (UWantedSubsystem* Wn = GI->GetSubsystem<UWantedSubsystem>())
			Wn->AumentarBusqueda(N);
}

bool UManifestacionSubsystem::Convocar(FVector Punto, const TArray<FVector>& RutaMarcha)
{
	if (Estado != EEstadoManifestacion::Inactiva) return false;
	UWorld* W = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!W) return false;
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(W);

	PuntoActual = Punto;
	Ruta = RutaMarcha;
	RutaIdx = 0;
	Tiempo = 0.f;

	const int32 Tam = TamanoPorApoyo();
	for (int32 i = 0; i < Tam; ++i)
	{
		const float Ang = FMath::FRandRange(0.f, 2.f * PI);
		const float Dist = FMath::FRandRange(0.f, RadioConcentracion);
		FVector Cand = Punto + FVector(FMath::Cos(Ang) * Dist, FMath::Sin(Ang) * Dist, 0.f);

		FNavLocation Loc;
		if (Nav && Nav->ProjectPointToNavigation(Cand, Loc, FVector(300, 300, 1000))) Cand = Loc.Location;
		Cand.Z += 90.f;

		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AManifestanteActor* M = W->SpawnActor<AManifestanteActor>(
			AManifestanteActor::StaticClass(), Cand, FRotator(0, FMath::FRandRange(0.f, 360.f), 0), SP);
		if (M) { M->PuntoObjetivo = PuntoActual; Multitud.Add(M); }
	}

	FijarEstado(EEstadoManifestacion::Concentracion);
	UE_LOG(LogTemp, Log, TEXT("[Manifa] convocada: %d manifestantes"), Multitud.Num());
	return Multitud.Num() > 0;
}

int32 UManifestacionSubsystem::PoliciasCerca() const
{
	const UWorld* W = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!W) return 0;
	int32 n = 0;
	for (TActorIterator<APoliciaActor> It(W); It; ++It)
		if (FVector::Dist(It->GetActorLocation(), PuntoActual) <= RadioPresionPolicia) ++n;
	return n;
}

void UManifestacionSubsystem::ActualizarObjetivos()
{
	for (int32 i = Multitud.Num() - 1; i >= 0; --i)
	{
		AManifestanteActor* M = Multitud[i];
		if (!IsValid(M)) { Multitud.RemoveAtSwap(i); continue; }
		M->PuntoObjetivo = PuntoActual;
	}
}

void UManifestacionSubsystem::DespawnTodos()
{
	for (AManifestanteActor* M : Multitud) if (IsValid(M)) M->Destroy();
	Multitud.Empty();
}

void UManifestacionSubsystem::FijarEstado(EEstadoManifestacion E)
{
	Estado = E;
	Tiempo = 0.f;
	if (E == EEstadoManifestacion::Dispersando)
		for (AManifestanteActor* M : Multitud) if (IsValid(M)) M->bDispersar = true;
	OnEstado.Broadcast(E);
}

void UManifestacionSubsystem::Disolver(bool bPorPolicia)
{
	if (Estado == EEstadoManifestacion::Inactiva || Estado == EEstadoManifestacion::Dispersando) return;
	if (bPorPolicia)
	{
		SubirBusqueda(1);
		AplicarApoyo(+3.f);   // la represión visible genera simpatía
		UE_LOG(LogTemp, Log, TEXT("[Manifa] carga policial: dispersada"));
	}
	FijarEstado(EEstadoManifestacion::Dispersando);
}

void UManifestacionSubsystem::Tick(float DeltaTime)
{
	if (Estado == EEstadoManifestacion::Inactiva) return;
	Tiempo += DeltaTime;
	ActualizarObjetivos();

	// Presión policial en cualquier fase activa (salvo ya dispersando).
	if (Estado != EEstadoManifestacion::Dispersando && PoliciasCerca() >= PoliciasParaCarga)
	{ Disolver(true); return; }

	switch (Estado)
	{
	case EEstadoManifestacion::Concentracion:
		AplicarApoyo(TasaApoyoPorSeg * DeltaTime);
		if (Tiempo >= DuracionConcentracion)
		{
			if (Ruta.Num() > 0) FijarEstado(EEstadoManifestacion::Marcha);
			else                Disolver(false);   // mitin estático: termina pacífico
		}
		break;

	case EEstadoManifestacion::Marcha:
	{
		AplicarApoyo(TasaApoyoPorSeg * DeltaTime);
		if (Ruta.IsValidIndex(RutaIdx))
		{
			const FVector Meta = Ruta[RutaIdx];
			const FVector Dir = (Meta - PuntoActual).GetSafeNormal2D();
			PuntoActual += Dir * VelocidadMarcha * DeltaTime;
			if (FVector::Dist2D(PuntoActual, Meta) < 200.f) ++RutaIdx;
		}
		else Disolver(false);   // fin de recorrido
		break;
	}

	case EEstadoManifestacion::Dispersando:
		if (Tiempo >= DuracionDispersion)
		{
			DespawnTodos();
			FijarEstado(EEstadoManifestacion::Inactiva);
			UE_LOG(LogTemp, Log, TEXT("[Manifa] inactiva"));
		}
		break;

	default: break;
	}
}

void UManifestacionSubsystem::HandleConvocarDelegate(FVector Punto)
{
	if (!Activa())
		Convocar(Punto, TArray<FVector>());
}
