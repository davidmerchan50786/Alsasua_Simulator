#include "World/AlsasuaLightProbeSystem.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"

UAlsasuaLightProbeSystem::UAlsasuaLightProbeSystem()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAlsasuaLightProbeSystem::BeginPlay()
{
	Super::BeginPlay();
	SpawnLightProbes();
}

void UAlsasuaLightProbeSystem::SpawnLightProbes()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	const FVector Origin = Owner->GetActorLocation();
	const float BuildingWidth = 800.f;
	const float BuildingHeight = 1200.f;

	const int32 NumProbesX = FMath::CeilToInt(BuildingWidth / ProbeSpacing);
	const int32 NumProbesY = 2;
	const int32 NumProbesZ = FMath::CeilToInt(BuildingHeight / ProbeSpacing);

	for (int32 x = 0; x < NumProbesX; ++x)
	{
		for (int32 y = 0; y < NumProbesY; ++y)
		{
			for (int32 z = 0; z < NumProbesZ; ++z)
			{
				const FVector ProbeLoc = Origin + FVector(
					(x - NumProbesX * 0.5f) * ProbeSpacing,
					(y - NumProbesY * 0.5f) * ProbeSpacing,
					(z + 0.5f) * ProbeSpacing);

				USphereComponent* Probe = NewObject<USphereComponent>(Owner);
				if (!Probe) continue;

				Probe->SetupAttachment(Owner->GetRootComponent());
				Probe->SetRelativeLocation(ProbeLoc - Origin);
				Probe->SetSphereRadius(ProbeRadius);
				Probe->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Probe->SetCastShadow(false);
				Probe->SetMobility(EComponentMobility::Movable);
				Probe->RegisterComponent();

				SpawnedProbes.Add(Probe);
			}
		}
	}
}
