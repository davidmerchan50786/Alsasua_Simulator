#include "Environment/VegetationSpawnerSubsystem.h"
#include "Engine/World.h"
#include "Landscape.h"
#include "LandscapeInfo.h"
#include "LandscapeDataAccess.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"

void UVegetationSpawnerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UVegetationSpawnerSubsystem::Deinitialize()
{
	LandscapeCells.Empty();
	Super::Deinitialize();
}

void UVegetationSpawnerSubsystem::SetTargetLandscape(ALandscape* InLandscape)
{
	TargetLandscape = InLandscape;
}

void UVegetationSpawnerSubsystem::SpawnAllVegetation()
{
	for (const auto& Veg : VegetationTypes)
	{
		if (Veg && Veg->bEnabled)
		{
			SpawnVegetationType(Veg);
		}
	}
}

void UVegetationSpawnerSubsystem::SpawnVegetationType(UVegetationType* Vegetation)
{
	if (!Vegetation || !Vegetation->bEnabled || !TargetLandscape.IsValid())
	{
		return;
	}

	if (Vegetation->bCollisionCheck)
	{
		RebuildCollisionCache();
	}

	Vegetation->InstanceCount = 0;

	FBox RegionBounds = TargetLandscape->GetComponentsBoundingBox();

	TArray<FTransform> SpawnPoints = GeneratePoissonDiscPoints(Vegetation, RegionBounds);

	SpawnInstances(Vegetation, SpawnPoints);
}

void UVegetationSpawnerSubsystem::ClearAllVegetation()
{
	for (const auto& Veg : VegetationTypes)
	{
		if (Veg)
		{
			Veg->InstanceCount = 0;
		}
	}
}

