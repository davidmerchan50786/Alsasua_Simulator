#include "AlsasuaTrafficAgent.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

AAlsasuaTrafficAgent::AAlsasuaTrafficAgent()
{
	PrimaryActorTick.bCanEverTick = true;
	DistanceAlongSpline = 0.0f;
}

void AAlsasuaTrafficAgent::BeginPlay()
{
	Super::BeginPlay();

	if (!RouteSpline)
	{
		UE_LOG(LogTemp, Warning, TEXT("TrafficAgent %s: sin RouteSpline asignado"), *GetName());
		PrimaryActorTick.bCanEverTick = false;
	}
}

void AAlsasuaTrafficAgent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!RouteSpline) return;

	if (ScanForObstacles())
	{
		return;
	}

	DistanceAlongSpline += Speed * DeltaTime;

	if (DistanceAlongSpline > RouteSpline->GetSplineLength())
	{
		DistanceAlongSpline = 0.0f;
	}

	const FVector NewLocation = RouteSpline->GetLocationAtDistanceAlongSpline(
		DistanceAlongSpline, ESplineCoordinateSpace::World);
	const FRotator NewRotation = RouteSpline->GetRotationAtDistanceAlongSpline(
		DistanceAlongSpline, ESplineCoordinateSpace::World);

	SetActorLocationAndRotation(NewLocation, NewRotation);
}

bool AAlsasuaTrafficAgent::ScanForObstacles()
{
	if (!GetWorld()) return false;

	const FVector Start = GetActorLocation() + GetActorForwardVector() * 100.f;
	const FVector End = Start + GetActorForwardVector() * 400.f;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	return GetWorld()->SweepSingleByChannel(
		Hit, Start, End, FQuat::Identity,
		ECC_WorldDynamic,
		FCollisionShape::MakeSphere(100.f),
		Params
	);
}
