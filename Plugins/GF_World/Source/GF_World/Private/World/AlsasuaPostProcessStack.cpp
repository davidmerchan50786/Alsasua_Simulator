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

	PPVolumeRefreshTimer += DeltaTime;
	if (PPVolumeRefreshTimer >= 5.f || CachedPPVolumes.Num() == 0)
	{
		PPVolumeRefreshTimer = 0.f;
		CachedPPVolumes.Empty();
		UGameplayStatics::GetAllActorsOfClass(W, APostProcessVolume::StaticClass(), CachedPPVolumes);
	}

	for (AActor* VolActor : CachedPPVolumes)
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

		S.bOverride_AutoExposureMinBrightness = true;
		S.AutoExposureMinBrightness = ExposureMinBrightness;

		S.bOverride_AutoExposureMaxBrightness = true;
		S.AutoExposureMaxBrightness = ExposureMaxBrightness;

		S.bOverride_AutoExposureSpeedUp = true;
		S.AutoExposureSpeedUp = ExposureSpeed * 1.5f;

		S.bOverride_AutoExposureSpeedDown = true;
		S.AutoExposureSpeedDown = ExposureSpeed * 0.5f;

		// Contextual DOF: diaphragm DOF via F-stop (UE 5.8 physical camera)
		if (bEnableContextualDOF)
		{
			S.bOverride_DepthOfFieldEnabled = true;
			S.DepthOfFieldEnabled = true;

			S.bOverride_DepthOfFieldFocalDistance = true;
			S.DepthOfFieldFocalDistance = DOFFocalDistance;

			S.bOverride_DepthOfFieldFstop = true;
			S.DepthOfFieldFstop = DOFAperture;

			S.bOverride_DepthOfFieldDepthBlurAmount = true;
			S.DepthOfFieldDepthBlurAmount = DOFMaxBlur;
		}

		// Color Grading LUT
		if (bEnableLUT && ColorGradingLUT)
		{
			S.bOverride_ColorGradingLUT = true;
			S.ColorGradingLUT = ColorGradingLUT;
		}
	}
}