TArray<FTransform> UVegetationSpawnerSubsystem::GeneratePoissonDiscPoints(UVegetationType* Vegetation, const FBox& RegionBounds)
{
	TArray<FTransform> Results;

	if (!Vegetation || !RegionBounds.IsValid)
	{
		return Results;
	}

	float Radius = Vegetation->MinDistance;
	if (Radius <= 0.0f)
	{
		Radius = 5.0f;
	}

	float CellSize2D = Radius / FMath::Sqrt((float)PoissonDimensions);

	FVector BoxSize = RegionBounds.GetSize();
	int32 XCells = FMath::CeilToInt(BoxSize.X / CellSize2D);
	int32 ZCells = FMath::CeilToInt(BoxSize.Z / CellSize2D);

	TArray<int32> Grid;
	Grid.SetNumZeroed(XCells * ZCells);

	TArray<FVector2D> Samples;
	TArray<FVector2D> Points;

	int32 CombinedSeed = GlobalSeed + Vegetation->Seed;
	FRandomStream Rng(CombinedSeed);

	FVector2D RandomStart(
		Rng.FRand() * BoxSize.X,
		Rng.FRand() * BoxSize.Z
	);
	Samples.Add(RandomStart);

	while (Samples.Num() > 0)
	{
		int32 SampleIdx = Rng.RandRange(0, Samples.Num() - 1);
		FVector2D SampleCenter = Samples[SampleIdx];

		bool bValid = false;

		for (int32 Attempt = 0; Attempt < PoissonMaxAttempts; Attempt++)
		{
			FRandomStream AttemptRng(CombinedSeed + Attempt + SampleIdx);

			float Angle = 2.0f * PI * AttemptRng.FRand();
			FVector2D Dir(FMath::Cos(Angle), FMath::Sin(Angle));
			float Dist = AttemptRng.FRandRange(Radius, Radius * 2.0f);
			FVector2D Sample = SampleCenter + Dir * Dist;

			if (Sample.X < 0.0f || Sample.X >= BoxSize.X || Sample.Y < 0.0f || Sample.Y >= BoxSize.Z)
			{
				continue;
			}

			int32 GX = FMath::FloorToInt(Sample.X / CellSize2D);
			int32 GY = FMath::FloorToInt(Sample.Y / CellSize2D);

			int32 XMin = FMath::Max(GX - 2, 0);
			int32 XMax = FMath::Min(GX + 2, XCells - 1);
			int32 YMin = FMath::Max(GY - 2, 0);
			int32 YMax = FMath::Min(GY + 2, ZCells - 1);

			bool bTooClose = false;
			for (int32 Y = YMin; Y <= YMax; Y++)
			{
				for (int32 X = XMin; X <= XMax; X++)
				{
					int32 Idx = Grid[Y * XCells + X];
					if (Idx > 0)
					{
						int32 PtIdx = Idx - 1;
						if (PtIdx < Points.Num())
						{
							FVector2D Diff = Sample - Points[PtIdx];
							if (Diff.SizeSquared() < Radius * Radius)
							{
								bTooClose = true;
								break;
							}
						}
					}
				}
				if (bTooClose) break;
			}

			if (bTooClose) continue;

			float WorldX = RegionBounds.Min.X + Sample.X;
			float WorldZ = RegionBounds.Min.Y + Sample.Y;

			FVector WorldPos(WorldX, WorldZ, 0.0f);

			float Height = 0.0f, WorldHeight = 0.0f, NormalizedHeight = 0.0f;
			SampleLandscapeHeight(WorldPos, Height, WorldHeight, NormalizedHeight);

			WorldPos.Z = WorldHeight;

			FRandomStream ProbRng(Vegetation->Seed + (int32)Sample.X * (int32)Sample.Y);

			if ((ProbRng.FRand() * 100.0f) > Vegetation->GlobalProbability)
			{
				continue;
			}

			if (Vegetation->bCollisionCheck && IsInsideOccupiedCell(WorldPos))
			{
				continue;
			}

			if (Vegetation->bRejectUnderwater && WorldHeight < WaterHeight)
			{
				continue;
			}

			if (WorldHeight < Vegetation->HeightRange.X || WorldHeight > Vegetation->HeightRange.Y)
			{
				continue;
			}

			if (Vegetation->SlopeRange.X > 0.0f || Vegetation->SlopeRange.Y < 90.0f)
			{
				float Slope = SampleLandscapeSlope(WorldPos);
				if (Slope < Vegetation->SlopeRange.X || Slope > Vegetation->SlopeRange.Y)
				{
					continue;
				}
			}

			float SpawnChance = 0.0f;
			if (Vegetation->LayerMasks.Num() > 0)
			{
				FVector2D NormalizedPos = GetNormalizedPosition(WorldPos);
				if (!TestSplatmap(NormalizedPos, Vegetation, SpawnChance))
				{
					continue;
				}
			}
			else
			{
				SpawnChance = 100.0f;
			}

			FRandomStream FinalRng((int32)Sample.X * (int32)Sample.Y);
			if ((FinalRng.FRand() * 100.0f) > SpawnChance)
			{
				continue;
			}

			FTransform SpawnTransform;
			SpawnTransform.SetLocation(WorldPos);

			if (Vegetation->bRandomRotation)
			{
				FRandomStream RotRng(CombinedSeed + (int32)Sample.X * (int32)Sample.Y);
				SpawnTransform.SetRotation(FRotator(0.0f, RotRng.FRandRange(0.0f, 360.0f), 0.0f).Quaternion());
			}

			float Scale = FMath::RandRange(Vegetation->ScaleRange.X, Vegetation->ScaleRange.Y);
			SpawnTransform.SetScale3D(FVector(Scale));

			Results.Add(SpawnTransform);
			Vegetation->InstanceCount++;

			int32 PtIdx = Points.Add(Sample);
			Samples.Add(Sample);
			Grid[GY * XCells + GX] = PtIdx + 1;

			bValid = true;
			break;
		}

		if (!bValid)
		{
			Samples.RemoveAt(SampleIdx);
		}
	}

	return Results;
}

