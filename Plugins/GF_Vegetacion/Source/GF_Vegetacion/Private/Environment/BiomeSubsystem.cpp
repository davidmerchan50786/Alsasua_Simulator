#include "Environment/BiomeSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Landscape.h"
#include "Math/UnrealMathUtility.h"

void UBiomeSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	DiscoverLandscape();
}

void UBiomeSubsystem::SetLandscape(ALandscape* InLandscape)
{
	Landscape = InLandscape;
	bLandscapeDiscovered = Landscape.IsValid();
}

void UBiomeSubsystem::DiscoverLandscape() const
{
	if (bLandscapeDiscovered || !GetWorld())
	{
		return;
	}

	for (TActorIterator<ALandscape> It(GetWorld()); It; ++It)
	{
		Landscape = *It;
		break;
	}

	bLandscapeDiscovered = true;

	if (!Landscape.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("BiomeSubsystem: no landscape found in world"));
	}
}

bool UBiomeSubsystem::SampleTerrain(const FVector& WorldPos, float& OutHeightCm, float& OutSlopeDegrees) const
{
	OutHeightCm = WorldPos.Z;
	OutSlopeDegrees = 0.0f;

	const ALandscape* Terrain = Landscape.Get();
	if (!Terrain)
	{
		return false;
	}

	OutHeightCm = Terrain->GetHeightAtLocation(WorldPos).Get(WorldPos.Z);

	constexpr float Step = 50.0f;
	const FVector Right = WorldPos + FVector(Step, 0.0f, 0.0f);
	const FVector Forward = WorldPos + FVector(0.0f, Step, 0.0f);

	const float HCenter = OutHeightCm;
	const float HRight = Terrain->GetHeightAtLocation(Right).Get(HCenter);
	const float HForward = Terrain->GetHeightAtLocation(Forward).Get(HCenter);

	const float DX = (HRight - HCenter) / Step;
	const float DY = (HForward - HCenter) / Step;

	OutSlopeDegrees = FMath::RadiansToDegrees(FMath::Atan2(FMath::Sqrt(DX * DX + DY * DY), 1.0f));
	return true;
}

float UBiomeSubsystem::GetTemperatureAtLocation(const FVector& WorldPos) const
{
	float HeightCm = WorldPos.Z;
	float Slope = 0.0f;
	SampleTerrain(WorldPos, HeightCm, Slope);

	return FMath::Clamp(1.0f - HeightCm * 0.00005f, 0.0f, 1.0f);
}

float UBiomeSubsystem::GetMoistureAtLocation(const FVector& WorldPos) const
{
	const float Noise = FMath::PerlinNoise2D(FVector2D(WorldPos.X, WorldPos.Y) * MoistureNoiseScale);
	float Moisture = Noise * 0.5f + 0.5f;

	float HeightCm = WorldPos.Z;
	float Slope = 0.0f;
	if (SampleTerrain(WorldPos, HeightCm, Slope))
	{
		const float AboveWater = HeightCm - WaterHeight;
		const float Proximity = 1.0f - FMath::Clamp(AboveWater / FMath::Max(WaterInfluenceDistance, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
		Moisture += WaterMoistureBonus * Proximity;
	}

	return FMath::Clamp(Moisture, 0.0f, 1.0f);
}

UBiomeDefinition* UBiomeSubsystem::GetBiomeAtLocation(const FVector& WorldPos) const
{
	DiscoverLandscape();

	float HeightCm = WorldPos.Z;
	float Slope = 0.0f;
	SampleTerrain(WorldPos, HeightCm, Slope);

	const float Temperature = GetTemperatureAtLocation(WorldPos);
	const float Moisture = GetMoistureAtLocation(WorldPos);

	for (const TObjectPtr<UBiomeDefinition>& Biome : Biomes)
	{
		if (Biome && Biome->Matches(Temperature, Moisture, HeightCm, Slope))
		{
			return Biome;
		}
	}

	return nullptr;
}

TArray<UBiomeDefinition*> UBiomeSubsystem::SampleBiomeGrid(const FVector& Center, FVector2D Extent, int32 Resolution, int32& OutGridSizeX, int32& OutGridSizeY) const
{
	OutGridSizeX = FMath::Max(Resolution, 1);
	OutGridSizeY = OutGridSizeX;

	TArray<UBiomeDefinition*> Grid;
	Grid.SetNumUninitialized(OutGridSizeX * OutGridSizeY);

	const FVector Origin(Center.X - Extent.X * 0.5f, Center.Y - Extent.Y * 0.5f, Center.Z);
	const FVector2D CellSize(Extent.X / OutGridSizeX, Extent.Y / OutGridSizeY);

	for (int32 Y = 0; Y < OutGridSizeY; ++Y)
	{
		for (int32 X = 0; X < OutGridSizeX; ++X)
		{
			const FVector SamplePos(
				Origin.X + (X + 0.5f) * CellSize.X,
				Origin.Y + (Y + 0.5f) * CellSize.Y,
				Center.Z);
			Grid[Y * OutGridSizeX + X] = GetBiomeAtLocation(SamplePos);
		}
	}

	return Grid;
}
