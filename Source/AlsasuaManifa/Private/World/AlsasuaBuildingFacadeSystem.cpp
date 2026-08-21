#include "World/AlsasuaBuildingFacadeSystem.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"

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

	UWorld* World = GetWorld();
	if (!World) return;

	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (!CubeMesh || !PlaneMesh) return;

	const FVector Origin = Owner->GetActorLocation();
	const FVector Forward = Owner->GetActorForwardVector();
	const FVector Right = Owner->GetActorRightVector();
	const FRotator Rotation = Owner->GetActorRotation();

	const float BuildingWidth = 800.f;
	const float FloorHeight = 300.f;
	const int32 NumFloors = FMath::RandRange(2, 4);

	UMaterialInterface* WhiteMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	auto SpawnCube = [&](const FString& Label, const FVector& Loc, const FVector& Scale, const FLinearColor& Col) -> AStaticMeshActor*
	{
		AStaticMeshActor* Act = World->SpawnActor<AStaticMeshActor>(Loc, Rotation);
		if (!Act || !Act->GetStaticMeshComponent()) return nullptr;
		Act->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
		Act->GetStaticMeshComponent()->SetWorldScale3D(Scale);
		Act->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		UMaterialInstanceDynamic* Dyn = Act->GetStaticMeshComponent()->CreateDynamicMaterialInstance(0);
		if (Dyn) Dyn->SetVectorParameterValue(FName("Color"), Col);
		Act->SetActorLabel(*Label);
		SpawnedElements.Add(Act);
		return Act;
	};

	auto SpawnPlane = [&](const FString& Label, const FVector& Loc, const FVector& Scale, const FLinearColor& Col) -> AStaticMeshActor*
	{
		AStaticMeshActor* Act = World->SpawnActor<AStaticMeshActor>(Loc, Rotation);
		if (!Act || !Act->GetStaticMeshComponent()) return nullptr;
		Act->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);
		Act->GetStaticMeshComponent()->SetWorldScale3D(Scale);
		Act->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		UMaterialInstanceDynamic* Dyn = Act->GetStaticMeshComponent()->CreateDynamicMaterialInstance(0);
		if (Dyn) Dyn->SetVectorParameterValue(FName("Color"), Col);
		Act->SetActorLabel(*Label);
		SpawnedElements.Add(Act);
		return Act;
	};

	for (int32 Floor = 0; Floor < NumFloors; ++Floor)
	{
		const float FloorZ = Origin.Z + (Floor + 0.5f) * FloorHeight;

		if (Floor > 0 || bIsResidential)
		{
			const int32 NumWindows = FMath::RandRange(2, 5);
			for (int32 w = 0; w < NumWindows; ++w)
			{
				const float OffX = (w - (NumWindows - 1) * 0.5f) * WindowSpacingX;
				const FVector WinLoc = FVector(Origin.X + OffX * Right.X, Origin.Y + OffX * Right.Y, FloorZ);

				// Window frame (dark gray)
				SpawnCube(*FString::Printf(TEXT("Frame_%d_%d"), Floor, w),
					WinLoc,
					FVector(WindowWidth * 0.005f + WindowFrameWidth * 0.01f, 0.05f, WindowHeight * 0.005f + WindowFrameWidth * 0.01f),
					FLinearColor(0.2f, 0.2f, 0.22f));

				// Window sill (light gray)
				SpawnCube(*FString::Printf(TEXT("Sill_%d_%d"), Floor, w),
					WinLoc + Forward * WindowSillDepth * 0.5f - FVector(0, 0, WindowHeight * 0.5f),
					FVector(0.08f, WindowWidth * 0.005f, 0.05f),
					FLinearColor(0.6f, 0.6f, 0.6f));
			}
		}

		if (Floor == 0)
		{
			const int32 NumDoors = bIsCommercial ? 2 : 1;
			for (int32 d = 0; d < NumDoors; ++d)
			{
				const float OffX = (d - (NumDoors - 1) * 0.5f) * DoorSpacing;
				const FVector DoorLoc = FVector(Origin.X + OffX * Right.X, Origin.Y + OffX * Right.Y,
					FloorZ - (FloorHeight - DoorHeight) * 0.5f);

				SpawnCube(*FString::Printf(TEXT("Door_%d"), d),
					DoorLoc,
					FVector(DoorWidth * 0.005f, DoorFrameWidth * 0.01f, DoorHeight * 0.005f),
					FLinearColor(0.35f, 0.22f, 0.12f));
			}
		}

		if (Floor == 0 && bIsCommercial)
		{
			// Shop front (glass)
			SpawnCube(*FString::Printf(TEXT("ShopFront_%d"), SpawnedElements.Num()),
				FVector(Origin.X, Origin.Y, FloorZ - (FloorHeight - ShopFrontHeight) * 0.5f),
				FVector(ShopFrontWidth * 0.005f, 0.1f, ShopFrontHeight * 0.005f),
				FLinearColor(0.3f, 0.5f, 0.7f));

			// Shop sign (white)
			SpawnPlane(*FString::Printf(TEXT("Sign_%d"), SpawnedElements.Num()),
				FVector(Origin.X, Origin.Y, FloorZ + ShopFrontHeight * 0.5f + ShopSignHeight * 0.5f),
				FVector(ShopFrontWidth * 0.005f, 0.1f, ShopSignHeight * 0.005f),
				FLinearColor(0.95f, 0.95f, 0.9f));
		}

		if (Floor > 0 && bIsResidential && FMath::FRand() < 0.4f)
		{
			const float OffX = FMath::RandRange(-BuildingWidth * 0.3f, BuildingWidth * 0.3f);
			const FVector BalconyCenter = FVector(Origin.X + OffX * Right.X, Origin.Y + OffX * Right.Y, FloorZ - FloorHeight * 0.5f);

			// Balcony floor (dark)
			SpawnCube(*FString::Printf(TEXT("BalconFloor_%d"), SpawnedElements.Num()),
				BalconyCenter + Forward * BalconyDepth * 0.5f,
				FVector(BalconyDepth * 0.005f, WindowWidth * 0.005f, 0.05f),
				FLinearColor(0.3f, 0.3f, 0.32f));

			// Balcony rail (metal)
			SpawnCube(*FString::Printf(TEXT("BalconRail_%d"), SpawnedElements.Num()),
				BalconyCenter + Forward * BalconyDepth + FVector(0, 0, BalconyRailHeight * 0.5f),
				FVector(0.05f, WindowWidth * 0.005f, BalconyRailHeight * 0.005f),
				FLinearColor(0.15f, 0.15f, 0.18f));
		}
	}
}
