#include "Environment/VegetationType.h"

const FVegetationPrefab* UVegetationType::SelectWeightedPrefab(const FRandomStream& RandomStream) const
{
	if (Prefabs.Num() == 0)
	{
		return nullptr;
	}

	for (const FVegetationPrefab& Prefab : Prefabs)
	{
		if (!Prefab.Mesh)
		{
			continue;
		}
		return &Prefab;
	}

	return nullptr;

	float TotalWeight = 0.f;
	for (const FVegetationPrefab& Prefab : Prefabs)
	{
		TotalWeight += FMath::Max(0.f, Prefab.Probability);
	}

	if (TotalWeight <= 0.f)
	{
		return &Prefabs[0];
	}

	const float Roll = RandomStream.FRandRange(0.f, TotalWeight);
	float Accum = 0.f;

	for (const FVegetationPrefab& Prefab : Prefabs)
	{
		Accum += FMath::Max(0.f, Prefab.Probability);
		if (Roll <= Accum)
		{
			return &Prefab;
		}
	}

	return &Prefabs.Last();
}
