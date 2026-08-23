#include "World/AlsasuaEnhancedPostProcessComponent.h"
#include "AlsasuaServiceRegistry.h"
#include "ContratosClima.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/PostProcessVolume.h"

UAlsasuaEnhancedPostProcessComponent::UAlsasuaEnhancedPostProcessComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
}

void UAlsasuaEnhancedPostProcessComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UAlsasuaEnhancedPostProcessComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdatePostProcess(DeltaTime);
}

void UAlsasuaEnhancedPostProcessComponent::UpdatePostProcess(float DeltaTime)
{
	UWorld* W = GetWorld();
	if (!W) return;

	UAlsasuaServiceRegistry* Registro = UAlsasuaServiceRegistry::Get(W);
	ITimeOfDayService* Tiempo = Registro ? Registro->PedirComo<ITimeOfDayService>("Clima.TiempoDelDia") : nullptr;

	// La mezcla día/noche sale de la elevación real del sol, no de tramos de
	// hora: así el grading cambia cuando cambia la luz, no a las 20:00 en punto.
	// Sin servicio publicado (plugin dormido) se asume día, como en el resto
	// de consumidores.
	float DayFactor = 1.f;
	if (Tiempo)
	{
		DayFactor = FMath::Clamp(Tiempo->GetSunPitch() / 10.f, 0.f, 1.f);
	}

	CurrentTint = FLinearColor::LerpUsingHSV(NightColorTint, DayColorTint, DayFactor);

	CurrentSaturation = FMath::FInterpTo(CurrentSaturation,
		FMath::Lerp(NightSaturation, DaySaturation, DayFactor), DeltaTime, 2.f);

	CurrentChromatic = FMath::FInterpTo(CurrentChromatic,
		FMath::Lerp(NightChromaticAberration, DayChromaticAberration, DayFactor), DeltaTime, 2.f);

	CurrentVignette = FMath::FInterpTo(CurrentVignette,
		FMath::Lerp(NightVignetteIntensity, DayVignetteIntensity, DayFactor), DeltaTime, 2.f);

	CurrentBloom = FMath::FInterpTo(CurrentBloom,
		FMath::Lerp(NightBloomIntensity, DayBloomIntensity, DayFactor), DeltaTime, 2.f);

	CurrentExposureBias = FMath::FInterpTo(CurrentExposureBias,
		FMath::Lerp(NightExposureBias, DayExposureBias, DayFactor), DeltaTime, 1.f);

	TArray<AActor*> PPVolumes;
	UGameplayStatics::GetAllActorsOfClass(W, APostProcessVolume::StaticClass(), PPVolumes);

	for (AActor* VolActor : PPVolumes)
	{
		APostProcessVolume* PPV = Cast<APostProcessVolume>(VolActor);
		if (!PPV) continue;

		FPostProcessSettings& S = PPV->Settings;

		// ColorSaturation, BloomIntensity, VignetteIntensity are owned by
		// UAlsasuaZonePostProcess (per-barrio grading). Only set our unique
		// fields here to avoid fighting every tick.

		S.bOverride_MotionBlurAmount = true;
		S.MotionBlurAmount = NormalMotionBlur;

		S.bOverride_SceneColorTint = true;
		S.SceneColorTint = CurrentTint;

		S.bOverride_SceneFringeIntensity = true;
		S.SceneFringeIntensity = CurrentChromatic;

		S.bOverride_AutoExposureBias = true;
		S.AutoExposureBias = CurrentExposureBias;
	}
}
