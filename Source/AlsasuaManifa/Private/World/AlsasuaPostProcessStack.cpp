#include "World/AlsasuaPostProcessStack.h"
#include "World/AlsasuaVisualEffectsManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/PostProcessVolume.h"

UAlsasuaPostProcessStack::UAlsasuaPostProcessStack()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.15f;
}

void UAlsasuaPostProcessStack::BeginPlay()
{
	Super::BeginPlay();
}

void UAlsasuaPostProcessStack::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdatePostProcess(DeltaTime);
}

void UAlsasuaPostProcessStack::UpdatePostProcess(float DeltaTime)
{
	UWorld* W = GetWorld();
	if (!W) return;

	UAlsasuaVisualEffectsManager* VFXMgr = W->GetSubsystem<UAlsasuaVisualEffectsManager>();
	const float Rain = VFXMgr ? VFXMgr->GlobalWetness : 0.f;

	// Viñeta, aberración cromática y bloom los lleva
	// UAlsasuaEnhancedPostProcessComponent. Los dos escribían los mismos
	// campos de los mismos volúmenes con valores distintos cada 0.15 s y
	// 0.1 s: la imagen parpadeaba al ritmo del que ticaba último.
	TArray<AActor*> PPVolumes;
	UGameplayStatics::GetAllActorsOfClass(W, APostProcessVolume::StaticClass(), PPVolumes);

	for (AActor* VolActor : PPVolumes)
	{
		APostProcessVolume* PPV = Cast<APostProcessVolume>(VolActor);
		if (!PPV) continue;

		FPostProcessSettings& S = PPV->Settings;

		if (bEnableFilmGrain)
		{
			S.bOverride_FilmGrainIntensity = true;
			S.FilmGrainIntensity = FMath::Lerp(GrainIntensity, GrainIntensity * 2.f, Rain);
		}

		S.bOverride_BloomThreshold = true;
		S.BloomThreshold = BloomThreshold;

		// Rango de adaptación estrecho. Con 0.1-10 el auto-exposure recuperaba
		// dos órdenes de magnitud y la noche acababa igual de clara que el día.
		S.bOverride_AutoExposureMinBrightness = true;
		S.AutoExposureMinBrightness = ExposureMinBrightness;

		S.bOverride_AutoExposureMaxBrightness = true;
		S.AutoExposureMaxBrightness = ExposureMaxBrightness;

		// Adaptación del ojo: rápida al deslumbrarse, lenta al oscuro.
		S.bOverride_AutoExposureSpeedUp = true;
		S.AutoExposureSpeedUp = ExposureSpeed * 1.5f;

		S.bOverride_AutoExposureSpeedDown = true;
		S.AutoExposureSpeedDown = ExposureSpeed * 0.5f;
	}
}
