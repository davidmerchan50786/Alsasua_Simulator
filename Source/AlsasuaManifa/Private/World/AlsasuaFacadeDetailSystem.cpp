#include "World/AlsasuaFacadeDetailSystem.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

UAlsasuaFacadeDetailSystem::UAlsasuaFacadeDetailSystem()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAlsasuaFacadeDetailSystem::BeginPlay()
{
	Super::BeginPlay();
	SpawnFacadeDetails();
}

void UAlsasuaFacadeDetailSystem::SpawnFacadeDetails()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	const FVector Origin = Owner->GetActorLocation();
	const FVector Forward = Owner->GetActorForwardVector();
	const FVector Right = Owner->GetActorRightVector();
	const FRotator Rotation = Owner->GetActorRotation();

	// Approximate building facade dimensions
	const float BuildingWidth = 800.f;
	const float BuildingHeight = 1200.f;
	const float FloorHeight = 300.f;
	const int32 NumFloors = FMath::RandRange(2, 4);

	for (int32 Floor = 0; Floor < NumFloors; ++Floor)
	{
		const float FloorZ = Origin.Z + (Floor + 0.5f) * FloorHeight;

		// --- Balconies ---
		if (bEnableBalconies && FMath::FRand() < BalconyProbability)
		{
			const float BalconyX = Origin.X + FMath::RandRange(-BuildingWidth * 0.3f, BuildingWidth * 0.3f) * Right.X;
			const float BalconyY = Origin.Y + FMath::FRandRange(-BuildingWidth * 0.3f, BuildingWidth * 0.3f) * Right.Y;

			UBoxComponent* Balcony = NewObject<UBoxComponent>(Owner);
			if (Balcony)
			{
				Balcony->SetupAttachment(Owner->GetRootComponent());
				Balcony->SetRelativeLocation(FVector(
					BalconyX - Origin.X, BalconyY - Origin.Y, FloorZ - Origin.Z));
				Balcony->SetBoxExtent(FVector(BalconyDepth, BalconyWidth, BalconyHeight));
				Balcony->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Balcony->SetMobility(EComponentMobility::Movable);
				Balcony->RegisterComponent();
				SpawnedDetailCount++;
			}
		}

		// --- Shutters ---
		if (bEnableShutters && FMath::FRand() < ShutterProbability)
		{
			const float ShutterX = Origin.X + FMath::RandRange(-BuildingWidth * 0.3f, BuildingWidth * 0.3f) * Right.X;
			const float ShutterY = Origin.Y + FMath::RandRange(-BuildingWidth * 0.3f, BuildingWidth * 0.3f) * Right.Y;

			const TArray<FLinearColor> Colors = { ShutterColorBrown, ShutterColorGreen, ShutterColorBlue };
			const FLinearColor ChosenColor = Colors[FMath::RandRange(0, Colors.Num() - 1)];

			UBoxComponent* Shutter = NewObject<UBoxComponent>(Owner);
			if (Shutter)
			{
				Shutter->SetupAttachment(Owner->GetRootComponent());
				Shutter->SetRelativeLocation(FVector(
					ShutterX - Origin.X, ShutterY - Origin.Y, FloorZ - Origin.Z));
				Shutter->SetBoxExtent(FVector(ShutterWidth, 5.f, ShutterHeight));
				Shutter->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Shutter->RegisterComponent();
				SpawnedDetailCount++;
			}
		}

		// --- Flower Pots ---
		if (bEnableFlowerPots && FMath::FRand() < FlowerPotProbability)
		{
			const float PotX = Origin.X + FMath::RandRange(-BuildingWidth * 0.3f, BuildingWidth * 0.3f) * Right.X;
			const float PotY = Origin.Y + FMath::RandRange(-BuildingWidth * 0.3f, BuildingWidth * 0.3f) * Right.Y;

			const TArray<FLinearColor> FlowerColors = { FlowerColor1, FlowerColor2, FlowerColor3 };
			const FLinearColor ChosenColor = FlowerColors[FMath::RandRange(0, FlowerColors.Num() - 1)];

			UBoxComponent* Pot = NewObject<UBoxComponent>(Owner);
			if (Pot)
			{
				Pot->SetupAttachment(Owner->GetRootComponent());
				Pot->SetRelativeLocation(FVector(
					PotX - Origin.X, PotY - Origin.Y, FloorZ - Origin.Z + BalconyHeight));
				Pot->SetBoxExtent(FVector(FlowerPotSize, FlowerPotSize, FlowerPotSize));
				Pot->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Pot->RegisterComponent();
				SpawnedDetailCount++;
			}
		}

		// --- Awnings ---
		if (bEnableAwnings && FMath::FRand() < AwningProbability)
		{
			const float AwningX = Origin.X + FMath::RandRange(-BuildingWidth * 0.2f, BuildingWidth * 0.2f) * Right.X;
			const float AwningY = Origin.Y + FMath::RandRange(-BuildingWidth * 0.2f, BuildingWidth * 0.2f) * Right.Y;

			UBoxComponent* Awning = NewObject<UBoxComponent>(Owner);
			if (Awning)
			{
				Awning->SetupAttachment(Owner->GetRootComponent());
				Awning->SetRelativeLocation(FVector(
					AwningX - Origin.X + Forward.X * AwningProjection * 0.5f,
					AwningY - Origin.Y + Forward.Y * AwningProjection * 0.5f,
					FloorZ - Origin.Z + BalconyHeight + 20.f));
				Awning->SetBoxExtent(FVector(AwningProjection * 0.5f, AwningWidth * 0.5f, 5.f));
				Awning->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Awning->RegisterComponent();
				SpawnedDetailCount++;
			}
		}

		// --- AC Units ---
		if (bEnableACUnits && FMath::FRand() < ACProbability)
		{
			const float ACX = Origin.X + FMath::RandRange(-BuildingWidth * 0.3f, BuildingWidth * 0.3f) * Right.X;
			const float ACY = Origin.Y + FMath::RandRange(-BuildingWidth * 0.3f, BuildingWidth * 0.3f) * Right.Y;

			UBoxComponent* AC = NewObject<UBoxComponent>(Owner);
			if (AC)
			{
				AC->SetupAttachment(Owner->GetRootComponent());
				AC->SetRelativeLocation(FVector(
					ACX - Origin.X, ACY - Origin.Y, FloorZ - Origin.Z + BalconyHeight - 10.f));
				AC->SetBoxExtent(FVector(ACSize, ACSize * 0.5f, ACSize * 0.5f));
				AC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				AC->RegisterComponent();
				SpawnedDetailCount++;
			}
		}
	}
}