void UVegetationSpawnerSubsystem::RebuildCollisionCache()
{
	if (!TargetLandscape.IsValid())
	{
		return;
	}

	LandscapeCells.Empty();

	FBox LandscapeBounds = TargetLandscape->GetComponentsBoundingBox();
	FVector Orig = TargetLandscape->GetActorLocation();
	FVector Size = LandscapeBounds.GetSize();

	int32 XCount = FMath::CeilToInt(Size.X / CellSize);
	int32 ZCount = FMath::CeilToInt(Size.Z / CellSize);

	TArray<FCellCollisionData> CellGrid;
	CellGrid.SetNum(XCount * ZCount);

	FCollisionQueryParams TraceParams(FName(TEXT("VegetationCollision")), true);
	TraceParams.bTraceComplex = true;

	for (int32 X = 0; X < XCount; X++)
	{
		for (int32 Z = 0; Z < ZCount; Z++)
		{
			FVector CellCenter(
				Orig.X + (X * CellSize) + (CellSize * 0.5f),
				Orig.Y + (Z * CellSize) + (CellSize * 0.5f),
				0.0f
			);

			float H = 0.0f, WH = 0.0f, NH = 0.0f;
			SampleLandscapeHeight(CellCenter, H, WH, NH);
			CellCenter.Z = WH;

			FCellCollisionData Cell;
			Cell.Bounds = FBox(CellCenter - FVector(CellSize * 0.5f, CellSize * 0.5f, 0.0f),
							   CellCenter + FVector(CellSize * 0.5f, CellSize * 0.5f, 0.0f));
			Cell.CellSize = CellSize;
			Cell.SubDivisions = FMath::Max(1, CellDivisions);

			int32 SubCount = Cell.SubDivisions * Cell.SubDivisions;
			Cell.SubCells.SetNum(SubCount);

			float SubSize = CellSize / Cell.SubDivisions;

			for (int32 SX = 0; SX < Cell.SubDivisions; SX++)
			{
				for (int32 SZ = 0; SZ < Cell.SubDivisions; SZ++)
				{
					FVector SubCenter(
						Cell.Bounds.Min.X + (SX * SubSize) + SubSize * 0.5f,
						Cell.Bounds.Min.Y + (SZ * SubSize) + SubSize * 0.5f,
						WH
					);

					bool bOccupied = false;

					if (bHighPrecisionCollision)
					{
						float HalfSub = SubSize * 0.5f;
						FVector Corners[4] = {
							FVector(SubCenter.X - HalfSub, SubCenter.Y - HalfSub, SubCenter.Z),
							FVector(SubCenter.X - HalfSub, SubCenter.Y + HalfSub, SubCenter.Z),
							FVector(SubCenter.X + HalfSub, SubCenter.Y - HalfSub, SubCenter.Z),
							FVector(SubCenter.X + HalfSub, SubCenter.Y + HalfSub, SubCenter.Z)
						};

						int32 HitCount = 0;
						FHitResult Hit;
						for (int32 C = 0; C < 4; C++)
						{
							FVector Start = Corners[C] + FVector(0.0f, 0.0f, 100.0f);
							FVector End = Start - FVector(0.0f, 0.0f, 250.0f);
							if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, TraceParams))
							{
								if (Hit.GetComponent() && Hit.GetComponent()->IsA<ULandscapeHeightfieldCollisionComponent>())
								{
									// Hit landscape itself, not a blocker
								}
								else
								{
									HitCount++;
								}
							}
						}
						bOccupied = (HitCount > 0);
					}
					else
					{
						FHitResult Hit;
						FVector Start = SubCenter + FVector(0.0f, 0.0f, 100.0f);
						FVector End = Start - FVector(0.0f, 0.0f, 250.0f);
						if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, TraceParams))
						{
							bOccupied = !(Hit.GetComponent() && Hit.GetComponent()->IsA<ULandscapeHeightfieldCollisionComponent>());
						}
					}

					Cell.SubCells[SZ * Cell.SubDivisions + SX] = bOccupied;
				}
			}

			CellGrid[Z * XCount + X] = Cell;
		}
	}

	LandscapeCells.Add(TargetLandscape, CellGrid);
}

TArray<FTransform> UVegetationSpawnerSubsystem::GetSpawnTransforms(UVegetationType* Vegetation, const FBox& RegionBounds)
{
	return GeneratePoissonDiscPoints(Vegetation, RegionBounds);
}

