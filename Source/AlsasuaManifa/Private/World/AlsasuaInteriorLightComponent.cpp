#include "World/AlsasuaInteriorLightComponent.h"
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

void UAlsasuaInteriorLightComponent::SetupInteriorLights()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	const int32 TotalLights = MaxFloors * NumLightsPerFloor;
	InteriorLights.SetNum(TotalLights);
	LightActive.SetNum(TotalLights);
	LightFlickerPhase.SetNum(TotalLights);

	for (int32 Floor = 0; Floor < MaxFloors; ++Floor)
	{
		for (int32 i = 0; i < NumLightsPerFloor; ++i)
		{
			const int32 Idx = Floor * NumLightsPerFloor + i;

			UPointLightComponent* Light = NewObject<UPointLightComponent>(Owner);
			if (!Light) continue;

			Light->SetupAttachment(Owner->GetRootComponent());

			const float OffsetX = (i - (NumLightsPerFloor - 1) * 0.5f) * 250.f;
			const float OffsetY = FMath::RandRange(-30.f, 30.f);
			const float OffsetZ = (Floor + 0.5f) * FloorHeight;

			Light->SetRelativeLocation(FVector(OffsetX, OffsetY, OffsetZ));
			Light->SetIntensity(0.f);
			Light->SetLightColor(WarmLightColor);
			Light->SetAttenuationRadius(LightRadius);
			Light->SetCastShadows(false);
			Light->SetIntensityUnits(ELightUnits::Candelas);
			Light->RegisterComponent();

			InteriorLights[Idx] = Light;
			LightActive[Idx] = (FMath::FRand() < OnProbability);
			LightFlickerPhase[Idx] = FMath::RandRange(0.f, 10.f);
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

		const float CurrentInt = Light->Intensity;
		Light->SetIntensity(FMath::FInterpTo(CurrentInt, TargetIntensity, DeltaTime, 3.f));

		const bool bWarm = (i % 3 == 0);
		const FLinearColor Color = bWarm ? WarmLightColor :
			FMath::Lerp(WarmLightColor, CoolLightColor, WarmCoolBlend);
		Light->SetLightColor(Color);
	}
}
