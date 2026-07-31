#include "World/AlsasuaEnhancedPostProcessComponent.h"
#include "World/Time/TimeOfDayManager.h"
#include "World/AlsasuaVisualEffectsManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
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

	UTimeOfDayManager* TimeMgr = W->GetSubsystem<UTimeOfDayManager>();
	UAlsasuaVisualEffectsManager* VFXMgr = W->GetSubsystem<UAlsasuaVisualEffectsManager>();

	const float Hour = TimeMgr ? TimeMgr->CurrentTime : 12.f;
	const float Rain = VFXMgr ? VFXMgr->GlobalWetness : 0.f;

	// Day/night blend factor: 0=full night, 1=full day
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

	CurrentTemperature = FMath::FInterpTo(CurrentTemperature,
		FMath::Lerp(NightTemperature, DayTemperature, DayFactor), DeltaTime, 2.f);

	CurrentSaturation = FMath::FInterpTo(CurrentSaturation,
		FMath::Lerp(NightSaturation, DaySaturation, DayFactor), DeltaTime, 2.f);

	CurrentChromatic = FMath::FInterpTo(CurrentChromatic,
		FMath::Lerp(NightChromaticAberration, DayChromaticAberration, DayFactor), DeltaTime, 2.f);

	CurrentVignette = FMath::FInterpTo(CurrentVignette,
		FMath::Lerp(NightVignetteIntensity, DayVignetteIntensity, DayFactor), DeltaTime, 2.f);

	CurrentGrain = FMath::FInterpTo(CurrentGrain,
		FMath::Lerp(DayGrainIntensity, RainGrainIntensity, Rain), DeltaTime, 2.f);

	CurrentBloom = FMath::FInterpTo(CurrentBloom,
		FMath::Lerp(NightBloomIntensity, DayBloomIntensity, DayFactor), DeltaTime, 2.f);

	APlayerController* PC = UGameplayStatics::GetPlayerController(W, 0);
	if (!PC) return;

	APawn* Pawn = PC->GetPawn();
	if (!Pawn) return;

	TArray<AActor*> PPVolumes;
	UGameplayStatics::GetAllActorsOfClass(W, APostProcessVolume::StaticClass(), PPVolumes);

	for (AActor* VolActor : PPVolumes)
	{
		APostProcessVolume* PPV = Cast<APostProcessVolume>(VolActor);
		if (!PPV || !PPV->Settings.bOverride_AutoExposureMinBrightness) continue;

		FPostProcessSettings& S = PPV->Settings;

		S.bOverride_ColorSaturation = true;
		const float Sat = CurrentSaturation;
		S.ColorSaturation = FVector4(Sat, Sat, Sat, 1.f);

		S.bOverride_BloomIntensity = true;
		S.BloomIntensity = CurrentBloom;

		S.bOverride_MotionBlurAmount = true;
		S.MotionBlurAmount = NormalMotionBlur;

		S.bOverride_VignetteIntensity = true;
		S.VignetteIntensity = CurrentVignette;

		S.bOverride_SceneColorTint = true;
		const float TempNorm = (CurrentTemperature - 3000.f) / 10000.f;
		S.SceneColorTint = FLinearColor(
			1.f + (TempNorm - 0.5f) * 0.1f,
			1.f,
			1.f - (TempNorm - 0.5f) * 0.15f
		);

		S.bOverride_SceneFringeIntensity = true;
		S.SceneFringeIntensity = CurrentChromatic;
	}
}
