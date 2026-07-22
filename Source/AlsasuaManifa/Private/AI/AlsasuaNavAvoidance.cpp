#include "AI/AlsasuaNavAvoidance.h"
#include "Core/AlsasuaSpatialGrid.h"
#include "GameFramework/Character.h"

UAlsasuaNavAvoidance::UAlsasuaNavAvoidance()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAlsasuaNavAvoidance::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

FVector UAlsasuaNavAvoidance::GetSteeringAdjustment(const FVector& DesiredVelocity)
{
	AActor* Owner = GetOwner();
	if (!Owner || DesiredVelocity.IsNearlyZero()) return FVector::ZeroVector;

	UAlsasuaSpatialGrid* Grid = GetWorld()->GetSubsystem<UAlsasuaSpatialGrid>();
	if (!Grid) return FVector::ZeroVector;

	TArray<AActor*> NearbyActors;
	Grid->GetNearbyActors(Owner->GetActorLocation(), AgentRadius * 3.f, NearbyActors);

	FVector AvoidanceForce = FVector::ZeroVector;

	for (AActor* Other : NearbyActors)
	{
		if (Other == Owner) continue;

		FVector ToOther = Other->GetActorLocation() - Owner->GetActorLocation();
		float Distance = ToOther.Size();

		// Predicción de colisión: ¿Nuestras trayectorias se cortan?
		FVector RelativeVelocity = Owner->GetVelocity() - Other->GetVelocity();

		// Si nos acercamos peligrosamente
		if (FVector::DotProduct(RelativeVelocity, ToOther) > 0)
		{
			// Calcular vector de escape perpendicular
			FVector SideVector = FVector::CrossProduct(ToOther, FVector::UpVector);
			float Direction = FVector::DotProduct(SideVector, DesiredVelocity) > 0 ? 1.f : -1.f;

			float ForceMagnitude = FMath::Clamp(1.0f - (Distance / (AgentRadius * 3.f)), 0.f, 1.f);
			AvoidanceForce += SideVector.GetSafeNormal() * Direction * ForceMagnitude * 500.f;
		}
	}

	return AvoidanceForce;
}
