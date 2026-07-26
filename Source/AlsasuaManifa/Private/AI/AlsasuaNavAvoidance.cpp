#include "AI/AlsasuaNavAvoidance.h"
#include "Core/AlsasuaSpatialGrid.h"
#include "GameFramework/Character.h"

UAlsasuaNavAvoidance::UAlsasuaNavAvoidance()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAlsasuaNavAvoidance::DetectConflictAgencies(TArray<AActor*>& OutConflicts)
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	UWorld* W = GetWorld();
	if (!W) return;

	UAlsasuaSpatialGrid* Grid = W->GetSubsystem<UAlsasuaSpatialGrid>();
	if (!Grid) return;

	TArray<AActor*> Nearby;
	Grid->GetNearbyActors(Owner->GetActorLocation(), AgentRadius * 3.f, Nearby);

	for (AActor* Other : Nearby)
	{
		if (Other == Owner) continue;

		FVector ToOther = Other->GetActorLocation() - Owner->GetActorLocation();
		float Distance = ToOther.Size();
		if (Distance > AgentRadius * 2.f) continue;

		FVector RelativeVelocity = Owner->GetVelocity() - Other->GetVelocity();
		if (FVector::DotProduct(RelativeVelocity, ToOther) > 0)
		{
			OutConflicts.Add(Other);
		}
	}
}

FVector UAlsasuaNavAvoidance::GetSteeringAdjustment(const FVector& DesiredVelocity)
{
	AActor* Owner = GetOwner();
	if (!Owner || DesiredVelocity.IsNearlyZero()) return FVector::ZeroVector;

	UWorld* W = GetWorld();
	if (!W) return FVector::ZeroVector;

	UAlsasuaSpatialGrid* Grid = W->GetSubsystem<UAlsasuaSpatialGrid>();
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
