#include "AI/Vehicle/VehicleAIController.h"
#include "Vehicle/BaseVehicle.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

AVehicleAIController::AVehicleAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AVehicleAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (ABaseVehicle* V = Cast<ABaseVehicle>(InPawn))
	{
		MaxAISpeed = V->MaxSpeed;
	}
}

// ═══════════════════════════════════════════════════════════════════════════
//  API pública
// ═══════════════════════════════════════════════════════════════════════════
void AVehicleAIController::StartPursuit(APawn* TargetPawn)
{
	PursuitTarget = TargetPawn;
	bVehicleStopped = false;
}

void AVehicleAIController::StopPursuit()
{
	PursuitTarget = nullptr;
	StopVehicle();
}

void AVehicleAIController::SetWaypoints(const TArray<FVector>& InWaypoints)
{
	Waypoints = InWaypoints;
	CurrentWaypointIndex = 0;
	bVehicleStopped = false;
}

void AVehicleAIController::StopVehicle()
{
	bVehicleStopped = true;
	if (ABaseVehicle* V = Cast<ABaseVehicle>(GetPawn()))
	{
		V->Drive(0.f, 0.f);
	}
}

// ═══════════════════════════════════════════════════════════════════════════
//  Tick
// ═══════════════════════════════════════════════════════════════════════════
void AVehicleAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bVehicleStopped) return;

	APawn* MyPawn = GetPawn();
	if (!MyPawn) return;

	// Si hay obstáculos, frenar.
	if (ScanForObstacles())
	{
		if (ABaseVehicle* V = Cast<ABaseVehicle>(MyPawn))
		{
			V->Drive(-0.3f, 0.f); // Frenar suavemente.
		}
		return;
	}

	if (PursuitTarget && IsValid(PursuitTarget))
	{
		TickPursuit(DeltaTime);
	}
	else if (Waypoints.Num() > 0)
	{
		TickAutopilot(DeltaTime);
	}
}

// ═══════════════════════════════════════════════════════════════════════════
//  TickPursuit: persecución de un objetivo.
// ═══════════════════════════════════════════════════════════════════════════
void AVehicleAIController::TickPursuit(float DeltaTime)
{
	APawn* MyPawn = GetPawn();
	if (!MyPawn || !PursuitTarget) return;

	const FVector ToTarget = PursuitTarget->GetActorLocation() - MyPawn->GetActorLocation();
	const float Dist = ToTarget.Size();
	const FVector Dir = ToTarget.GetSafeNormal();

	const float Forward = FVector::DotProduct(MyPawn->GetActorForwardVector(), Dir);
	const float Right = FVector::CrossProduct(MyPawn->GetActorForwardVector(), Dir).Z > 0 ? 1.f : -1.f;

	// Aceleración proporcional a la distancia (frenar al acercarse).
	const float Throttle = FMath::Clamp((1000.f - Dist) / 1000.f * PursuitAggression, 0.3f, 1.f);

	if (ABaseVehicle* V = Cast<ABaseVehicle>(MyPawn))
	{
		V->Drive(Throttle, Right * 0.5f);
	}
}

// ═══════════════════════════════════════════════════════════════════════════
//  TickAutopilot: sigue una ruta de waypoints.
// ═══════════════════════════════════════════════════════════════════════════
void AVehicleAIController::TickAutopilot(float DeltaTime)
{
	APawn* MyPawn = GetPawn();
	if (!MyPawn || Waypoints.Num() == 0) return;

	const FVector CurrentWP = Waypoints[CurrentWaypointIndex];
	const FVector ToWP = CurrentWP - MyPawn->GetActorLocation();
	const float DistSq = ToWP.SizeSquared();

	// waypoint alcanzado (5m).
	if (DistSq < 250000.f)
	{
		CurrentWaypointIndex = (CurrentWaypointIndex + 1) % Waypoints.Num();
		return;
	}

	const FVector Dir = ToWP.GetSafeNormal();
	const float Forward = FVector::DotProduct(MyPawn->GetActorForwardVector(), Dir);
	const float Right = FVector::CrossProduct(MyPawn->GetActorForwardVector(), Dir).Z > 0 ? 1.f : -1.f;

	// Throttle: mantener velocidad constante, frenar en curvas.
	const float CurveFactor = FMath::Clamp(Forward, 0.3f, 1.0f);
	const float Throttle = 0.7f * CurveFactor;

	if (ABaseVehicle* V = Cast<ABaseVehicle>(MyPawn))
	{
		V->Drive(Throttle, Right * 0.4f);
	}
}

// ═══════════════════════════════════════════════════════════════════════════
//  ScanForObstacles: sphere trace frontal.
// ═══════════════════════════════════════════════════════════════════════════
bool AVehicleAIController::ScanForObstacles() const
{
	const APawn* MyPawn = GetPawn();
	if (!MyPawn || !GetWorld()) return false;

	const FVector Start = MyPawn->GetActorLocation() + MyPawn->GetActorForwardVector() * 100.f;
	const FVector End = Start + MyPawn->GetActorForwardVector() * ObstacleStopDistance;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(MyPawn);

	return GetWorld()->SweepSingleByChannel(
		Hit, Start, End, FQuat::Identity,
		ECC_WorldDynamic,
		FCollisionShape::MakeSphere(ObstacleDetectionRadius),
		Params
	);
}
