#include "World/AlsasuaEnhancedPostProcessComponent.h"
#include "World/Time/TimeOfDayManager.h"
#include "Services/ITimeOfDayService.h"
#include "AlsasuaServiceRegistry.h"
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

	UAlsasuaServiceRegistry* Reg = UAlsasuaServiceRegistry::Get(W);
	ITimeOfDayService* Atmos = Reg ? Reg->PedirComo<ITimeOfDayService>(FName("TimeOfDay")) : nullptr;
	UTimeOfDayManager* TimeMgr = W->GetSubsystem<UTimeOfDayManager>();

	float DayFactor;
	if (Atmos)
	{
		DayFactor = FMath::Clamp(Atmos->GetSunPitch() / 10.f, 0.f, 1.f);
	}
	else
	{
		const float Hour = TimeMgr ? TimeMgr->CurrentTime : 12.f;
		DayFactor = (Hour >= 7.f && Hour <= 20.f) ? 1.f : 0.f;
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

	// Refresh cached PP volumes every 5s instead of every tick
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
