// AudioAmbienteSubsystem.cpp
#include "AudioAmbienteSubsystem.h"
#include "ClimaSubsystem.h"
#include "ManifestacionSubsystem.h"
#include "DiaNocheSubsystem.h"
#include "DialogoSubsystem.h"
#include "ArranqueMundo.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

UAudioComponent* UAudioAmbienteSubsystem::CrearCama(const TCHAR* Ruta)
{
	UWorld* W = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!W) return nullptr;
	USoundBase* S = LoadObject<USoundBase>(nullptr, Ruta);
	if (!S) { UE_LOG(LogTemp, Warning, TEXT("[Audio] sin asset %s"), Ruta); return nullptr; }
	// Persistente, sin autodestruir, volumen inicial 0 (lo sube el fundido).
	UAudioComponent* C = UGameplayStatics::SpawnSound2D(W, S, 0.f, 1.f, 0.f, nullptr, true, false);
	return C;
}

void UAudioAmbienteSubsystem::FundirA(UAudioComponent* C, float Objetivo, float DeltaTime)
{
	if (!C) return;
	const float a = FMath::Clamp(SuavizadoPorSeg * DeltaTime, 0.f, 1.f);
	const float v = FMath::Lerp(C->VolumeMultiplier, FMath::Clamp(Objetivo, 0.f, 1.f), a);
	C->SetVolumeMultiplier(v);
}

void UAudioAmbienteSubsystem::Tick(float DeltaTime)
{
	if (!ArranqueMundo::BaselineListo) return;

	if (!bInit)
	{
		bInit = true;
		Lluvia   = CrearCama(TEXT("/Game/Audio/Amb_Lluvia.Amb_Lluvia"));
		Viento   = CrearCama(TEXT("/Game/Audio/Amb_Viento.Amb_Viento"));
		Multitud = CrearCama(TEXT("/Game/Audio/Amb_Multitud.Amb_Multitud"));
		AmbDia   = CrearCama(TEXT("/Game/Audio/Amb_Dia.Amb_Dia"));
		AmbNoche = CrearCama(TEXT("/Game/Audio/Amb_Noche.Amb_Noche"));
	}

	const UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	// --- Objetivos de volumen por contexto ---
	float vLluvia = 0.f, vViento = 0.15f, vMultitud = 0.f, vDia = 0.6f, vNoche = 0.f;

	if (const UClimaSubsystem* Cl = GI->GetSubsystem<UClimaSubsystem>())
	{
		vLluvia = Cl->Lluvia();
		vViento = 0.12f + (1.f - Cl->FactorNubosidad());   // viento sube con mal tiempo
	}

	if (const UManifestacionSubsystem* Mf = GI->GetSubsystem<UManifestacionSubsystem>())
		if (Mf->Activa())
			vMultitud = FMath::Clamp(Mf->NumManifestantes() / 40.f, 0.2f, 1.f);

	if (const UDiaNocheSubsystem* Dn = GI->GetSubsystem<UDiaNocheSubsystem>())
	{
		const bool bDia = Dn->EsDia();
		vDia   = bDia ? 0.6f : 0.f;
		vNoche = bDia ? 0.f  : 0.5f;
	}

	// La lluvia apaga un poco los ambientes de fauna.
	const float silenciaFauna = 1.f - vLluvia * 0.7f;

	// Ducking: baja el ambiente cuando hay diálogo, para que se entienda la voz.
	float Duck = 1.f;
	if (const UDialogoSubsystem* Di = GI->GetSubsystem<UDialogoSubsystem>())
		if (Di->EnCurso()) Duck = DuckingDialogo;

	const float M = FMath::Clamp(VolumenMaestro, 0.f, 1.f) * Duck;

	FundirA(Lluvia,   vLluvia * M, DeltaTime);
	FundirA(Viento,   vViento * M, DeltaTime);
	FundirA(Multitud, vMultitud * M, DeltaTime);
	FundirA(AmbDia,   vDia * silenciaFauna * M, DeltaTime);
	FundirA(AmbNoche, vNoche * silenciaFauna * M, DeltaTime);
}
