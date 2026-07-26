#include "World/AlsasuaWaterFlowMap.h"
#include "World/AlsasuaVisualEffectsManager.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"

UAlsasuaWaterFlowMap::UAlsasuaWaterFlowMap()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
}

void UAlsasuaWaterFlowMap::BeginPlay()
{
	Super::BeginPlay();
}

void UAlsasuaWaterFlowMap::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateFlowMap(DeltaTime);
}

void UAlsasuaWaterFlowMap::UpdateFlowMap(float DeltaTime)
{
	TimeAccum += DeltaTime;

	UWorld* W = GetWorld();
	if (!W) return;

	UAlsasuaVisualEffectsManager* VFXMgr = W->GetSubsystem<UAlsasuaVisualEffectsManager>();
	const float Wind = VFXMgr ? VFXMgr->WindIntensity : 0.3f;

	AActor* Owner = GetOwner();
	if (!Owner) return;

	TArray<UStaticMeshComponent*> MeshComps;
	Owner->GetComponents<UStaticMeshComponent>(MeshComps);

	for (UStaticMeshComponent* SMC : MeshComps)
	{
		if (!SMC) continue;

		const int32 NumMats = SMC->GetNumMaterials();
		for (int32 i = 0; i < NumMats; ++i)
		{
			UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(SMC->GetMaterial(i));
			if (!MID) continue;

			// Flow direction with variation
			const float DirVar = FMath::Sin(TimeAccum * 0.3f) * DirectionVariation;
			const FVector FinalDir = FlowDirectionBase.GetSafeNormal() *
				(FVector(1, DirVar, 0).GetSafeNormal().Y);

			MID->SetVectorParameterValue(FName("FlowDirection"),
				FLinearColor(FinalDir.X, FinalDir.Y, 0, BaseFlowSpeed));

			// Ripple
			const float Ripple = FMath::Sin(TimeAccum * RippleFrequency * 6.283f) * RippleAmplitude;
			MID->SetScalarParameterValue(FName("RippleAmount"), Ripple);

			// Foam based on flow speed
			const float CurrentSpeed = BaseFlowSpeed + Wind * 0.3f;
			const float Foam = (CurrentSpeed > FoamSpeedThreshold) ?
				FoamIntensity * (CurrentSpeed - FoamSpeedThreshold) / (1.f - FoamSpeedThreshold) : 0.f;
			MID->SetScalarParameterValue(FName("FoamIntensity"), Foam);

			// Time for animation
			MID->SetScalarParameterValue(FName("FlowTime"), TimeAccum * BaseFlowSpeed);
		}
	}
}
