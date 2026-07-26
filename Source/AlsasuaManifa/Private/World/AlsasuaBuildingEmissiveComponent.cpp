#include "World/AlsasuaBuildingEmissiveComponent.h"
#include "World/Time/TimeOfDayManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"

UAlsasuaBuildingEmissiveComponent::UAlsasuaBuildingEmissiveComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.2f;
}

void UAlsasuaBuildingEmissiveComponent::BeginPlay()
{
	Super::BeginPlay();
	SetupWindowMaterials();
}

void UAlsasuaBuildingEmissiveComponent::SetupWindowMaterials()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	TArray<UStaticMeshComponent*> MeshComps;
	Owner->GetComponents<UStaticMeshComponent>(MeshComps);

	const int32 TotalWindows = WindowRows * WindowCols;
	WindowOnOff.SetNum(TotalWindows);
	for (int32 i = 0; i < TotalWindows; ++i)
	{
		WindowOnOff[i] = (FMath::FRand() < NightOnProbability);
	}

	for (UStaticMeshComponent* SMC : MeshComps)
	{
		if (!SMC) continue;

		const int32 NumMats = SMC->GetNumMaterials();
		for (int32 MatIdx = 0; MatIdx < NumMats; ++MatIdx)
		{
			UMaterialInterface* MatInterface = SMC->GetMaterial(MatIdx);
			if (!MatInterface) continue;

			UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(MatInterface);
			if (!MID)
			{
				MID = UMaterialInstanceDynamic::Create(MatInterface, this);
				if (MID)
				{
					SMC->SetMaterial(MatIdx, MID);
				}
			}

			if (MID)
			{
				MID->SetVectorParameterValue(FName("EmissiveColor"), WindowOffColor);
				MID->SetScalarParameterValue(FName("EmissiveIntensity"), 0.f);
				WindowMaterials.Add(MID);
			}
		}
	}

	bInitialized = WindowMaterials.Num() > 0;
}

void UAlsasuaBuildingEmissiveComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bInitialized) return;

	UpdateEmissiveWindows(DeltaTime);
}

void UAlsasuaBuildingEmissiveComponent::UpdateEmissiveWindows(float DeltaTime)
{
	UWorld* W = GetWorld();
	if (!W) return;

	UTimeOfDayManager* TimeMgr = W->GetSubsystem<UTimeOfDayManager>();
	if (!TimeMgr) return;

	const float Hour = TimeMgr->CurrentTime;
	float TargetIntensity = 0.f;

	if (Hour >= 18.5f || Hour < 5.5f)
	{
		TargetIntensity = MaxEmissiveIntensity;
	}
	else if (Hour >= 5.5f && Hour < 7.0f)
	{
		const float T = (Hour - 5.5f) / 1.5f;
		TargetIntensity = MaxEmissiveIntensity * (1.f - T);
	}
	else if (Hour >= 17.0f && Hour < 18.5f)
	{
		const float T = (Hour - 17.0f) / 1.5f;
		TargetIntensity = MaxEmissiveIntensity * T;
	}

	CurrentEmissiveStrength = FMath::FInterpTo(CurrentEmissiveStrength, TargetIntensity, DeltaTime, 2.f);

	FlickerPhase += DeltaTime * FlickerSpeed;

	const float WarmCoolBlend = (Hour >= 20.f || Hour < 6.f) ? 0.f : 1.f;
	const FLinearColor BaseColor = FMath::Lerp(WindowColorCool, WindowColorWarm, WarmCoolBlend);

	const int32 TotalWindows = WindowOnOff.Num();

	for (int32 i = 0; i < WindowMaterials.Num(); ++i)
	{
		UMaterialInstanceDynamic* MID = WindowMaterials[i];
		if (!MID) continue;

		const int32 WindowIdx = i % TotalWindows;
		const bool bOn = WindowOnOff[WindowIdx];

		float Flicker = 1.f;
		if (bOn && CurrentEmissiveStrength > 0.1f)
		{
			Flicker = 0.85f + 0.15f * FMath::Sin(FlickerPhase + WindowIdx * 1.7f);
		}

		const float Strength = bOn ? CurrentEmissiveStrength * Flicker : 0.f;
		const FLinearColor FinalColor = bOn ? BaseColor : WindowOffColor;

		MID->SetVectorParameterValue(FName("EmissiveColor"), FinalColor);
		MID->SetScalarParameterValue(FName("EmissiveIntensity"), Strength);
	}
}
