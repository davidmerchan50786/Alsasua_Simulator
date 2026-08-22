#include "Environment/VegetationSpawnerSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Landscape.h"
#include "LandscapeInfo.h"
#include "LandscapeDataAccess.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/Package.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/Console.h"
#include "TimerManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/Package.h"
#include "Engine/AssetManager.h"
#include "Engine/World.h"
#include "Landscape.h"
#include "LandscapeInfo.h"
#include "LandscapeDataAccess.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"
#include "Engine/Engine.h"
#include "Engine/Console.h"
#include "TimerManager.h"

void UVegetationSpawnerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	PopulateVegetationTypesFromContent();
}

void UVegetationSpawnerSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	AutoDiscoverLandscapeAndSpawn();
}

static void ExecRebuildVegetation(UWorld* World)
{
	if (!World)
	{
		UE_LOG(LogTemp, Log, TEXT("VegetationSpawnerSubsystem: no world"));
		return;
	}

	if (UVegetationSpawnerSubsystem* Subsystem = World->GetSubsystem<UVegetationSpawnerSubsystem>())
	{
		Subsystem->DebugRebuildVegetation();
		UE_LOG(LogTemp, Log, TEXT("VegetationSpawnerSubsystem: rebuild requested"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("VegetationSpawnerSubsystem: subsystem not found"));
	}
}

static FAutoConsoleCommandWithWorld RebuildVegetationCommand(
	TEXT("rebuildvegetation"),
	TEXT("Rebuild vegetation from the current world"),
	FConsoleCommandWithWorldDelegate::CreateStatic(&ExecRebuildVegetation));

static void ExecForcedTestSpawn(UWorld* World)
{
	if (!World)
	{
		UE_LOG(LogTemp, Log, TEXT("VegetationSpawnerSubsystem: no world"));
		return;
	}

	if (UVegetationSpawnerSubsystem* Subsystem = World->GetSubsystem<UVegetationSpawnerSubsystem>())
	{
		Subsystem->RunForcedTestSpawn();
		UE_LOG(LogTemp, Log, TEXT("VegetationSpawnerSubsystem: forced test spawn requested"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("VegetationSpawnerSubsystem: subsystem not found"));
	}
}

static FAutoConsoleCommandWithWorld ForcedTestSpawnCommand(
	TEXT("forcedtestspawn"),
	TEXT("Force a simple HISM test spawn using a default engine mesh"),
	FConsoleCommandWithWorldDelegate::CreateStatic(&ExecForcedTestSpawn));

void UVegetationSpawnerSubsystem::Deinitialize()
{
	LandscapeCells.Empty();
	ClearAllVegetation();
	VegetationTypes.Empty();
	TargetLandscape.Reset();
	Super::Deinitialize();
}

void UVegetationSpawnerSubsystem::SetTargetLandscape(ALandscape* InLandscape)
{
	TargetLandscape = InLandscape;
}

void UVegetationSpawnerSubsystem::AutoDiscoverLandscapeAndSpawn()
{
	if (!GetWorld())
	{
		return;
	}

	ALandscape* FoundLandscape = nullptr;
	for (TActorIterator<ALandscape> It(GetWorld()); It; ++It)
	{
		FoundLandscape = *It;
		break;
	}

	if (!FoundLandscape)
	{
		UE_LOG(LogTemp, Warning, TEXT("VegetationSpawnerSubsystem: no se encontró landscape en el mundo"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("VegetationSpawnerSubsystem: landscape found: %s"), *FoundLandscape->GetName());
	SetTargetLandscape(FoundLandscape);
	PopulateVegetationTypesFromContent();
	SpawnAllVegetation();
}

void UVegetationSpawnerSubsystem::DebugRebuildVegetation()
{
	if (!GetWorld())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("VegetationSpawnerSubsystem: debug rebuild requested"));
	ClearAllVegetation();
	AutoDiscoverLandscapeAndSpawn();
}

void UVegetationSpawnerSubsystem::ScheduleRetrySpawn(float DelaySeconds)
{
	if (!GetWorld())
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(RetrySpawnHandle);
	GetWorld()->GetTimerManager().SetTimer(RetrySpawnHandle, this, &UVegetationSpawnerSubsystem::DebugRebuildVegetation, DelaySeconds, false);
	UE_LOG(LogTemp, Log, TEXT("VegetationSpawnerSubsystem: scheduled retry in %.2f seconds"), DelaySeconds);
}

FString UVegetationSpawnerSubsystem::GetDebugStatus() const
{
	int32 EnabledCount = 0;
	int32 TotalPrefabs = 0;
	for (const UVegetationType* Veg : VegetationTypes)
	{
		if (!Veg) continue;
		if (Veg->bEnabled) EnabledCount++;
		TotalPrefabs += Veg->Prefabs.Num();
	}

	FString Status = FString::Printf(TEXT("Landscape=%s | Types=%d | Enabled=%d | Prefabs=%d | Actors=%d"),
		TargetLandscape.IsValid() ? *TargetLandscape->GetName() : TEXT("none"),
		VegetationTypes.Num(),
		EnabledCount,
		TotalPrefabs,
		SpawnedVegetationActors.Num());
	return Status;
}

void UVegetationSpawnerSubsystem::RunForcedTestSpawn()
{
	if (!GetWorld() || !TargetLandscape.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("VegetationSpawnerSubsystem: forced test spawn needs a valid landscape"));
		return;
	}

	ClearAllVegetation();
	PopulateVegetationTypesFromContent();

	UStaticMesh* TestMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube"));
	if (!TestMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("VegetationSpawnerSubsystem: forced test mesh not found"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("VegetationSpawnerSubsystem: forcing test spawn with mesh %s"), *TestMesh->GetPathName());

	FBox RegionBounds = TargetLandscape->GetComponentsBoundingBox();
	FVector Center = (RegionBounds.Min + RegionBounds.Max) * 0.5f;
	TArray<FTransform> TestTransforms;
	for (int32 i = 0; i < 8; i++)
	{
		FTransform T;
		T.SetLocation(Center + FVector(i * 20.0f, 0.0f, 0.0f));
		T.SetScale3D(FVector(0.5f));
		TestTransforms.Add(T);
	}

	AActor* SpawnActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity);
	if (!SpawnActor)
	{
		return;
	}

	SpawnedVegetationActors.Add(SpawnActor);
	UHierarchicalInstancedStaticMeshComponent* HISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(SpawnActor, UHierarchicalInstancedStaticMeshComponent::StaticClass(), TEXT("ForcedTestHISM"));
	HISM->SetStaticMesh(TestMesh);
	HISM->SetMobility(EComponentMobility::Static);
	HISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HISM->SetVisibility(true);
	HISM->SetHiddenInGame(false);
	HISM->RegisterComponentWithWorld(GetWorld());
	HISM->AddInstances(TestTransforms, false);
	UE_LOG(LogTemp, Log, TEXT("VegetationSpawnerSubsystem: forced test spawn added %d instances"), TestTransforms.Num());
}

void UVegetationSpawnerSubsystem::PopulateVegetationTypesFromContent()
{
	VegetationTypes.Empty();

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	TArray<FAssetData> AssetDataList;
	AssetRegistry.GetAssetsByClass(UVegetationType::StaticClass()->GetClassPathName(), AssetDataList);

	for (const FAssetData& AssetData : AssetDataList)
	{
		FString AssetPath = AssetData.GetSoftObjectPath().ToString();
		if (!AssetPath.Contains(TEXT("Vegetation/DataAssets")) && !AssetPath.Contains(TEXT("Vegetation")))
		{
			continue;
		}

		if (UVegetationType* VegType = Cast<UVegetationType>(AssetData.ToSoftObjectPath().TryLoad()))
		{
			VegetationTypes.Add(VegType);
		}
	}

	if (VegetationTypes.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("VegetationSpawnerSubsystem: no se encontraron DataAssets de vegetación"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("VegetationSpawnerSubsystem: %d DataAssets de vegetación cargados"), VegetationTypes.Num());
	}

	for (const auto& Veg : VegetationTypes)
	{
		if (!Veg)
		{
			continue;
		}
		if (Veg->Prefabs.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("VegetationSpawnerSubsystem: %s has no prefabs"), *Veg->GetName());
		}
	}
}

void UVegetationSpawnerSubsystem::SpawnAllVegetation()
{
	if (VegetationTypes.Num() == 0)
	{
		PopulateVegetationTypesFromContent();
	}

	int32 EnabledCount = 0;
	for (const auto& Veg : VegetationTypes)
	{
		if (Veg && Veg->bEnabled)
		{
			EnabledCount++;
			SpawnVegetationType(Veg);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("VegetationSpawnerSubsystem: enabled vegetation types for spawn = %d"), EnabledCount);
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

		if (Vegetation->Prefabs.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("VegetationSpawnerSubsystem: %s no tiene prefabs válidos"), *Vegetation->TypeName);
			return;
		}
		UE_LOG(LogTemp, Log, TEXT("VegetationSpawnerSubsystem: %s has %d prefabs | allowFallback=%s | collision=%s"), *Vegetation->TypeName, Vegetation->Prefabs.Num(), Vegetation->bAllowFallbackSpawn ? TEXT("yes") : TEXT("no"), Vegetation->bCollisionCheck ? TEXT("yes") : TEXT("no"));


	Vegetation->InstanceCount = 0;

	FBox RegionBounds = TargetLandscape->GetComponentsBoundingBox();

	TArray<FTransform> SpawnPoints = GeneratePoissonDiscPoints(Vegetation, RegionBounds);
	int32 SpawnedCount = SpawnPoints.Num();
	SpawnInstances(Vegetation, SpawnPoints);
	UE_LOG(LogTemp, Log, TEXT("VegetationSpawnerSubsystem: %s -> %d transforms | status=%s"), *Vegetation->TypeName, SpawnedCount, *GetDebugStatus());
}

void UVegetationSpawnerSubsystem::ClearAllVegetation()
{
	for (const auto& ActorPtr : SpawnedVegetationActors)
	{
		if (AActor* Actor = ActorPtr.Get())
		{
			Actor->Destroy();
		}
	}
	SpawnedVegetationActors.Empty();

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

	int32 MaxPasses = FMath::Max(1, Vegetation->MaxSpawnPasses);
	for (int32 Pass = 0; Pass < MaxPasses; Pass++)
	{
		TArray<FTransform> PassResults;
		float Radius = Vegetation->MinDistance * (1.0f + (0.1f * Pass));
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

			float EffectiveProbability = Vegetation->GlobalProbability;
			
			if (Vegetation->bUseNaturalClustering || Vegetation->bRiverAffinity || Vegetation->bNorthFacing || Vegetation->bSouthFacing)
			{
				float DensityMult = GetNaturalDensityMultiplier(WorldPos, Vegetation);
				EffectiveProbability *= DensityMult;
			}

			bool bPassedProbability = (ProbRng.FRand() * 100.0f) <= EffectiveProbability;
			if (!bPassedProbability)
			{
				if (!Vegetation->bAllowFallbackSpawn)
				{
					continue;
				}
				if (Results.Num() > 0 && FMath::RandRange(0, 3) != 0)
				{
					continue;
				}
			}

			if (Vegetation->bCollisionCheck && IsInsideOccupiedCell(WorldPos))
			{
				if (!Vegetation->bAllowFallbackSpawn)
				{
					continue;
				}
			}

			if (Vegetation->bRejectUnderwater && WorldHeight < WaterHeight)
			{
				if (!Vegetation->bAllowFallbackSpawn)
				{
					continue;
				}
			}

			if (WorldHeight < Vegetation->HeightRange.X || WorldHeight > Vegetation->HeightRange.Y)
			{
				if (!Vegetation->bAllowFallbackSpawn)
				{
					continue;
				}
			}

			if (Vegetation->SlopeRange.X > 0.0f || Vegetation->SlopeRange.Y < 90.0f)
			{
				float Slope = SampleLandscapeSlope(WorldPos);
				if (Slope < Vegetation->SlopeRange.X || Slope > Vegetation->SlopeRange.Y)
				{
					if (!Vegetation->bAllowFallbackSpawn)
					{
						continue;
					}
				}
			}

			float SpawnChance = 0.0f;
			if (Vegetation->LayerMasks.Num() > 0)
			{
				FVector2D NormalizedPos = GetNormalizedPosition(WorldPos);
				if (!TestSplatmap(NormalizedPos, Vegetation, SpawnChance))
				{
					if (!Vegetation->bAllowFallbackSpawn)
					{
						continue;
					}
					SpawnChance = 100.0f;
				}
			}
			else
			{
				SpawnChance = 100.0f;
			}

			FRandomStream FinalRng((int32)Sample.X * (int32)Sample.Y);
			if ((FinalRng.FRand() * 100.0f) > SpawnChance)
			{
				if (!Vegetation->bAllowFallbackSpawn)
				{
					continue;
				}
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

			PassResults.Add(SpawnTransform);
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

		Results.Append(PassResults);
		UE_LOG(LogTemp, Log, TEXT("VegetationSpawnerSubsystem: pass %d -> %d points for %s"), Pass + 1, PassResults.Num(), *Vegetation->TypeName);
	}

	if (Results.Num() == 0)
	{
		FVector Center = (RegionBounds.Min + RegionBounds.Max) * 0.5f;
		for (int32 i = 0; i < FMath::Min(8, FMath::Max(1, Vegetation->Prefabs.Num())); i++)
		{
			FTransform FallbackTransform;
			FallbackTransform.SetLocation(Center + FVector(i * 5.0f, 0.0f, 0.0f));
			FallbackTransform.SetScale3D(FVector(1.0f));
			Results.Add(FallbackTransform);
		}
		UE_LOG(LogTemp, Warning, TEXT("VegetationSpawnerSubsystem: fallback spawn used for %s | points=%d"), *Vegetation->TypeName, Results.Num());
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
			UE_LOG(LogTemp, Warning, TEXT("VegetationSpawnerSubsystem: prefab without mesh in %s"), *Vegetation->TypeName);
			continue;
		}
		UE_LOG(LogTemp, Log, TEXT("VegetationSpawnerSubsystem: mesh %s for %s"), *Prefab.Mesh->GetPathName(), *Vegetation->TypeName);

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		UE_LOG(LogTemp, Log, TEXT("VegetationSpawnerSubsystem: create container for %s"), *Vegetation->TypeName);
		SpawnActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
		if (!SpawnActor)
		{
			continue;
		}
		SpawnedVegetationActors.Add(SpawnActor);

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
			HISM->bUseDefaultCollision = false;
			HISM->SetCanEverAffectNavigation(false);
			HISM->SetVisibility(true);
			HISM->SetHiddenInGame(false);
			HISM->SetWorldLocation(FVector::ZeroVector);
			HISM->SetWorldRotation(FRotator::ZeroRotator);
			HISM->SetRelativeScale3D(FVector(1.0f));
			HISM->RegisterComponentWithWorld(GetWorld());
			HISM->MarkRenderStateDirty();
			HISM->RecreateRenderState_Concurrent();


#if WITH_EDITOR
		SpawnActor->SetActorLabel(FString::Printf(TEXT("VegetationSpawner_%s"), *Vegetation->TypeName));
#endif

		TArray<FTransform> InstanceTransforms;
		int32 PrefabCount = Vegetation->Prefabs.Num();
		UE_LOG(LogTemp, Log, TEXT("VegetationSpawnerSubsystem: %s -> %d transforms / %d prefabs"), *Vegetation->TypeName, Transforms.Num(), PrefabCount);
		int32 TotalCount = Transforms.Num();
		int32 AddedInstances = 0;

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
			HISM->UpdateInstanceTransform(0, FTransform::Identity, false, true);
			HISM->MarkRenderStateDirty();
			UE_LOG(LogTemp, Log, TEXT("VegetationSpawnerSubsystem: added %d instances to %s"), InstanceTransforms.Num(), *HISM->GetName());
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

float UVegetationSpawnerSubsystem::GetNaturalDensityMultiplier(const FVector& WorldPos, UVegetationType* Vegetation)
{
	if (!Vegetation)
	{
		return 1.0f;
	}

	float Multiplier = 1.0f;

	if (Vegetation->bUseNaturalClustering)
	{
		float ClusterMask = GetClusteringMask(WorldPos, Vegetation->ClusterSize);
		Multiplier *= FMath::Lerp(1.0f, ClusterMask, Vegetation->ClusterStrength);
	}

	if (Vegetation->bRiverAffinity)
	{
		float WaterMask = GetWaterProximityMask(WorldPos, Vegetation->WaterDistanceRange);
		Multiplier *= WaterMask;
	}

	if (Vegetation->bNorthFacing || Vegetation->bSouthFacing)
	{
		float OrientationMask = GetOrientationMask(WorldPos, Vegetation->bNorthFacing, Vegetation->bSouthFacing);
		Multiplier *= OrientationMask;
	}

	return FMath::Clamp(Multiplier, 0.0f, 3.0f);
}

float UVegetationSpawnerSubsystem::GetClusteringMask(const FVector& WorldPos, float ClusterSize)
{
	FVector NoisePos = WorldPos / (ClusterSize * 100.0f);
	float Noise3D = FMath::PerlinNoise3D(NoisePos);
	float ClusterMask = FMath::Clamp((Noise3D + 0.3f) / 0.6f, 0.0f, 1.0f);
	return ClusterMask;
}

float UVegetationSpawnerSubsystem::GetWaterProximityMask(const FVector& WorldPos, const FVector2D& WaterDistanceRange)
{
	const FVector RiverCenter(566633.0f * 100.0f, 4741532.0f * 100.0f, 0.0f);
	
	float Distance = FVector::Dist2D(WorldPos, RiverCenter);
	
	float MinDist = WaterDistanceRange.X * 100.0f;
	float MaxDist = WaterDistanceRange.Y * 100.0f;
	
	if (Distance < MinDist)
	{
		return 1.0f;
	}
	else if (Distance > MaxDist)
	{
		return 0.0f;
	}
	else
	{
		return 1.0f - ((Distance - MinDist) / (MaxDist - MinDist));
	}
}

float UVegetationSpawnerSubsystem::GetOrientationMask(const FVector& WorldPos, bool bNorthFacing, bool bSouthFacing)
{
	if (!TargetLandscape.IsValid())
	{
		return 1.0f;
	}

	const float Step = 200.0f;
	const FVector SamplePosX = WorldPos + FVector(Step, 0.0f, 0.0f);
	const FVector SampleNegX = WorldPos - FVector(Step, 0.0f, 0.0f);
	const FVector SamplePosY = WorldPos + FVector(0.0f, Step, 0.0f);
	const FVector SampleNegY = WorldPos - FVector(0.0f, Step, 0.0f);

	float PosX = TargetLandscape->GetHeightAtLocation(SamplePosX).Get(WorldPos.Z);
	float NegX = TargetLandscape->GetHeightAtLocation(SampleNegX).Get(WorldPos.Z);
	float PosY = TargetLandscape->GetHeightAtLocation(SamplePosY).Get(WorldPos.Z);
	float NegY = TargetLandscape->GetHeightAtLocation(SampleNegY).Get(WorldPos.Z);

	FVector Normal = FVector(NegX - PosX, NegY - PosY, 2.0f * Step).GetSafeNormal();

	FVector NorthDir = FVector(0.0f, 1.0f, 0.0f);
	float DotNorth = FVector::DotProduct(Normal, NorthDir);

	if (bNorthFacing && DotNorth > 0.3f)
	{
		return FMath::Clamp((DotNorth - 0.3f) / 0.4f, 0.0f, 1.0f);
	}
	else if (bSouthFacing && DotNorth < -0.3f)
	{
		return FMath::Clamp((-DotNorth - 0.3f) / 0.4f, 0.0f, 1.0f);
	}

	return bNorthFacing || bSouthFacing ? 0.0f : 1.0f;
}
