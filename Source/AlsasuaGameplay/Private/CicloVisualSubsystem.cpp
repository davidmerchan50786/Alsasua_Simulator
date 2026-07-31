// CicloVisualSubsystem.cpp
#include "CicloVisualSubsystem.h"
#include "DiaNocheSubsystem.h"
#include "ClimaSubsystem.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/SkyLight.h"
#include "Components/SkyLightComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"   // TActorIterator

template<typename T>
static T* PrimeroONulo(UWorld* W)
{
	for (TActorIterator<T> It(W); It; ++It) return *It;
	return nullptr;
}

void UCicloVisualSubsystem::CrearCielo(UWorld* W)
{
	if (!W) return;

	// Reutiliza los actores que el mapa ya tenga; solo crea los que falten.
	Sol      = PrimeroONulo<ADirectionalLight>(W);
	CieloLuz = PrimeroONulo<ASkyLight>(W);
	Niebla   = PrimeroONulo<AExponentialHeightFog>(W);

	// Sol (luz direccional móvil, marcada como sol de la atmósfera).
	if (!Sol) Sol = W->SpawnActor<ADirectionalLight>();
	if (Sol)
	{
		if (USceneComponent* R = Sol->GetRootComponent()) R->SetMobility(EComponentMobility::Movable);
		if (UDirectionalLightComponent* L = Cast<UDirectionalLightComponent>(Sol->GetLightComponent()))
		{
			L->SetIntensity(IntensidadDia);
			L->bAtmosphereSunLight = true;
			L->SetDynamicShadowCascades(4);
			L->MarkRenderStateDirty();
		}
	}

	// Atmósfera (cielo físico). Solo si no hay ninguna ya.
	Atmosfera = nullptr;
	for (TActorIterator<AActor> It(W); It; ++It)
		if (It->FindComponentByClass<USkyAtmosphereComponent>()) { Atmosfera = *It; break; }
	if (!Atmosfera) Atmosfera = W->SpawnActor<AActor>();
	if (Atmosfera && !Atmosfera->FindComponentByClass<USkyAtmosphereComponent>())
	{
#if WITH_EDITOR
		Atmosfera->SetActorLabel(TEXT("SkyAtmosphere_Alsasua"));
#endif
		USkyAtmosphereComponent* Atm = NewObject<USkyAtmosphereComponent>(Atmosfera, TEXT("SkyAtmosphere"));
		Atmosfera->SetRootComponent(Atm);
		Atm->RegisterComponent();
	}

	// Luz de cielo con captura en tiempo real (rebote ambiental dinámico).
	if (!CieloLuz) CieloLuz = W->SpawnActor<ASkyLight>();
	if (CieloLuz)
	{
		if (USceneComponent* R = CieloLuz->GetRootComponent()) R->SetMobility(EComponentMobility::Movable);
		if (USkyLightComponent* S = CieloLuz->GetLightComponent())
		{
			S->bRealTimeCapture = true;
			S->SetIntensity(1.f);
		}
	}

	// Niebla volumétrica de altura.
	if (!Niebla) Niebla = W->SpawnActor<AExponentialHeightFog>();
}

void UCicloVisualSubsystem::Actualizar(float Hora)
{
	if (!Sol) return;

	// Elevación: 0 al amanecer/atardecer (6/18 h), 1 a mediodía, negativa de noche.
	const float Elev = FMath::Sin((Hora - 6.f) / 12.f * PI);
	const float ElevDeg = Elev * 80.f;
	const float Yaw = 90.f + (Hora - 6.f) / 12.f * 180.f;
	Sol->SetActorRotation(FRotator(-ElevDeg, Yaw, 0.f));

	const float Dia = FMath::Clamp(Elev, 0.f, 1.f);           // 0 noche, 1 mediodía
	const float Horizonte = FMath::Clamp(Elev * 3.f, 0.f, 1.f); // 0 en horizonte, 1 alto

	// Atenuación por nubosidad (clima): sol y cielo bajan con mal tiempo.
	float Nub = 1.f;
	if (const UGameInstance* GI = GetGameInstance())
		if (const UClimaSubsystem* Cl = GI->GetSubsystem<UClimaSubsystem>())
			Nub = Cl->FactorNubosidad();

	if (UDirectionalLightComponent* L = Cast<UDirectionalLightComponent>(Sol->GetLightComponent()))
	{
		L->SetIntensity(FMath::Lerp(0.f, IntensidadDia, FMath::Clamp(Elev * 1.3f, 0.f, 1.f)) * Nub);
		// Tono cálido cerca del horizonte -> blanco en alto; gris si está nublado.
		const FLinearColor Calido(1.f, 0.55f, 0.30f), Blanco(1.f, 0.96f, 0.9f), Gris(0.6f, 0.62f, 0.66f);
		FLinearColor Tono = FMath::Lerp(Calido, Blanco, Horizonte);
		L->SetLightColor(FMath::Lerp(Gris, Tono, Nub));
	}

	if (CieloLuz)
		if (USkyLightComponent* S = CieloLuz->GetLightComponent())
			S->SetIntensity(FMath::Lerp(IntensidadNoche, 1.f, Dia) * FMath::Lerp(0.6f, 1.f, Nub));
}

void UCicloVisualSubsystem::Tick(float DeltaTime)
{
	UWorld* W = GetWorld() ? GetWorld() : (GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr);
	if (!W || W->WorldType == EWorldType::Editor) return;

	if (!bCreado)
	{
		// Espera a tener mundo de juego con player controller listo.
		if (!W->GetFirstPlayerController()) return;
		CrearCielo(W);
		bCreado = true;
	}

	float Hora = 12.f;
	if (const UGameInstance* GI = GetGameInstance())
		if (const UDiaNocheSubsystem* Dn = GI->GetSubsystem<UDiaNocheSubsystem>())
			Hora = Dn->Hora;

	// SetActorRotation/SetIntensity/SetLightColor del sol y skylight solo 4x/seg:
	// por frame invalidan el cache de draw commands del pueblo entero.
	TiempoActualizacion -= DeltaTime;
	if (TiempoActualizacion <= 0.f)
	{
		TiempoActualizacion = 0.25f;
		Actualizar(Hora);
	}
}
