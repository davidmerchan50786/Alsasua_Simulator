// ManifestacionSubsystem.cpp
#include "ManifestacionSubsystem.h"
#include "ManifestanteActor.h"
#include "PoliciaActor.h"
#include "ApoyoPopularSubsystem.h"
#include "WantedSubsystem.h"
#include "AI/AlsasuaCrowdSentiment.h"
#include "Character/GameplayPostProcessComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"
#include "NavigationSystem.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "AlsasuaServiceRegistry.h"

void UManifestacionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (UWorld* W = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		if (UAlsasuaCrowdSentiment* Sent = W->GetSubsystem<UAlsasuaCrowdSentiment>())
			Sent->OnConvocarManifestacion.AddDynamic(this, &UManifestacionSubsystem::HandleConvocarDelegate);

		if (UAlsasuaServiceRegistry* Reg = GetGameInstance()->GetSubsystem<UAlsasuaServiceRegistry>())
			Reg->Publicar(FName("Manifestacion"), this);
	}
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
		// sqrt para que el reparto sea uniforme por AREA: sorteando la
		// distancia en lineal se apelotonan todos en el centro, que es
		// justo donde esta la camara cuando convoca la mision.
		const float RMin = FMath::Min(RadioMinimoConcentracion, RadioConcentracion);
		const float T = FMath::Sqrt(FMath::FRand());
		const float Dist = RMin + T * (RadioConcentracion - RMin);
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

	// Spawn visual effects at manifestation center
	SpawnManifestacionVFX(PuntoActual, Multitud.Num());

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
	// Cleanup VFX
	if (ManifestacionVFX) { ManifestacionVFX->DeactivateImmediate(); ManifestacionVFX = nullptr; }
	if (ManifestacionAudio) { ManifestacionAudio->Stop(); ManifestacionAudio = nullptr; }
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

	// Crowd dust on player when near manifestation.
	if (UWorld* W = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		if (APawn* Jug = UGameplayStatics::GetPlayerPawn(W, 0))
		{
			if (UGameplayPostProcessComponent* PP = Jug->FindComponentByClass<UGameplayPostProcessComponent>())
			{
				const float Dist = FVector::Dist(Jug->GetActorLocation(), PuntoActual);
				const float Intensity = FMath::Clamp(1.f - Dist / 3000.f, 0.f, 1.f);
				if (Intensity > 0.1f)
					PP->TriggerCrowdDust(Intensity);
			}
		}
	}

	// Presión policial en cualquier fase activa (salvo ya dispersando).
	if (Estado != EEstadoManifestacion::Dispersando && PoliciasCerca() >= PoliciasParaCarga)
	{ Disolver(true); return; }

	// Tension builds from crowd size + police proximity, drives riot probability
	if (Estado != EEstadoManifestacion::Dispersando && Estado != EEstadoManifestacion::Inactiva)
	{
		const float CrowdFactor = static_cast<float>(Multitud.Num()) / FMath::Max(1.f, static_cast<float>(TamMax));
		const float PoliceNear = PoliciasCerca();
		const float PoliceFactor = FMath::Clamp(PoliceNear / 5.f, 0.f, 1.f);
		const float TensionTarget = FMath::Clamp(CrowdFactor * 0.4f + PoliceFactor * 0.6f, 0.f, 1.f);
		Tension = FMath::FInterpConstantTo(Tension, TensionTarget, DeltaTime, 0.1f);
	}
	else
	{
		Tension = FMath::FInterpConstantTo(Tension, 0.f, DeltaTime, 0.3f);
	}

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

void UManifestacionSubsystem::SpawnManifestacionVFX(const FVector& Centro, int32 NumNPCs)
{
	UWorld* W = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!W) return;

	const float CrowdFactor = FMath::Clamp(static_cast<float>(NumNPCs) / 200.f, 0.1f, 1.f);

	// Dust cloud at crowd feet
	UNiagaraSystem* DustNS = LoadObject<UNiagaraSystem>(nullptr,
		TEXT("/Game/VFX/NS_DustCloud.NS_DustCloud"));
	if (DustNS)
	{
		ManifestacionVFX = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			W, DustNS, Centro, FRotator::ZeroRotator, FVector(1.f), true,
			true, ENCPoolMethod::AutoRelease);
		if (ManifestacionVFX)
		{
			ManifestacionVFX->SetFloatParameter(TEXT("Radius"), 800.f * CrowdFactor);
			ManifestacionVFX->SetFloatParameter(TEXT("Intensity"), CrowdFactor * 0.5f);
		}
	}

	// Crowd ambient sound
	USoundBase* CrowdSound = LoadObject<USoundBase>(nullptr,
		TEXT("/Game/Audio/SC_CrowdNoise.SC_CrowdNoise"));
	if (CrowdSound)
	{
		ManifestacionAudio = UGameplayStatics::SpawnSoundAtLocation(
			W, CrowdSound, Centro, FRotator::ZeroRotator,
			CrowdFactor * 0.8f, 1.0f, 0.f);
	}

	// Spawn protest signs scattered around the crowd
	static const FString SignMeshes[] = {
		TEXT("/Game/Props/SM_Sign_01"),
		TEXT("/Game/Props/SM_Sign_02"),
		TEXT("/Game/Props/SM_Sign_03")
	};
	const int32 NumSigns = FMath::RandRange(3, FMath::Min(8, NumNPCs / 5));
	for (int32 i = 0; i < NumSigns; ++i)
	{
		const float Ang = FMath::FRand() * 2.f * PI;
		const float Dist = FMath::FRandRange(100.f, 600.f * CrowdFactor);
		const FVector SignPos = Centro + FVector(
			FMath::Cos(Ang) * Dist, FMath::Sin(Ang) * Dist,
			FMath::RandRange(50.f, 120.f));

		UStaticMesh* SignMesh = LoadObject<UStaticMesh>(nullptr, *SignMeshes[i % 3]);
		if (!SignMesh) continue;

		AStaticMeshActor* Sign = W->SpawnActor<AStaticMeshActor>(
			AStaticMeshActor::StaticClass(), SignPos,
			FRotator(0, FMath::FRand() * 360.f, 0));
		if (Sign)
		{
			Sign->GetStaticMeshComponent()->SetStaticMesh(SignMesh);
			Sign->SetActorScale3D(FVector(FMath::FRandRange(0.3f, 0.6f)));
		}
	}
}
