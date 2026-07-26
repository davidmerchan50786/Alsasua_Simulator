#include "World/AlsasuaGrassDetailMesh.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "Engine/World.h"
#include "GeoDataAlsasua.h"

UAlsasuaGrassDetailMesh::UAlsasuaGrassDetailMesh()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAlsasuaGrassDetailMesh::BeginPlay()
{
	Super::BeginPlay();
	SpawnGrassPatch();
}

void UAlsasuaGrassDetailMesh::SpawnGrassPatch()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	UProceduralMeshComponent* ProcMesh = Owner->FindComponentByClass<UProceduralMeshComponent>();
	if (!ProcMesh)
	{
		ProcMesh = NewObject<UProceduralMeshComponent>(Owner);
		if (ProcMesh)
		{
			ProcMesh->SetupAttachment(Owner->GetRootComponent());
			ProcMesh->RegisterComponent();
		}
	}

	if (!ProcMesh) return;

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> VertexColors;

	const int32 NumBlades = FMath::RandRange(50, 150);

	for (int32 i = 0; i < NumBlades; ++i)
	{
		const float Angle = FMath::RandRange(0.f, 360.f);
		const float Radius = FMath::RandRange(MinSpawnRadius, MaxSpawnRadius);

		const FVector BladeOrigin = Owner->GetActorLocation() + FVector(
			FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
			FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
			0.f);

		const float BladeHeight = GrassBladeHeight * FMath::RandRange(1.f - GrassHeightVariation, 1.f + GrassHeightVariation);
		const float BladeWidth = GrassBladeWidth * FMath::RandRange(0.8f, 1.2f);

		const int32 BaseVertex = Vertices.Num();

		// 4 vertices per blade (quad)
		Vertices.Add(BladeOrigin + FVector(-BladeWidth * 0.5f, 0, 0));
		Vertices.Add(BladeOrigin + FVector(BladeWidth * 0.5f, 0, 0));
		Vertices.Add(BladeOrigin + FVector(BladeWidth * 0.25f, 0, BladeHeight * 0.6f));
		Vertices.Add(BladeOrigin + FVector(-BladeWidth * 0.25f, 0, BladeHeight * 0.6f));

		// Tip vertices
		Vertices.Add(BladeOrigin + FVector(0, 0, BladeHeight));

		Normals.Add(FVector::UpVector);
		Normals.Add(FVector::UpVector);
		Normals.Add(FVector::UpVector);
		Normals.Add(FVector::UpVector);
		Normals.Add(FVector::UpVector);

		UVs.Add(FVector2D(0, 0));
		UVs.Add(FVector2D(1, 0));
		UVs.Add(FVector2D(1, 0.6f));
		UVs.Add(FVector2D(0, 0.6f));
		UVs.Add(FVector2D(0.5f, 1.f));

		const float ColorVar = FMath::RandRange(-ColorVariation, ColorVariation);
		const FLinearColor Base = FLinearColor(
			GrassBaseColor.R + ColorVar,
			GrassBaseColor.G + ColorVar,
			GrassBaseColor.B + ColorVar);

		const FLinearColor Tip = FLinearColor(
			GrassTipColor.R + ColorVar,
			GrassTipColor.G + ColorVar,
			GrassTipColor.B + ColorVar);

		VertexColors.Add(Base.ToFColor(true));
		VertexColors.Add(Base.ToFColor(true));
		VertexColors.Add(FMath::Lerp(Base, Tip, 0.6f).ToFColor(true));
		VertexColors.Add(FMath::Lerp(Base, Tip, 0.6f).ToFColor(true));
		VertexColors.Add(Tip.ToFColor(true));

		// Two triangles per blade
		Triangles.Add(BaseVertex + 0);
		Triangles.Add(BaseVertex + 1);
		Triangles.Add(BaseVertex + 2);

		Triangles.Add(BaseVertex + 0);
		Triangles.Add(BaseVertex + 2);
		Triangles.Add(BaseVertex + 3);

		// Upper triangle
		Triangles.Add(BaseVertex + 3);
		Triangles.Add(BaseVertex + 2);
		Triangles.Add(BaseVertex + 4);

		SpawnedBlades++;
	}

	ProcMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs,
		TArray<FLinearColor>(), TArray<FProcMeshTangent>(), true);

	ProcMesh->SetCastShadow(false);
	ProcMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
