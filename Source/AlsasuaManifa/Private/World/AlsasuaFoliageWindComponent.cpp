#include "World/AlsasuaFoliageWindComponent.h"
#include "World/AlsasuaVisualEffectsManager.h"
#include "Engine/World.h"

UAlsasuaFoliageWindComponent::UAlsasuaFoliageWindComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.05f;
}

void UAlsasuaFoliageWindComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UAlsasuaFoliageWindComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateWindParameters(DeltaTime);
}

void UAlsasuaFoliageWindComponent::UpdateWindParameters(float DeltaTime)
{
	UWorld* W = GetWorld();
	if (!W) return;

	UAlsasuaVisualEffectsManager* VFXMgr = W->GetSubsystem<UAlsasuaVisualEffectsManager>();
	if (!VFXMgr) return;

	TimeAccum += DeltaTime;

	const float GlobalWind = VFXMgr->WindIntensity;
	const float WindDir = VFXMgr->WindDirection;

	CurrentGust = FMath::FInterpTo(
		CurrentGust,
		(FMath::Sin(TimeAccum * GustFrequency * 6.283f) * 0.5f + 0.5f) * GustStrength * GlobalWind,
		DeltaTime, 3.f
	);

	const float TreeSway = TreeSwayAmplitude * GlobalWind * FMath::Sin(TimeAccum * TreeSwayFrequency + WindDir);
	const float TreeLeanAngle = TreeLean * GlobalWind * FMath::Cos(WindDir);

	const float GrassSway = GrassSwayAmplitude * GlobalWind *
		(FMath::Sin(TimeAccum * GrassSwayFrequency + WindDir) + CurrentGust);
	const float GrassBend = GrassBendAmount * GlobalWind;

	const float BushSway = BushSwayAmplitude * GlobalWind *
		FMath::Sin(TimeAccum * BushSwayFrequency * 0.7f + WindDir + CurrentGust * 2.f);

	AActor* Owner = GetOwner();
	if (!Owner) return;

	const FVector ActorOrigin = Owner->GetActorLocation();
	const float NoiseOffset = FMath::Sin(ActorOrigin.X * 0.001f) * FMath::Cos(ActorOrigin.Y * 0.001f);

	const float FinalTreeSway = TreeSway + NoiseOffset * 2.f;
	const float FinalGrassSway = GrassSway + NoiseOffset * 4.f;
	const float FinalBushSway = BushSway + NoiseOffset * 3.f;

	Owner->Tags.Empty();
	Owner->Tags.Add(FName("TreeWind"));
	Owner->Tags.Add(*FString::Printf(TEXT("Sway:%.2f"), FinalTreeSway));
	Owner->Tags.Add(*FString::Printf(TEXT("Lean:%.2f"), TreeLeanAngle));
	Owner->Tags.Add(*FString::Printf(TEXT("GrassSway:%.2f"), FinalGrassSway));
	Owner->Tags.Add(*FString::Printf(TEXT("GrassBend:%.2f"), GrassBend));
	Owner->Tags.Add(*FString::Printf(TEXT("BushSway:%.2f"), FinalBushSway));
}