void UVegetationSpawnerSubsystem::SpawnInstances(UVegetationType* Vegetation, const TArray<FTransform>& Transforms)
{
	if (!Vegetation || Transforms.Num() == 0)
	{
		return;
	}

	AActor* SpawnActor = nullptr;

	for (const auto& Prefab : Vegetation->Prefabs)
	{
		if (!Prefab.Mesh)
		{
			continue;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
		if (!SpawnActor)
		{
			continue;
		}

		FName CompName = FName(*FString::Printf(TEXT("HISM_%s_%s"), *Vegetation->TypeName, *Prefab.Mesh->GetName()));
		UHierarchicalInstancedStaticMeshComponent* HISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(SpawnActor, UHierarchicalInstancedStaticMeshComponent::StaticClass(), CompName);
		if (!HISM)
		{
			SpawnActor->Destroy();
			continue;
		}

		HISM->SetStaticMesh(Prefab.Mesh);
		HISM->SetMobility(EComponentMobility::Static);
		HISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		HISM->bDisallowMeshPaintPerInstance = true;
		HISM->bUseDefaultCollision = false;
		HISM->SetCanEverAffectNavigation(false);
		HISM->RegisterComponentWithWorld(GetWorld());

#if WITH_EDITOR
		SpawnActor->SetActorLabel(FString::Printf(TEXT("VegetationSpawner_%s"), *Vegetation->TypeName));
#endif

		TArray<FTransform> InstanceTransforms;
		int32 PrefabCount = Vegetation->Prefabs.Num();
		int32 TotalCount = Transforms.Num();

		for (int32 i = 0; i < TotalCount; i++)
		{
			int32 PrefabIdx = (PrefabCount > 1) ? GetPrefabIndex(Vegetation, FMath::FRand()) : 0;
			if (PrefabIdx != GetPrefabIndex(Vegetation, 0.0f))
			{
				continue;
			}

			FTransform T = Transforms[i];
			T.SetLocation(T.GetLocation() - FVector(0.0f, 0.0f, Vegetation->SinkAmount));
			InstanceTransforms.Add(T);
		}

		if (InstanceTransforms.Num() > 0)
		{
			HISM->AddInstances(InstanceTransforms, false);
		}
	}
}

void UVegetationSpawnerSubsystem::SampleLandscapeHeight(const FVector& WorldPos, float& OutHeight, float& OutWorldHeight, float& OutNormalizedHeight)
{
	OutHeight = 0.0f;
	OutWorldHeight = WorldPos.Z;
	OutNormalizedHeight = 0.0f;

	if (!TargetLandscape.IsValid())
	{
		return;
	}

	FVector LocalPos = WorldPos - TargetLandscape->GetActorLocation();
	FVector2D NormalizedPos(
		LocalPos.X / TargetLandscape->GetActorScale().X,
		LocalPos.Y / TargetLandscape->GetActorScale().Y
	);

	float HeightValue = TargetLandscape->GetHeightAtLocation(WorldPos).Get(WorldPos.Z);
	OutWorldHeight = HeightValue;
	OutHeight = HeightValue;
	OutNormalizedHeight = HeightValue / TargetLandscape->GetActorScale().Z;
}

float UVegetationSpawnerSubsystem::SampleLandscapeSlope(const FVector& WorldPos)
{
	if (!TargetLandscape.IsValid())
	{
		return 0.0f;
	}

	float Step = 50.0f;
	FVector Center = WorldPos;
	FVector Right = WorldPos + FVector(Step, 0.0f, 0.0f);
	FVector Forward = WorldPos + FVector(0.0f, Step, 0.0f);

	float HCenter = TargetLandscape->GetHeightAtLocation(Center).Get(Center.Z);
	float HRight = TargetLandscape->GetHeightAtLocation(Right).Get(Right.Z);
	float HForward = TargetLandscape->GetHeightAtLocation(Forward).Get(Forward.Z);

	float DX = (HRight - HCenter) / Step;
	float DY = (HForward - HCenter) / Step;

	float SlopeRad = FMath::Atan2(FMath::Sqrt(DX * DX + DY * DY), 1.0f);
	return FMath::RadiansToDegrees(SlopeRad);
}

float UVegetationSpawnerSubsystem::SampleLandscapeConvexity(const FVector& WorldPos, float Radius)
{
	if (!TargetLandscape.IsValid())
	{
		return 0.5f;
	}

	float Step = Radius * 50.0f;
	const FVector SamplePosX = WorldPos + FVector(Step, 0.0f, 0.0f);
	const FVector SampleNegX = WorldPos - FVector(Step, 0.0f, 0.0f);
	const FVector SamplePosY = WorldPos + FVector(0.0f, Step, 0.0f);
	const FVector SampleNegY = WorldPos - FVector(0.0f, Step, 0.0f);

	float PosX = TargetLandscape->GetHeightAtLocation(SamplePosX).Get(SamplePosX.Z);
	float NegX = TargetLandscape->GetHeightAtLocation(SampleNegX).Get(SampleNegX.Z);
	float PosY = TargetLandscape->GetHeightAtLocation(SamplePosY).Get(SamplePosY.Z);
	float NegY = TargetLandscape->GetHeightAtLocation(SampleNegY).Get(SampleNegY.Z);

	float ConvX = (PosX - NegX) + 0.5f;
	float ConvY = (PosY - NegY) + 0.5f;

	float Convexity = (ConvY < 0.5f) ? 2.0f * ConvX * ConvY : 1.0f - 2.0f * (1.0f - ConvX) * (1.0f - ConvY);

	return (Convexity - (1.0f - Convexity)) * 0.5f + 0.5f;
}

FVector2D UVegetationSpawnerSubsystem::GetNormalizedPosition(const FVector& WorldPos)
{
	if (!TargetLandscape.IsValid())
	{
		return FVector2D::ZeroVector;
	}

	FVector LocalPos = WorldPos - TargetLandscape->GetActorLocation();
	return FVector2D(
		LocalPos.X / TargetLandscape->GetActorScale().X,
		LocalPos.Y / TargetLandscape->GetActorScale().Y
	);
}

bool UVegetationSpawnerSubsystem::IsInsideOccupiedCell(const FVector& WorldPos)
{
	if (!TargetLandscape.IsValid())
	{
		return false;
	}

	const TArray<FCellCollisionData>* Cells = LandscapeCells.Find(TargetLandscape);
	if (!Cells)
	{
		return false;
	}

	FVector LocalPos = WorldPos - TargetLandscape->GetActorLocation();
	FVector Size = TargetLandscape->GetComponentsBoundingBox().GetSize();

	int32 XCount = FMath::CeilToInt(Size.X / CellSize);
	int32 ZCount = FMath::CeilToInt(Size.Z / CellSize);

	int32 CellX = FMath::FloorToInt((Size.X > 0.0f) ? (LocalPos.X / Size.X) * XCount : 0);
	int32 CellZ = FMath::FloorToInt((Size.Z > 0.0f) ? (LocalPos.Y / Size.Z) * ZCount : 0);

	CellX = FMath::Clamp(CellX, 0, XCount - 1);
	CellZ = FMath::Clamp(CellZ, 0, ZCount - 1);

	if (CellZ >= Cells->Num() || CellX >= (*Cells)[CellZ * XCount + CellX].SubCells.Num())
	{
		return false;
	}

	const FCellCollisionData& Cell = (*Cells)[CellZ * XCount + CellX];
	float SubSize = CellSize / Cell.SubDivisions;

	int32 SubX = FMath::FloorToInt((LocalPos.X - Cell.Bounds.Min.X) / SubSize);
	int32 SubZ = FMath::FloorToInt((LocalPos.Y - Cell.Bounds.Min.Y) / SubSize);
	SubX = FMath::Clamp(SubX, 0, Cell.SubDivisions - 1);
	SubZ = FMath::Clamp(SubZ, 0, Cell.SubDivisions - 1);

	return Cell.SubCells[SubZ * Cell.SubDivisions + SubX];
}

bool UVegetationSpawnerSubsystem::TestSplatmap(const FVector2D& NormalizedPos, UVegetationType* Vegetation, float& OutSpawnChance)
{
    OutSpawnChance = 0.0f;

    if (!Vegetation || !TargetLandscape.IsValid() || Vegetation->LayerMasks.Num() == 0)
    {
        return false;
    }

    ALandscapeProxy* Proxy = TargetLandscape.Get();
    if (!Proxy) return false;

    float TotalWeight = 0.0f;
    int32 ValidMasks = 0;

    for (const FVegetationLayerMask& Mask : Vegetation->LayerMasks)
    {
        float ProceduralWeight = FMath::PerlinNoise2D(FVector2D(
            NormalizedPos.X * (Mask.LayerIndex + 1) * 3.7f,
            NormalizedPos.Y * (Mask.LayerIndex + 1) * 3.7f
        )) * 0.5f + 0.5f;

        if (ProceduralWeight >= Mask.Threshold)
        {
            TotalWeight += ProceduralWeight;
            ValidMasks++;
        }
    }

    if (ValidMasks == 0)
    {
        return false;
    }

    OutSpawnChance = (TotalWeight / static_cast<float>(ValidMasks)) * 100.0f;
    return true;
}

int32 UVegetationSpawnerSubsystem::GetSplatmapID(int32 LayerID)
{
	if (LayerID > 11) return 3;
	if (LayerID > 7) return 2;
	if (LayerID > 3) return 1;
	return 0;
}

int32 UVegetationSpawnerSubsystem::GetPrefabIndex(UVegetationType* Vegetation, float RandomValue)
{
	if (!Vegetation || Vegetation->Prefabs.Num() == 0)
	{
		return 0;
	}

	float TotalWeight = 0.0f;
	for (const auto& P : Vegetation->Prefabs)
	{
		TotalWeight += P.Probability;
	}

	if (TotalWeight <= 0.0f)
	{
		return 0;
	}

	float Roll = RandomValue * TotalWeight;
	float Accum = 0.0f;
	for (int32 i = 0; i < Vegetation->Prefabs.Num(); i++)
	{
		Accum += Vegetation->Prefabs[i].Probability;
		if (Roll <= Accum)
		{
			return i;
		}
	}

	return Vegetation->Prefabs.Num() - 1;
}
