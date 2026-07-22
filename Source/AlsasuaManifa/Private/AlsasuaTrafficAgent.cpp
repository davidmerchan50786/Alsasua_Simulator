#include "AlsasuaTrafficAgent.h"
#include "Components/SplineComponent.h"
#include "Kismet/KismetSystemLibrary.h"

AAlsasuaTrafficAgent::AAlsasuaTrafficAgent()
{
	PrimaryActorTick.bCanEverTick = true;
	DistanceAlongSpline = 0.0f;
}

void AAlsasuaTrafficAgent::BeginPlay()
{
	Super::BeginPlay();
}

void AAlsasuaTrafficAgent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!RouteSpline) return;

	// Si hay obstáculos (manifestantes, otros coches), frenar
	if (ScanForObstacles())
	{
		return; 
	}

	DistanceAlongSpline += Speed * DeltaTime;

	// Si llega al final de la ruta, volver al inicio (bucle)
	if (DistanceAlongSpline > RouteSpline->GetSplineLength())
	{
		DistanceAlongSpline = 0.0f;
	}

	FVector NewLocation = RouteSpline->GetLocationAtDistanceAlongSpline(DistanceAlongSpline, ESplineCoordinateSpace::World);
	FRotator NewRotation = RouteSpline->GetRotationAtDistanceAlongSpline(DistanceAlongSpline, ESplineCoordinateSpace::World);

	SetActorLocationAndRotation(NewLocation, NewRotation);
}

bool AAlsasuaTrafficAgent::ScanForObstacles()
{
	FVector Start = GetActorLocation() + GetActorForwardVector() * 100.f;
	FVector End = Start + GetActorForwardVector() * 400.f; // Detectar 4 metros delante

	FHitResult Hit;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);

	// Raycast para detectar colisión con la multitud o el jugador
	bool bHit = UKismetSystemLibrary::SphereTraceSingle(
		this, Start, End, 100.f, 
		UEngineTypes::ConvertToTraceType(ECC_WorldDynamic), 
		false, ActorsToIgnore, 
		EDrawDebugTrace::None, Hit, true
	);

	return bHit;
}
