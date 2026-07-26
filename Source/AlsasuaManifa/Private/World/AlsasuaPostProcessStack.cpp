#include "World/AlsasuaPostProcessStack.h"
#include "World/Time/TimeOfDayManager.h"
#include "World/AlsasuaVisualEffectsManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "PostProcessVolume.h"

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

	UTimeOfDayManager* TimeMgr = W->GetSubsystem<UTimeOfDayManager>();
	UAlsasuaVisualEffectsManager* VFXMgr = W->GetSubsystem<UAlsasuaVisualEffectsManager>();

	const float Hour = TimeMgr ? TimeMgr->CurrentTime : 12.f;
	const float Rain = VFXMgr ? VFXMgr->GlobalWetness : 0.f;

	// Day/night factor
	float DayFactor = 1.f;
	if (Hour >= 6.f && Hour <= 20.f)
	{
		const float MidDay = 13.f;
		DayFactor = 1.f - FMath::Clamp(FMath::Abs(Hour - MidDay) / 7.f, 0.f, 1.f) * 0.3f;
	}
	else
	{
		DayFactor = 0.f;
	}

	TArray<AActor*> PPVolumes;
	UGameplayStatics::GetAllActorsOfClass(W, APostProcessVolume::StaticClass(), PPVolumes);

	for (AActor* VolActor : PPVolumes)
	{
		APostProcessVolume* PPV = Cast<APostProcessVolume>(VolActor);
		if (!PPV) continue;

		FPostProcessSettings& S = PPV->Settings;

		if (bEnableVignette)
		{
			S.bOverride_VignetteIntensity = true;
			S.VignetteIntensity = FMath::Lerp(VignetteIntensity * 1.3f, VignetteIntensity, DayFactor);
		}

		if (bEnableFilmGrain)
		{
			S.bOverride_GrainIntensity = true;
			S.GrainIntensity = FMath::Lerp(GrainIntensity, GrainIntensity * 2.f, Rain);
		}

		if (bEnableChromaticAberration)
		{
			S.bOverride_SceneFringeIntensity = true;
			S.SceneFringeIntensity = FMath::Lerp(ChromaticIntensity * 1.5f, ChromaticIntensity, DayFactor);
		}

		S.bOverride_BloomIntensity = true;
		S.BloomIntensity = FMath::Lerp(BloomIntensity * 1.5f, BloomIntensity, DayFactor);

		S.bOverride_BloomThreshold = true;
		S.BloomThreshold = BloomThreshold;

		S.bOverride_AutoExposureMinBrightness = true;
		S.AutoExposureMinBrightness = ExposureMinBrightness;

		S.bOverride_AutoExposureMaxBrightness = true;
		S.AutoExposureMaxBrightness = ExposureMaxBrightness;
	}
}
