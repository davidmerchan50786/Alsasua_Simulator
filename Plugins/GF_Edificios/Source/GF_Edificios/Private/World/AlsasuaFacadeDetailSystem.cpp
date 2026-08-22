#include "World/AlsasuaFacadeDetailSystem.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"

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

	UWorld* World = GetWorld();
	if (!World) return;

	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (!CubeMesh || !CylinderMesh || !PlaneMesh) return;

	const FVector Origin = Owner->GetActorLocation();
	const FVector Forward = Owner->GetActorForwardVector();
	const FVector Right = Owner->GetActorRightVector();
	const FRotator Rotation = Owner->GetActorRotation();

	const float BuildingWidth = 800.f;
	const float FloorHeight = 300.f;
	const int32 NumFloors = FMath::RandRange(2, 4);

	for (int32 Floor = 0; Floor < NumFloors; ++Floor)
	{
		const float FloorZ = Origin.Z + (Floor + 0.5f) * FloorHeight;

		if (bEnableBalconies && FMath::FRand() < BalconyProbability)
		{
			const float OffX = FMath::RandRange(-BuildingWidth * 0.3f, BuildingWidth * 0.3f);
			const FVector Loc = FVector(Origin.X + OffX * Right.X, Origin.Y + OffX * Right.Y, FloorZ);

			AStaticMeshActor* Act = World->SpawnActor<AStaticMeshActor>(Loc, Rotation);
			if (Act && Act->GetStaticMeshComponent())
			{
				Act->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
				Act->GetStaticMeshComponent()->SetWorldScale3D(FVector(BalconyWidth * 0.01f, BalconyDepth * 0.01f, BalconyHeight * 0.01f));
				Act->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Act->SetActorLabel(*FString::Printf(TEXT("Balcon_%d"), SpawnedDetailCount));
				SpawnedDetailCount++;
			}
		}

		if (bEnableShutters && FMath::FRand() < ShutterProbability)
		{
			const float OffX = FMath::RandRange(-BuildingWidth * 0.3f, BuildingWidth * 0.3f);
			const FVector Loc = FVector(Origin.X + OffX * Right.X, Origin.Y + OffX * Right.Y, FloorZ);

			const TArray<FLinearColor> Colors = { ShutterColorBrown, ShutterColorGreen, ShutterColorBlue };
			const FLinearColor ChosenColor = Colors[FMath::RandRange(0, Colors.Num() - 1)];

			AStaticMeshActor* Act = World->SpawnActor<AStaticMeshActor>(Loc, Rotation);
			if (Act && Act->GetStaticMeshComponent())
			{
				Act->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
				Act->GetStaticMeshComponent()->SetWorldScale3D(FVector(ShutterWidth * 0.01f, 0.05f, ShutterHeight * 0.01f));
				Act->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

				UMaterialInstanceDynamic* Dyn = Act->GetStaticMeshComponent()->CreateDynamicMaterialInstance(0);
				if (Dyn) Dyn->SetVectorParameterValue(FName("Color"), ChosenColor);

				Act->SetActorLabel(*FString::Printf(TEXT("Shutter_%d"), SpawnedDetailCount));
				SpawnedDetailCount++;
			}
		}

		if (bEnableFlowerPots && FMath::FRand() < FlowerPotProbability)
		{
			const float OffX = FMath::RandRange(-BuildingWidth * 0.3f, BuildingWidth * 0.3f);
			const FVector Loc = FVector(Origin.X + OffX * Right.X, Origin.Y + OffX * Right.Y, FloorZ + BalconyHeight * 0.01f);

			const TArray<FLinearColor> FlowerColors = { FlowerColor1, FlowerColor2, FlowerColor3 };
			const FLinearColor ChosenColor = FlowerColors[FMath::RandRange(0, FlowerColors.Num() - 1)];

			AStaticMeshActor* Act = World->SpawnActor<AStaticMeshActor>(Loc, Rotation);
			if (Act && Act->GetStaticMeshComponent())
			{
				Act->GetStaticMeshComponent()->SetStaticMesh(CylinderMesh);
				Act->GetStaticMeshComponent()->SetWorldScale3D(FVector(FlowerPotSize * 0.005f, FlowerPotSize * 0.005f, FlowerPotSize * 0.01f));
				Act->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

				UMaterialInstanceDynamic* Dyn = Act->GetStaticMeshComponent()->CreateDynamicMaterialInstance(0);
				if (Dyn) Dyn->SetVectorParameterValue(FName("Color"), ChosenColor);

				Act->SetActorLabel(*FString::Printf(TEXT("FlowerPot_%d"), SpawnedDetailCount));
				SpawnedDetailCount++;
			}
		}

		if (bEnableAwnings && FMath::FRand() < AwningProbability)
		{
			const float OffX = FMath::RandRange(-BuildingWidth * 0.2f, BuildingWidth * 0.2f);
			const FVector Loc = FVector(
				Origin.X + OffX * Right.X + Forward.X * AwningProjection * 0.5f,
				Origin.Y + OffX * Right.Y + Forward.Y * AwningProjection * 0.5f,
				FloorZ + BalconyHeight * 0.01f + 0.2f);

			AStaticMeshActor* Act = World->SpawnActor<AStaticMeshActor>(Loc, Rotation);
			if (Act && Act->GetStaticMeshComponent())
			{
				Act->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);
				Act->GetStaticMeshComponent()->SetWorldScale3D(FVector(AwningProjection * 0.01f, AwningWidth * 0.01f, 1.f));
				Act->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

				const FLinearColor ChosenColor = (FMath::Rand() > 0.5f) ? AwningColorRed : AwningColorStripe;
				UMaterialInstanceDynamic* Dyn = Act->GetStaticMeshComponent()->CreateDynamicMaterialInstance(0);
				if (Dyn) Dyn->SetVectorParameterValue(FName("Color"), ChosenColor);

				Act->SetActorLabel(*FString::Printf(TEXT("Awning_%d"), SpawnedDetailCount));
				SpawnedDetailCount++;
			}
		}

		if (bEnableACUnits && FMath::FRand() < ACProbability)
		{
			const float OffX = FMath::RandRange(-BuildingWidth * 0.3f, BuildingWidth * 0.3f);
			const FVector Loc = FVector(Origin.X + OffX * Right.X, Origin.Y + OffX * Right.Y, FloorZ + BalconyHeight * 0.01f - 0.1f);

			AStaticMeshActor* Act = World->SpawnActor<AStaticMeshActor>(Loc, Rotation);
			if (Act && Act->GetStaticMeshComponent())
			{
				Act->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
				Act->GetStaticMeshComponent()->SetWorldScale3D(FVector(ACSize * 0.01f, ACSize * 0.005f, ACSize * 0.005f));
				Act->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

				UMaterialInstanceDynamic* Dyn = Act->GetStaticMeshComponent()->CreateDynamicMaterialInstance(0);
				if (Dyn) Dyn->SetVectorParameterValue(FName("Color"), FLinearColor(0.7f, 0.7f, 0.72f));

				Act->SetActorLabel(*FString::Printf(TEXT("AC_%d"), SpawnedDetailCount));
				SpawnedDetailCount++;
			}
		}
	}
}
