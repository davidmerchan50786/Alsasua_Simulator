#include "World/AlsasuaBuildingFacadeSystem.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

UAlsasuaBuildingFacadeSystem::UAlsasuaBuildingFacadeSystem()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAlsasuaBuildingFacadeSystem::BeginPlay()
{
	Super::BeginPlay();
	SpawnFacadeElements();
}

void UAlsasuaBuildingFacadeSystem::SpawnFacadeElements()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	const FVector Origin = Owner->GetActorLocation();
	const FVector Forward = Owner->GetActorForwardVector();
	const FVector Right = Owner->GetActorRightVector();

	const float BuildingWidth = 800.f;
	const float BuildingHeight = 1200.f;
	const float FloorHeight = 300.f;
	const int32 NumFloors = FMath::RandRange(2, 4);

	for (int32 Floor = 0; Floor < NumFloors; ++Floor)
	{
		const float FloorZ = Origin.Z + (Floor + 0.5f) * FloorHeight;

		// --- Windows ---
		if (Floor > 0 || bIsResidential)
		{
			const int32 NumWindows = FMath::RandRange(2, 5);
			for (int32 w = 0; w < NumWindows; ++w)
			{
				const float WindowX = Origin.X + (w - (NumWindows - 1) * 0.5f) * WindowSpacingX * Right.X;
				const float WindowY = Origin.Y + (w - (NumWindows - 1) * 0.5f) * WindowSpacingX * Right.Y;

				// Window frame
				UBoxComponent* Frame = NewObject<UBoxComponent>(Owner);
				if (Frame)
				{
					Frame->SetupAttachment(Owner->GetRootComponent());
					Frame->SetRelativeLocation(FVector(
						WindowX - Origin.X, WindowY - Origin.Y, FloorZ - Origin.Z));
					Frame->SetBoxExtent(FVector(WindowWidth * 0.5f + WindowFrameWidth,
						WindowFrameWidth, WindowHeight * 0.5f + WindowFrameWidth));
					Frame->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					Frame->RegisterComponent();
					SpawnedElements.Add(Frame);
				}

				// Window sill
				UBoxComponent* Sill = NewObject<UBoxComponent>(Owner);
				if (Sill)
				{
					Sill->SetupAttachment(Owner->GetRootComponent());
					Sill->SetRelativeLocation(FVector(
						WindowX - Origin.X + Forward.X * WindowSillDepth * 0.5f,
						WindowY - Origin.Y + Forward.Y * WindowSillDepth * 0.5f,
						FloorZ - Origin.Z - WindowHeight * 0.5f));
					Sill->SetBoxExtent(FVector(WindowSillDepth * 0.5f,
						WindowWidth * 0.5f, 5.f));
					Sill->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					Sill->RegisterComponent();
					SpawnedElements.Add(Sill);
				}
			}
		}

		// --- Doors (ground floor) ---
		if (Floor == 0)
		{
			const int32 NumDoors = bIsCommercial ? 2 : 1;
			for (int32 d = 0; d < NumDoors; ++d)
			{
				const float DoorX = Origin.X + (d - (NumDoors - 1) * 0.5f) * DoorSpacing * Right.X;
				const float DoorY = Origin.Y + (d - (NumDoors - 1) * 0.5f) * DoorSpacing * Right.Y;

				UBoxComponent* Door = NewObject<UBoxComponent>(Owner);
				if (Door)
				{
					Door->SetupAttachment(Owner->GetRootComponent());
					Door->SetRelativeLocation(FVector(
						DoorX - Origin.X, DoorY - Origin.Y,
						FloorZ - Origin.Z - (FloorHeight - DoorHeight) * 0.5f));
					Door->SetBoxExtent(FVector(DoorWidth * 0.5f + DoorFrameWidth,
						DoorFrameWidth, DoorHeight * 0.5f));
					Door->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					Door->RegisterComponent();
					SpawnedElements.Add(Door);
				}
			}
		}

		// --- Shop Fronts (ground floor, commercial) ---
		if (Floor == 0 && bIsCommercial)
		{
			UBoxComponent* ShopFront = NewObject<UBoxComponent>(Owner);
			if (ShopFront)
			{
				ShopFront->SetupAttachment(Owner->GetRootComponent());
				ShopFront->SetRelativeLocation(FVector(
					0, 0, FloorZ - Origin.Z - (FloorHeight - ShopFrontHeight) * 0.5f));
				ShopFront->SetBoxExtent(FVector(ShopFrontWidth * 0.5f,
					10.f, ShopFrontHeight * 0.5f));
				ShopFront->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				ShopFront->RegisterComponent();
				SpawnedElements.Add(ShopFront);
			}

			// Shop sign
			UBoxComponent* Sign = NewObject<UBoxComponent>(Owner);
			if (Sign)
			{
				Sign->SetupAttachment(Owner->GetRootComponent());
				Sign->SetRelativeLocation(FVector(
					0, 0, FloorZ - Origin.Z + ShopFrontHeight * 0.5f + ShopSignHeight * 0.5f));
				Sign->SetBoxExtent(FVector(ShopFrontWidth * 0.5f,
					5.f, ShopSignHeight * 0.5f));
				Sign->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Sign->RegisterComponent();
				SpawnedElements.Add(Sign);
			}
		}

		// --- Balconies (residential upper floors) ---
		if (Floor > 0 && bIsResidential && FMath::FRand() < 0.4f)
		{
			const float BalconyX = Origin.X + FMath::RandRange(-BuildingWidth * 0.3f, BuildingWidth * 0.3f) * Right.X;
			const float BalconyY = Origin.Y + FMath::RandRange(-BuildingWidth * 0.3f, BuildingWidth * 0.3f) * Right.Y;

			// Balcony floor
			UBoxComponent* BalconyFloor = NewObject<UBoxComponent>(Owner);
			if (BalconyFloor)
			{
				BalconyFloor->SetupAttachment(Owner->GetRootComponent());
				BalconyFloor->SetRelativeLocation(FVector(
					BalconyX - Origin.X + Forward.X * BalconyDepth * 0.5f,
					BalconyY - Origin.Y + Forward.Y * BalconyDepth * 0.5f,
					FloorZ - Origin.Z - FloorHeight * 0.5f));
				BalconyFloor->SetBoxExtent(FVector(BalconyDepth * 0.5f,
					WindowWidth, 5.f));
				BalconyFloor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				BalconyFloor->RegisterComponent();
				SpawnedElements.Add(BalconyFloor);
			}

			// Balcony rail
			UBoxComponent* Rail = NewObject<UBoxComponent>(Owner);
			if (Rail)
			{
				Rail->SetupAttachment(Owner->GetRootComponent());
				Rail->SetRelativeLocation(FVector(
					BalconyX - Origin.X + Forward.X * BalconyDepth,
					BalconyY - Origin.Y + Forward.Y * BalconyDepth,
					FloorZ - Origin.Z - FloorHeight * 0.5f + BalconyRailHeight * 0.5f));
				Rail->SetBoxExtent(FVector(5.f, WindowWidth, BalconyRailHeight * 0.5f));
				Rail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Rail->RegisterComponent();
				SpawnedElements.Add(Rail);
			}
		}
	}
}
