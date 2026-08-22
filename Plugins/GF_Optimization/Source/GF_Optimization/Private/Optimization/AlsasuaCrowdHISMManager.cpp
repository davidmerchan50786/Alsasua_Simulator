#include "Optimization/AlsasuaCrowdHISMManager.h"

AAlsasuaCrowdHISMManager::AAlsasuaCrowdHISMManager()
{
	HISMComponent = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISM"));
	RootComponent = HISMComponent;
}

void AAlsasuaCrowdHISMManager::UpdateInstance(int32 Index, FVector NewLocation)
{
	FTransform InstanceTransform;
	if (HISMComponent->GetInstanceTransform(Index, InstanceTransform, true))
	{
		InstanceTransform.SetLocation(NewLocation);
		HISMComponent->UpdateInstanceTransform(Index, InstanceTransform, true, true, true);
	}
}
