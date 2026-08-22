#include "World/AlsasuaRoadDecalSystem.h"
#include "Components/DecalComponent.h"
#include "Engine/World.h"

UAlsasuaRoadDecalSystem::UAlsasuaRoadDecalSystem()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAlsasuaRoadDecalSystem::BeginPlay()
{
	Super::BeginPlay();
	SpawnRoadDecals();
}

void UAlsasuaRoadDecalSystem::SpawnRoadDecals()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	const FVector Origin = Owner->GetActorLocation();
	const FRotator Rotation = Owner->GetActorRotation();

	// --- Lane Markings ---
	if (bEnableLaneMarkings)
	{
		const int32 NumMarkings = FMath::RandRange(3, 8);
		for (int32 i = 0; i < NumMarkings; ++i)
		{
			UDecalComponent* Decal = NewObject<UDecalComponent>(Owner);
			if (!Decal) continue;

			Decal->SetupAttachment(Owner->GetRootComponent());
			Decal->SetRelativeLocation(FVector(
				i * (LaneMarkingLength + LaneMarkingGap),
				FMath::RandRange(-LaneWidth * 0.5f, LaneWidth * 0.5f),
				5.f));
			Decal->SetRelativeRotation(FRotator(-90.f, 0, 0));

			Decal->DecalSize = FVector(LaneMarkingLength, LaneMarkingWidth, 2.f);
			Decal->SetFadeScreenSize(0.005f);
			Decal->RegisterComponent();
			SpawnedDecals.Add(Decal);
		}
	}

	// --- Crosswalks ---
	if (bEnableCrosswalks && FMath::FRand() < CrosswalkProbability)
	{
		const int32 NumStripes = FMath::RandRange(4, 8);
		for (int32 i = 0; i < NumStripes; ++i)
		{
			UDecalComponent* Decal = NewObject<UDecalComponent>(Owner);
			if (!Decal) continue;

			Decal->SetupAttachment(Owner->GetRootComponent());
			Decal->SetRelativeLocation(FVector(
				FMath::RandRange(-100.f, 100.f),
				i * (CrosswalkStripeWidth + CrosswalkStripeGap),
				5.f));
			Decal->SetRelativeRotation(FRotator(-90.f, 0, 0));

			Decal->DecalSize = FVector(CrosswalkStripeWidth, CrosswalkWidth, 2.f);
			Decal->SetFadeScreenSize(0.005f);
			Decal->RegisterComponent();
			SpawnedDecals.Add(Decal);
		}
	}

	// --- Speed Bumps ---
	if (bEnableSpeedBumps && FMath::FRand() < SpeedBumpProbability)
	{
		UDecalComponent* Decal = NewObject<UDecalComponent>(Owner);
		if (Decal)
		{
			Decal->SetupAttachment(Owner->GetRootComponent());
			Decal->SetRelativeLocation(FVector(0, 0, 10.f));
			Decal->SetRelativeRotation(FRotator(-90.f, 0, 0));

			Decal->DecalSize = FVector(SpeedBumpHeight, SpeedBumpWidth, 5.f);
			Decal->SetFadeScreenSize(0.005f);
			Decal->RegisterComponent();
			SpawnedDecals.Add(Decal);
		}
	}

	// --- Direction Arrows ---
	if (bEnableDirectionArrows && FMath::FRand() < ArrowProbability)
	{
		UDecalComponent* Decal = NewObject<UDecalComponent>(Owner);
		if (Decal)
		{
			Decal->SetupAttachment(Owner->GetRootComponent());
			Decal->SetRelativeLocation(FVector(
				FMath::RandRange(-50.f, 50.f),
				FMath::RandRange(-LaneWidth * 0.3f, LaneWidth * 0.3f),
				5.f));
			Decal->SetRelativeRotation(FRotator(-90.f, 0, 0));

			Decal->DecalSize = FVector(ArrowSize, ArrowSize, 2.f);
			Decal->SetFadeScreenSize(0.005f);
			Decal->RegisterComponent();
			SpawnedDecals.Add(Decal);
		}
	}

	// --- Stop Lines ---
	if (bEnableStopLines && FMath::FRand() < StopLineProbability)
	{
		UDecalComponent* Decal = NewObject<UDecalComponent>(Owner);
		if (Decal)
		{
			Decal->SetupAttachment(Owner->GetRootComponent());
			Decal->SetRelativeLocation(FVector(0, 0, 5.f));
			Decal->SetRelativeRotation(FRotator(-90.f, 0, 0));

			Decal->DecalSize = FVector(15.f, StopLineWidth, 2.f);
			Decal->SetFadeScreenSize(0.005f);
			Decal->RegisterComponent();
			SpawnedDecals.Add(Decal);
		}
	}

	// --- Road Edge ---
	if (bEnableRoadEdge)
	{
		for (float Side = -1.f; Side <= 1.f; Side += 2.f)
		{
			UDecalComponent* Decal = NewObject<UDecalComponent>(Owner);
			if (!Decal) continue;

			Decal->SetupAttachment(Owner->GetRootComponent());
			Decal->SetRelativeLocation(FVector(0, Side * LaneWidth * 0.6f, 5.f));
			Decal->SetRelativeRotation(FRotator(-90.f, 0, 0));

			Decal->DecalSize = FVector(RoadEdgeWidth, LaneWidth * 1.2f, 2.f);
			Decal->SetFadeScreenSize(0.005f);
			Decal->RegisterComponent();
			SpawnedDecals.Add(Decal);
		}
	}
}
