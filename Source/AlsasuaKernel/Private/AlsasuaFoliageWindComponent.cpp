#include "AlsasuaFoliageWindComponent.h"

UAlsasuaFoliageWindComponent::UAlsasuaFoliageWindComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	CurrentPhase = 0.f;
	CurrentSway = 0.f;
	OriginalRotation = FVector::ZeroVector;
}

void UAlsasuaFoliageWindComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bRandomizePhase)
	{
		PhaseOffset = FMath::FRandRange(0.f, 2.f * PI);
	}
	CurrentPhase = PhaseOffset;

	if (GetOwner())
	{
		const FRotator R = GetOwner()->GetActorRotation();
		OriginalRotation = FVector(R.Pitch, R.Yaw, R.Roll);
	}
}

void UAlsasuaFoliageWindComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner() || DeltaTime <= 0.f || WindIntensity <= 0.f) return;

	// Sinusoidal sway with phase
	CurrentPhase += DeltaTime * SwayFrequency;
	const float SwayRaw = FMath::Sin(CurrentPhase);

	// Secondary harmonic for organic feel
	const float Sway2 = FMath::Sin(CurrentPhase * 2.3f + 1.7f) * 0.3f;
	const float CombinedSway = (SwayRaw + Sway2) * 0.5f;

	// Target sway angle
	const float TargetSway = CombinedSway * MaxSwayAngle * WindIntensity;

	// Smooth interpolation (spring-like)
	CurrentSway = FMath::FInterpTo(CurrentSway, TargetSway, DeltaTime, 5.f);

	// Apply as pitch + roll
	const FRotator NewRotation(
		OriginalRotation.X + CurrentSway,
		OriginalRotation.Y,
		OriginalRotation.Z + CurrentSway * 0.6f
	);

	GetOwner()->SetActorRotation(NewRotation);
}
