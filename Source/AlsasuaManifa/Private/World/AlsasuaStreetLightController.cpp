#include "World/AlsasuaStreetLightController.h"
#include "World/Time/TimeOfDayManager.h"
#include "World/AlsasuaVisualEffectsManager.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/BillboardComponent.h"
#include "Engine/World.h"

UAlsasuaStreetLightController::UAlsasuaStreetLightController()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.15f;
}

void UAlsasuaStreetLightController::BeginPlay()
{
	Super::BeginPlay();
	SetupLightComponents();
}

void UAlsasuaStreetLightController::SetupLightComponents()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	if (!PointLight)
	{
		PointLight = NewObject<UPointLightComponent>(Owner);
		if (PointLight)
		{
			PointLight->SetupAttachment(Owner->GetRootComponent());
			PointLight->SetRelativeLocation(FVector(0, 0, LightHeight));
			PointLight->SetIntensity(0.f);
			PointLight->SetLightColor(WarmColor);
			PointLight->SetAttenuationRadius(LightRadius);
			PointLight->SetCastShadows(true);
			PointLight->SetIntensityUnits(ELightUnits::Candelas);
			PointLight->RegisterComponent();
		}
	}

	if (bCastPuddleReflection && PointLight)
	{
		UPointLightComponent* PuddleLight = NewObject<UPointLightComponent>(Owner);
		if (PuddleLight)
		{
			PuddleLight->SetupAttachment(Owner->GetRootComponent());
			PuddleLight->SetRelativeLocation(FVector(0, 0, -50.f));
			PuddleLight->SetIntensity(0.f);
			PuddleLight->SetLightColor(WarmColor * 0.3f);
			PuddleLight->SetAttenuationRadius(PuddleReflectionRadius);
			PuddleLight->SetCastShadows(false);
			PuddleLight->RegisterComponent();
		}
	}
}

void UAlsasuaStreetLightController::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateLightState(DeltaTime);

	if (bEnableFlicker && bIsOn && !bBurnedOut)
	{
		ApplyFlicker(DeltaTime);
	}
}

void UAlsasuaStreetLightController::UpdateLightState(float DeltaTime)
{
	UWorld* W = GetWorld();
	if (!W) return;

	UTimeOfDayManager* TimeMgr = W->GetSubsystem<UTimeOfDayManager>();
	if (!TimeMgr) return;

	const float Hour = TimeMgr->CurrentTime;
	bool bShouldBeOn = false;

	if (TurnOnHour > TurnOffHour)
	{
		bShouldBeOn = (Hour >= TurnOnHour || Hour < TurnOffHour);
	}
	else
	{
		bShouldBeOn = (Hour >= TurnOnHour && Hour < TurnOffHour);
	}

	if (bBurnedOut)
	{
		TargetIntensity = 0.f;
	}
	else
	{
		TargetIntensity = bShouldBeOn ? MaxIntensity : 0.f;
	}

	bIsOn = bShouldBeOn && !bBurnedOut;

	const float FadeSpeed = (TargetIntensity > CurrentIntensity) ? (1.f / FadeInTime) : (1.f / FadeOutTime);
	CurrentIntensity = FMath::FInterpTo(CurrentIntensity, TargetIntensity, DeltaTime, FadeSpeed);

	if (PointLight)
	{
		PointLight->SetIntensity(CurrentIntensity);
	}

	if (bShouldBeOn && !bBurnedOut && FMath::FRand() < BurnOutChance)
	{
		bBurnedOut = true;
		UE_LOG(LogTemp, Log, TEXT("StreetLight: Farola quemada en %s"),
			*GetOwner()->GetName());
	}
}

void UAlsasuaStreetLightController::ApplyFlicker(float DeltaTime)
{
	FlickerTimer += DeltaTime;

	if (FMath::FRand() < FlickerChance)
	{
		if (PointLight)
		{
			const float FlickerIntensity = CurrentIntensity * FMath::RandRange(0.3f, 0.9f);
			PointLight->SetIntensity(FlickerIntensity);
		}
		FlickerTimer = 0.f;
	}
}
