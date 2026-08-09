#include "World/AlsasuaReverbZoneSystem.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"

UAlsasuaReverbZoneSystem::UAlsasuaReverbZoneSystem()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAlsasuaReverbZoneSystem::BeginPlay()
{
	Super::BeginPlay();
	SpawnReverbZones();
}

void UAlsasuaReverbZoneSystem::SpawnReverbZones()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	const FVector Origin = Owner->GetActorLocation();
	const FString Label = Owner->GetName().ToLower();

	float Radius = DefaultZoneRadius;
	float ReverbTime = ExteriorReverbTime;
	float Volume = ExteriorVolume;

	if (Label.Contains(TEXT("iglesia")) || Label.Contains(TEXT("church")) ||
		Label.Contains(TEXT("basilica")))
	{
		Radius = ChurchZoneRadius;
		ReverbTime = ChurchReverbTime;
		Volume = ChurchVolume;
	}
	else if (Label.Contains(TEXT("cave")) || Label.Contains(TEXT("cueva")) ||
		Label.Contains(TEXT("sotano")))
	{
		Radius = CaveZoneRadius;
		ReverbTime = CaveReverbTime;
		Volume = CaveVolume;
	}
	else if (Label.Contains(TEXT("edificio")) || Label.Contains(TEXT("building")) ||
		Label.Contains(TEXT("casa")))
	{
		ReverbTime = InteriorReverbTime;
		Volume = InteriorVolume;
	}
	else if (Label.Contains(TEXT("callejon")) || Label.Contains(TEXT("alley")))
	{
		ReverbTime = AlleyReverbTime;
		Volume = AlleyVolume;
	}

	USphereComponent* Zone = NewObject<USphereComponent>(Owner);
	if (Zone)
	{
		Zone->SetupAttachment(Owner->GetRootComponent());
		Zone->SetRelativeLocation(FVector::ZeroVector);
		Zone->SetSphereRadius(Radius);
		Zone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Zone->SetCastShadow(false);
		Zone->RegisterComponent();

		SpawnedZones.Add(nullptr);
	}
}
