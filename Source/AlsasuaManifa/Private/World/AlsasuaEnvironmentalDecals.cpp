#include "World/AlsasuaEnvironmentalDecals.h"
#include "Components/DecalComponent.h"
#include "Engine/World.h"

UAlsasuaEnvironmentalDecals::UAlsasuaEnvironmentalDecals()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAlsasuaEnvironmentalDecals::BeginPlay()
{
	Super::BeginPlay();
	SpawnEnvironmentalDecals();
}

void UAlsasuaEnvironmentalDecals::SpawnEnvironmentalDecals()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	const FVector Origin = Owner->GetActorLocation();
	const FVector Forward = Owner->GetActorForwardVector();
	const FRotator Rotation = Owner->GetActorRotation();

	// --- Graffiti on walls ---
	if (bEnableGraffiti && FMath::FRand() < GraffitiProbability)
	{
		UDecalComponent* Decal = NewObject<UDecalComponent>(Owner);
		if (Decal)
		{
			Decal->SetupAttachment(Owner->GetRootComponent());
			Decal->SetRelativeLocation(FVector(
				FMath::RandRange(-200.f, 200.f),
				FMath::RandRange(-200.f, 200.f),
				FMath::RandRange(50.f, 200.f)));
			Decal->SetRelativeRotation(FRotator(0, FMath::RandRange(0.f, 360.f), 0));

			Decal->DecalSize = FVector(GraffitiSize, GraffitiSize, 10.f);
			Decal->SetFadeScreenSize(0.01f);
			Decal->RegisterComponent();
			SpawnedDecals.Add(Decal);
		}
	}

	// --- Posters on walls ---
	if (bEnablePosters && FMath::FRand() < PosterProbability)
	{
		UDecalComponent* Decal = NewObject<UDecalComponent>(Owner);
		if (Decal)
		{
			Decal->SetupAttachment(Owner->GetRootComponent());
			Decal->SetRelativeLocation(FVector(
				FMath::RandRange(-150.f, 150.f),
				FMath::RandRange(-150.f, 150.f),
				FMath::RandRange(80.f, 180.f)));
			Decal->SetRelativeRotation(FRotator(0, FMath::RandRange(0.f, 360.f), 0));

			Decal->DecalSize = FVector(PosterSize, PosterSize, 5.f);
			Decal->SetFadeScreenSize(0.01f);
			Decal->RegisterComponent();
			SpawnedDecals.Add(Decal);
		}
	}

	// --- Ground trash ---
	if (bEnableTrash && FMath::FRand() < TrashProbability)
	{
		const int32 NumTrash = FMath::RandRange(1, 5);
		for (int32 i = 0; i < NumTrash; ++i)
		{
			UDecalComponent* Decal = NewObject<UDecalComponent>(Owner);
			if (Decal)
			{
				Decal->SetupAttachment(Owner->GetRootComponent());
				Decal->SetRelativeLocation(FVector(
					FMath::RandRange(-300.f, 300.f),
					FMath::RandRange(-300.f, 300.f),
					5.f));
				Decal->SetRelativeRotation(FRotator(-90.f, FMath::RandRange(0.f, 360.f), 0));

				Decal->DecalSize = FVector(TrashSize, TrashSize, 3.f);
				Decal->SetFadeScreenSize(0.01f);
				Decal->RegisterComponent();
				SpawnedDecals.Add(Decal);
			}
		}
	}

	// --- Wall cracks ---
	if (bEnableWallCracks && FMath::FRand() < WallCrackProbability)
	{
		UDecalComponent* Decal = NewObject<UDecalComponent>(Owner);
		if (Decal)
		{
			Decal->SetupAttachment(Owner->GetRootComponent());
			Decal->SetRelativeLocation(FVector(
				FMath::RandRange(-200.f, 200.f),
				FMath::RandRange(-200.f, 200.f),
				FMath::RandRange(30.f, 250.f)));
			Decal->SetRelativeRotation(FRotator(0, FMath::RandRange(0.f, 360.f), 0));

			Decal->DecalSize = FVector(WallCrackSize, WallCrackSize, 5.f);
			Decal->SetFadeScreenSize(0.01f);
			Decal->RegisterComponent();
			SpawnedDecals.Add(Decal);
		}
	}

	// --- Moss on lower walls ---
	if (bEnableMoss && FMath::FRand() < MossProbability)
	{
		UDecalComponent* Decal = NewObject<UDecalComponent>(Owner);
		if (Decal)
		{
			Decal->SetupAttachment(Owner->GetRootComponent());
			Decal->SetRelativeLocation(FVector(
				FMath::RandRange(-200.f, 200.f),
				FMath::RandRange(-200.f, 200.f),
				FMath::RandRange(0.f, 60.f)));
			Decal->SetRelativeRotation(FRotator(-90.f, FMath::RandRange(0.f, 360.f), 0));

			Decal->DecalSize = FVector(MossSize, MossSize, 8.f);
			Decal->SetFadeScreenSize(0.005f);
			Decal->RegisterComponent();
			SpawnedDecals.Add(Decal);
		}
	}
}
