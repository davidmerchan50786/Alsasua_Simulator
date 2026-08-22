#include "World/AlsasuaInteriorLightComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "World/Time/TimeOfDayManager.h"
#include "World/AlsasuaVisualEffectsManager.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"

UAlsasuaInteriorLightComponent::UAlsasuaInteriorLightComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.2f;
}

void UAlsasuaInteriorLightComponent::BeginPlay()
{
	Super::BeginPlay();
	SetupInteriorLights();
}

void UAlsasuaInteriorLightComponent::Configurar(int32 EnPlantas, float EnAnchoCm, int32 EnSemilla)
{
	PlantasReales = EnPlantas;
	AnchoCm = EnAnchoCm;
	Semilla = EnSemilla;
}

void UAlsasuaInteriorLightComponent::SetupInteriorLights()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	// Plantas de verdad, no MaxFloors a secas. Un caserío de una planta se
	// llevaba cuatro pisos de luces, los tres de arriba flotando sobre el
	// tejado. Si el director no ha configurado la altura se cae a MaxFloors,
	// que es lo que había.
	const int32 Plantas = (PlantasReales > 0)
		? FMath::Clamp(PlantasReales, 1, MaxFloors)
		: MaxFloors;

	// Y las luces por planta salen del ancho del footprint: separadas 250 cm,
	// en un edificio de 4 m no caben tres y se salían por las paredes.
	const int32 PorPlanta = (AnchoCm > 0.f)
		? FMath::Clamp(FMath::FloorToInt(AnchoCm / 250.f), 1, NumLightsPerFloor)
		: NumLightsPerFloor;

	// Sembrado por el id del edificio: sin esto el pueblo se enciende distinto
	// en cada arranque y comparar dos perfilados no mide nada (CLAUDE.md §11).
	FRandomStream Sorteo(Semilla * 2654435761u + 0x9E37u);

	// Se sortea ANTES de crear. Antes se creaban las doce luces siempre y luego
	// se decidía cuáles encender, así que con OnProbability=0.5 la mitad eran
	// componentes de luz registrados para no alumbrar nunca.
	InteriorLights.Reset();
	LightActive.Reset();
	LightFlickerPhase.Reset();
	UltimaIntensidad.Reset();

	for (int32 Floor = 0; Floor < Plantas; ++Floor)
	{
		for (int32 i = 0; i < PorPlanta; ++i)
		{
			if (Sorteo.GetFraction() >= OnProbability) continue;

			UPointLightComponent* Light = NewObject<UPointLightComponent>(Owner);
			if (!Light) continue;

			Light->SetupAttachment(Owner->GetRootComponent());

			const float OffsetX = (i - (PorPlanta - 1) * 0.5f) * 250.f;
			const float OffsetY = Sorteo.FRandRange(-30.f, 30.f);
			const float OffsetZ = (Floor + 0.5f) * FloorHeight;

			Light->SetRelativeLocation(FVector(OffsetX, OffsetY, OffsetZ));
			Light->SetIntensity(0.f);
			// El color se fija aquí, una vez. Se estaba recalculando y
			// reescribiendo en cada tick para dar siempre el mismo valor.
			const bool bWarm = (InteriorLights.Num() % 3 == 0);
			Light->SetLightColor(bWarm ? WarmLightColor
				: FMath::Lerp(WarmLightColor, CoolLightColor, WarmCoolBlend));
			Light->SetAttenuationRadius(LightRadius);
			Light->SetCastShadows(false);
			Light->SetIntensityUnits(ELightUnits::Candelas);
			Light->RegisterComponent();

			InteriorLights.Add(Light);
			LightActive.Add(true);
			LightFlickerPhase.Add(Sorteo.FRandRange(0.f, 10.f));
			UltimaIntensidad.Add(0.f);
		}
	}

	bInitialized = InteriorLights.Num() > 0;
}

void UAlsasuaInteriorLightComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateInteriorLights(DeltaTime);
}

void UAlsasuaInteriorLightComponent::UpdateInteriorLights(float DeltaTime)
{
	if (!bInitialized) return;

	UWorld* W = GetWorld();
	if (!W) return;

	// Lejos no se hace nada. El pueblo son 7,2 km de lado y de noche ticaban a
	// la vez las luces de los 1030 edificios, estuviera el jugador donde
	// estuviera. Al cruzar el umbral se apagan UNA vez y se deja de tocar nada,
	// que es el mismo patrón que usa UAlsasuaOptimizerSubsystem con el LOD.
	if (const APawn* Peon = W->GetFirstPlayerController()
			? W->GetFirstPlayerController()->GetPawn() : nullptr)
	{
		const AActor* Duenio = GetOwner();
		const bool bAhoraLejos = Duenio &&
			FVector::DistSquared(Peon->GetActorLocation(), Duenio->GetActorLocation())
				> DistanciaMaximaCm * DistanciaMaximaCm;

		if (bAhoraLejos)
		{
			if (!bLejos)
			{
				bLejos = true;
				for (int32 i = 0; i < InteriorLights.Num(); ++i)
				{
					if (InteriorLights[i]) InteriorLights[i]->SetIntensity(0.f);
					UltimaIntensidad[i] = 0.f;
				}
			}
			return;
		}
		bLejos = false;
	}

	UTimeOfDayManager* TimeMgr = W->GetSubsystem<UTimeOfDayManager>();
	if (!TimeMgr) return;

	const float Hour = TimeMgr->CurrentTime;

	float TargetBlend = 0.f;
	if (Hour >= TurnOnHour || Hour < TurnOffHour)
	{
		TargetBlend = 1.f;
	}
	else if (Hour >= TurnOnHour && Hour < TurnOnHour + 0.5f)
	{
		TargetBlend = (Hour - TurnOnHour) * 2.f;
	}
	else if (Hour >= TurnOffHour - 0.5f && Hour < TurnOffHour)
	{
		TargetBlend = (TurnOffHour - Hour) * 2.f;
	}

	CurrentBlend = FMath::FInterpTo(CurrentBlend, TargetBlend, DeltaTime, 2.f);

	for (int32 i = 0; i < InteriorLights.Num(); ++i)
	{
		UPointLightComponent* Light = InteriorLights[i];
		if (!Light) continue;

		float TargetIntensity = 0.f;
		if (LightActive[i] && CurrentBlend > 0.01f)
		{
			LightFlickerPhase[i] += DeltaTime * 0.5f;
			const float Flicker = 0.9f + 0.1f * FMath::Sin(LightFlickerPhase[i] + i * 2.1f);
			TargetIntensity = LightIntensity * CurrentBlend * Flicker;
		}

		const float Nueva = FMath::FInterpTo(UltimaIntensidad[i], TargetIntensity, DeltaTime, 3.f);

		// Sólo se escribe cuando cambia de verdad. Escribir en un componente de
		// luz invalida estado de render (CLAUDE.md §8.2), y aquí se hacía en
		// cada tick para todas las luces, también de día con la intensidad
		// clavada en cero: con las 12 360 que se creaban salían del orden de
		// 120 000 invalidaciones por segundo sin que cambiara un píxel.
		if (!FMath::IsNearlyEqual(Nueva, UltimaIntensidad[i], 0.5f))
		{
			Light->SetIntensity(Nueva);
			UltimaIntensidad[i] = Nueva;
		}

		// El color ya no se toca: se fija una vez en SetupInteriorLights, porque
		// no depende de nada que cambie en el tiempo.
	}
}
