// ClimaSubsystem.cpp
#include "ClimaSubsystem.h"
#include "ArranqueMundo.h"
#include "DiaNocheSubsystem.h"
#include "World/Time/TimeOfDayManager.h"
#include "AlsasuaServiceRegistry.h"
#include "World/AlsasuaWorldSubsystem.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Sound/SoundBase.h"
#include "Materials/MaterialParameterCollection.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

UClimaSubsystem::FCondicion UClimaSubsystem::Preset(EClima C)
{
	switch (C)
	{
	case EClima::Nubes:    return { 0.f,  0.f,  0.5f };
	case EClima::Lluvia:   return { 0.6f, 0.2f, 0.7f };
	case EClima::Tormenta: return { 1.0f, 0.3f, 0.9f };
	case EClima::Niebla:   return { 0.f,  0.9f, 0.4f };
	case EClima::Despejado:
	default:               return { 0.f,  0.f,  0.f };
	}
}

void UClimaSubsystem::ForzarClima(EClima C) { Objetivo = Preset(C); }

void UClimaSubsystem::ElegirClimaAleatorio()
{
	// Pesos hacia buen tiempo (clima de valle navarro: nubes y lluvia frecuentes).
	static const EClima Tabla[] = {
		EClima::Despejado, EClima::Despejado, EClima::Nubes, EClima::Nubes,
		EClima::Lluvia, EClima::Niebla, EClima::Tormenta
	};
	Objetivo = Preset(Tabla[FMath::RandRange(0, UE_ARRAY_COUNT(Tabla) - 1)]);
	TiempoCambio = FMath::FRandRange(CambioMinSeg, CambioMaxSeg);
}

void UClimaSubsystem::AplicarNiebla()
{
	// AtmosphereController (GF_Clima) is the sole fog density owner when active.
	// Detect via ITimeOfDayService registration — no GF_Clima header dependency.
	if (!Niebla_) return;
	if (UWorld* W = GetWorld())
	{
		if (UGameInstance* GI = W->GetGameInstance())
		{
			if (UAlsasuaServiceRegistry* Reg = GI->GetSubsystem<UAlsasuaServiceRegistry>())
			{
				if (Reg->Pedir(FName("TimeOfDay")) != nullptr) return;
			}
		}
	}
	UExponentialHeightFogComponent* F = Niebla_->GetComponent();
	if (!F) return;
	const float Dens = 0.02f + Cur.Niebla * 0.18f + Cur.Lluvia * 0.04f;
	F->SetFogDensity(Dens);
	const FLinearColor Claro(0.6f, 0.7f, 0.85f), Plomo(0.5f, 0.52f, 0.55f);
	F->SetFogInscatteringColor(FMath::Lerp(Claro, Plomo, Cur.Nubosidad));
}

void UClimaSubsystem::GestionarLluviaVFX()
{
	UWorld* W = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!W) return;
	APawn* Jug = UGameplayStatics::GetPlayerPawn(W, 0);
	if (!Jug) return;

	const bool bDebeLlover = Cur.Lluvia > 0.05f;

	if (bDebeLlover && !LluviaVFX)
	{
		UNiagaraSystem* NS = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/VFX/NS_Lluvia.NS_Lluvia"));
		if (NS)
			LluviaVFX = UNiagaraFunctionLibrary::SpawnSystemAttached(
				NS, Jug->GetRootComponent(), NAME_None, FVector(0, 0, 600.f), FRotator::ZeroRotator,
				EAttachLocation::KeepRelativeOffset, true);
	}
	if (LluviaVFX)
	{
		if (!bDebeLlover) { LluviaVFX->Deactivate(); LluviaVFX->DestroyComponent(); LluviaVFX = nullptr; }
		else LluviaVFX->SetFloatParameter(TEXT("Intensidad"), Cur.Lluvia);   // si el sistema lo expone
	}
}

