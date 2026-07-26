#include "World/AlsasuaGroundCoverSystem.h"
#include "ProceduralMeshComponent.h"
#include "Engine/World.h"
#include "GeoDataAlsasua.h"

UAlsasuaGroundCoverSystem::UAlsasuaGroundCoverSystem()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAlsasuaGroundCoverSystem::BeginPlay()
{
	Super::BeginPlay();
	SpawnGroundCover();
}

void UAlsasuaGroundCoverSystem::SpawnGroundCover()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	GroundMesh = NewObject<UProceduralMeshComponent>(Owner);
	if (!GroundMesh) return;

	GroundMesh->SetupAttachment(Owner->GetRootComponent());
	GroundMesh->RegisterComponent();

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> VertexColors;

	const FVector Origin = Owner->GetActorLocation();
	const float SpreadRadius = 2000.f;

	// --- Fallen Leaves ---
	if (bEnableFallenLeaves)
	{
		const int32 NumLeaves = FMath::RandRange(50, MaxLeaves);
		const TArray<FLinearColor> LeafColors = { LeafColor1, LeafColor2, LeafColor3 };

		for (int32 i = 0; i < NumLeaves; ++i)
		{
			const float Angle = FMath::RandRange(0.f, 360.f);
			const float Radius = FMath::RandRange(100.f, SpreadRadius);
			const FVector LeafPos = Origin + FVector(
				FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
				FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
				5.f);

			const int32 BaseVertex = Vertices.Num();
			const float Rotation = FMath::RandRange(0.f, 360.f);
			const float CosR = FMath::Cos(FMath::DegreesToRadians(Rotation));
			const float SinR = FMath::Sin(FMath::DegreesToRadians(Rotation));

			const float HalfW = LeafSize * 0.5f;
			const float HalfH = LeafSize * 0.3f;

			Vertices.Add(LeafPos + FVector(-HalfW * CosR - HalfH * SinR, -HalfW * SinR + HalfH * CosR, 0));
			Vertices.Add(LeafPos + FVector(HalfW * CosR - HalfH * SinR, HalfW * SinR + HalfH * CosR, 0));
			Vertices.Add(LeafPos + FVector(HalfW * CosR + HalfH * SinR, HalfW * SinR - HalfH * CosR, 0));
			Vertices.Add(LeafPos + FVector(-HalfW * CosR + HalfH * SinR, -HalfW * SinR - HalfH * CosR, 0));

			for (int32 v = 0; v < 4; ++v)
			{
				Normals.Add(FVector::UpVector);
				UVs.Add(FVector2D(v == 0 || v == 3 ? 0 : 1, v < 2 ? 0 : 1));
			}

			const FLinearColor ChosenColor = LeafColors[FMath::RandRange(0, LeafColors.Num() - 1)];
			for (int32 v = 0; v < 4; ++v)
			{
				VertexColors.Add(ChosenColor.ToFColor(true));
			}

			Triangles.Add(BaseVertex + 0);
			Triangles.Add(BaseVertex + 1);
			Triangles.Add(BaseVertex + 2);
			Triangles.Add(BaseVertex + 0);
			Triangles.Add(BaseVertex + 2);
			Triangles.Add(BaseVertex + 3);

			TotalSpawned++;
		}
	}

	// --- Rocks ---
	if (bEnableRocks)
	{
		const int32 NumRocks = FMath::RandRange(20, MaxRocks);

		for (int32 i = 0; i < NumRocks; ++i)
		{
			const float Angle = FMath::RandRange(0.f, 360.f);
			const float Radius = FMath::RandRange(100.f, SpreadRadius);
			const FVector RockPos = Origin + FVector(
				FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
				FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
				5.f);

			const float RockSize = FMath::RandRange(RockMinSize, RockMaxSize);
			const int32 BaseVertex = Vertices.Num();

			// Simple hexagonal rock
			const int32 Sides = 6;
			for (int32 s = 0; s < Sides; ++s)
			{
				const float RockAngle = (360.f / Sides) * s;
				const float CosA = FMath::Cos(FMath::DegreesToRadians(RockAngle));
				const float SinA = FMath::Sin(FMath::DegreesToRadians(RockAngle));
				Vertices.Add(RockPos + FVector(CosA * RockSize, SinA * RockSize, 0));
				Normals.Add(FVector::UpVector);
				UVs.Add(FVector2D(CosA * 0.5f + 0.5f, SinA * 0.5f + 0.5f));
				VertexColors.Add(RockColor.ToFColor(true));
			}

			for (int32 s = 1; s < Sides - 1; ++s)
			{
				Triangles.Add(BaseVertex + 0);
				Triangles.Add(BaseVertex + s);
				Triangles.Add(BaseVertex + s + 1);
			}

			TotalSpawned++;
		}
	}

	// --- Moss Patches ---
	if (bEnableMossPatches)
	{
		const int32 NumMoss = FMath::RandRange(20, MaxMossPatches);

		for (int32 i = 0; i < NumMoss; ++i)
		{
			const float Angle = FMath::RandRange(0.f, 360.f);
			const float Radius = FMath::RandRange(100.f, SpreadRadius * 0.8f);
			const FVector MossPos = Origin + FVector(
				FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
				FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
				3.f);

			const float Size = MossPatchSize * FMath::RandRange(0.5f, 1.5f);
			const int32 BaseVertex = Vertices.Num();

			Vertices.Add(MossPos + FVector(-Size, -Size, 0));
			Vertices.Add(MossPos + FVector(Size, -Size, 0));
			Vertices.Add(MossPos + FVector(Size, Size, 0));
			Vertices.Add(MossPos + FVector(-Size, Size, 0));

			for (int32 v = 0; v < 4; ++v)
			{
				Normals.Add(FVector::UpVector);
				UVs.Add(FVector2D(v == 0 || v == 3 ? 0 : 1, v < 2 ? 0 : 1));
				VertexColors.Add(MossColor.ToFColor(true));
			}

			Triangles.Add(BaseVertex + 0);
			Triangles.Add(BaseVertex + 1);
			Triangles.Add(BaseVertex + 2);
			Triangles.Add(BaseVertex + 0);
			Triangles.Add(BaseVertex + 2);
			Triangles.Add(BaseVertex + 3);

			TotalSpawned++;
		}
	}

	if (Vertices.Num() > 0)
	{
		GroundMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs,
			TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
		GroundMesh->SetCastShadow(false);
		GroundMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}
