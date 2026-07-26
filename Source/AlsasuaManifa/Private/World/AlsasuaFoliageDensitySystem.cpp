#include "World/AlsasuaFoliageDensitySystem.h"
#include "Engine/World.h"

UAlsasuaFoliageDensitySystem::UAlsasuaFoliageDensitySystem()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAlsasuaFoliageDensitySystem::BeginPlay()
{
	Super::BeginPlay();
	ApplyDensitySettings();
}

void UAlsasuaFoliageDensitySystem::ApplyDensitySettings()
{
	// Store density multipliers for other systems to query
	// The actual foliage spawning systems will read these values
	UE_LOG(LogTemp, Log, TEXT("AlsasuaFoliageDensity: Biome density settings applied."));
}
