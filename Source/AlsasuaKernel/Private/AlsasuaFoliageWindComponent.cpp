#include "AlsasuaFoliageWindComponent.h"
#include "AlsasuaServiceRegistry.h"
#include "Services/IWeatherService.h"

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

	if (!GetOwner() || DeltaTime <= 0.f) return;

	// Read wind del servicio de clima, sin depender de GF_Clima en compile-time
	// (evitaria un ciclo: GF_Clima ya depende de AlsasuaKernel).
	const UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const UAlsasuaServiceRegistry* Reg = GI ? GI->GetSubsystem<UAlsasuaServiceRegistry>() : nullptr;
	const IWeatherService* Weather = Reg ? Reg->PedirComo<IWeatherService>(FName("Weather")) : nullptr;
	const float WindSpd = Weather ? Weather->GetWindSpeed() : 0.f;

	// Normalize: 0-30 kmh → 0-1
	const float WindNorm = FMath::Clamp(WindSpd / 30.f, 0.f, 1.f);
	const float EffectiveIntensity = WindIntensity * WindNorm;

	if (EffectiveIntensity <= 0.01f)
	{
		// No wind: return to rest
		if (GetOwner())
		{
			const FRotator Rest(OriginalRotation.X, OriginalRotation.Y, OriginalRotation.Z);
			GetOwner()->SetActorRotation(Rest);
		}
		CurrentSway = 0.f;
		return;
	}

	// Sinusoidal sway with phase
	CurrentPhase += DeltaTime * SwayFrequency;
	const float SwayRaw = FMath::Sin(CurrentPhase);

	// Secondary harmonic for organic feel
	const float Sway2 = FMath::Sin(CurrentPhase * 2.3f + 1.7f) * 0.3f;
	const float CombinedSway = (SwayRaw + Sway2) * 0.5f;

	// Target sway angle
	const float TargetSway = CombinedSway * MaxSwayAngle * EffectiveIntensity;

	// Smooth spring interpolation
	CurrentSway = FMath::FInterpTo(CurrentSway, TargetSway, DeltaTime, 5.f);

	// Apply rotation
	const FRotator NewRotation(
		OriginalRotation.X + CurrentSway,
		OriginalRotation.Y,
		OriginalRotation.Z + CurrentSway * 0.6f
	);

	GetOwner()->SetActorRotation(NewRotation);
}
