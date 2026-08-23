#include "World/AlsasuaDynamicCloudShadows.h"
#include "World/Time/TimeOfDayManager.h"
#include "World/AlsasuaVisualEffectsManager.h"
#include "AlsasuaServiceRegistry.h"
#include "ContratosClima.h"
#include "Engine/World.h"

UAlsasuaDynamicCloudShadows::UAlsasuaDynamicCloudShadows()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
}

void UAlsasuaDynamicCloudShadows::BeginPlay()
{
	Super::BeginPlay();
}

void UAlsasuaDynamicCloudShadows::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateCloudShadows(DeltaTime);
}

void UAlsasuaDynamicCloudShadows::UpdateCloudShadows(float DeltaTime)
{
	UWorld* W = GetWorld();
	if (!W) return;

	UTimeOfDayManager* TimeMgr = W->GetSubsystem<UTimeOfDayManager>();
	UAlsasuaVisualEffectsManager* VFXMgr = W->GetSubsystem<UAlsasuaVisualEffectsManager>();
	IWeatherService* Weather = [&]{ auto* R = UAlsasuaServiceRegistry::Get(W); return R ? R->PedirComo<IWeatherService>("Clima.Meteorologia") : nullptr; }();

	if (!TimeMgr || !VFXMgr) return;

	const float Hour = TimeMgr->CurrentTime;
	const bool bRaining = Weather && (Weather->GetWeatherState() == EAlsasuaWeatherState::Rainy ||
		Weather->GetWeatherState() == EAlsasuaWeatherState::Thunderstorm);

	TimeAccum += DeltaTime;

	// Cloud cover
	float TargetCover = CloudCoverDay;
	if (Hour >= 18.f || Hour < 6.f)
	{
		TargetCover = CloudCoverNight;
	}
	else if (Hour >= 16.f && Hour < 18.f)
	{
		const float T = (Hour - 16.f) / 2.f;
		TargetCover = FMath::Lerp(CloudCoverDay, CloudCoverNight, T);
	}

	if (bRaining) TargetCover = CloudCoverRain;

	CurrentCover = FMath::FInterpTo(CurrentCover, TargetCover, DeltaTime, 0.5f);

	// Cloud tint
	FLinearColor TargetColor = CloudColorDay;
	if (Hour >= 17.f && Hour < 19.f)
	{
		const float T = (Hour - 17.f) / 2.f;
		TargetColor = FMath::Lerp(CloudColorDay, CloudColorSunset, T);
	}
	else if (Hour >= 18.f || Hour < 5.f)
	{
		TargetColor = CloudColorNight;
	}

	VFXMgr->FogDensityMult = 1.f + CurrentCover * 0.5f;
}