void UClimaSubsystem::Tick(float DeltaTime)
{
	if (!ArranqueMundo::BaselineListo) return;

	// Busca la niebla cada frame hasta encontrarla (el ciclo visual la crea aparte).
	if (!Niebla_)
		if (UWorld* W = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
			for (TActorIterator<AExponentialHeightFog> It(W); It; ++It) { Niebla_ = *It; break; }

	if (!bInit)
	{
		bInit = true;
		ElegirClimaAleatorio();
	}

	// Cuenta atrás para el próximo cambio.
	TiempoCambio -= DeltaTime;
	if (TiempoCambio <= 0.f) ElegirClimaAleatorio();

	// Interpola la condición actual hacia el objetivo.
	const float a = FMath::Clamp(TransicionPorSeg * DeltaTime, 0.f, 1.f);
	Cur.Lluvia    = FMath::Lerp(Cur.Lluvia,    Objetivo.Lluvia,    a);
	Cur.Niebla    = FMath::Lerp(Cur.Niebla,    Objetivo.Niebla,    a);
	Cur.Nubosidad = FMath::Lerp(Cur.Nubosidad, Objetivo.Nubosidad, a);

	// Setters globales de render (niebla + MPC) solo 4x/seg: los valores cambian
	// lento, y un SetFogDensity/SetScalarParameterValue por frame invalida el
	// cache de draw commands de los ~117k prims del pueblo.
	TiempoActualizacion -= DeltaTime;
	if (TiempoActualizacion <= 0.f)
	{
		TiempoActualizacion = 0.25f;
		AplicarNiebla();
		GestionarMojado(DeltaTime);
	}

	GestionarLluviaVFX();
	GestionarTormenta(DeltaTime);
}

void UClimaSubsystem::GestionarMojado(float DeltaTime)
{
	UWorld* W = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!W) return;

	// Delegar al WorldSubsystem — gestiona los 4 parámetros MPC de una vez.
	if (UAlsasuaWorldSubsystem* WorldSS = W->GetSubsystem<UAlsasuaWorldSubsystem>())
	{
		// El suelo se moja deprisa con lluvia y se seca despacio al escampar.
		const float ObjetivoMojado = Cur.Lluvia;
		if (ObjetivoMojado > Wetness) Wetness = FMath::Min(ObjetivoMojado, Wetness + 0.25f * DeltaTime);
		else                          Wetness = FMath::Max(0.f,            Wetness - 0.04f * DeltaTime);
		WorldSS->SetGlobalWetness(Wetness);
	}

	if (!MPCClima)
		MPCClima = LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/Materiales/MPC_Clima.MPC_Clima"));
	if (!MPCClima) return;

	// Noche (0 día .. 1 noche): enciende las ventanas de las fachadas.
	float Noche = 0.f;
	if (const UGameInstance* GI = GetGameInstance())
		if (const UDiaNocheSubsystem* Dn = GI->GetSubsystem<UDiaNocheSubsystem>())
		{
			const float Elev = FMath::Sin((Dn->Hora - 6.f) / 12.f * PI);  // >0 día, <0 noche
			Noche = FMath::Clamp((0.12f - Elev) / 0.30f, 0.f, 1.f);        // se enciende al caer el sol
		}
	UKismetMaterialLibrary::SetScalarParameterValue(W, MPCClima, TEXT("Night"), Noche);
}

void UClimaSubsystem::GestionarTormenta(float DeltaTime)
{
	UWorld* W = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!W) return;

	// Destello del relámpago en curso (parpadeo que decae).
	if (FlashTimer > 0.f && Relampago)
	{
		FlashTimer -= DeltaTime;
		const float k = FMath::Clamp(FlashTimer / 0.20f, 0.f, 1.f);
		const float flick = (FMath::Frac(FlashTimer * 22.f) < 0.6f) ? 1.f : 0.25f;
		if (UDirectionalLightComponent* L = Cast<UDirectionalLightComponent>(Relampago->GetLightComponent()))
			L->SetIntensity(FlashTimer > 0.f ? IntensidadFlash * k * flick : 0.f);
	}

	// Trueno diferido (el sonido llega después del fogonazo).
	if (TruenoDelay > 0.f)
	{
		TruenoDelay -= DeltaTime;
		if (TruenoDelay <= 0.f)
			if (USoundBase* S = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SC_Trueno.SC_Trueno")))
				UGameplayStatics::PlaySound2D(W, S, FMath::FRandRange(0.7f, 1.f));
	}

	// ¿Hay tormenta? (lluvia fuerte + muy nublado)
	const bool bTormenta = Cur.Lluvia > 0.7f && Cur.Nubosidad > 0.75f;
	if (!bTormenta) return;

	ProxRayo -= DeltaTime;
	if (ProxRayo > 0.f) return;

	// Dispara un relámpago.
	if (!Relampago)
	{
		Relampago = W->SpawnActor<ADirectionalLight>();
		if (Relampago)
		{
			Relampago->SetActorRotation(FRotator(-60.f, FMath::FRandRange(0.f, 360.f), 0.f));
			if (UDirectionalLightComponent* L = Cast<UDirectionalLightComponent>(Relampago->GetLightComponent()))
			{ L->SetMobility(EComponentMobility::Movable); L->SetIntensity(0.f); L->SetCastShadows(false); }
		}
	}
	FlashTimer   = 0.20f;
	TruenoDelay  = FMath::FRandRange(1.f, 4.f);   // distancia ~ retardo del trueno
	ProxRayo     = FMath::FRandRange(RayoMinSeg, RayoMaxSeg);
}
