#include "Optimization/AlsasuaActorPoolSubsystem.h"

void UAlsasuaActorPoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UAlsasuaActorPoolSubsystem::WarmUpPool(TSubclassOf<AAlsasuaCharacter> Class, int32 Size)
{
	for (int32 i = 0; i < Size; i++)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AAlsasuaCharacter* NewActor = GetWorld()->SpawnActor<AAlsasuaCharacter>(Class, FVector(0,0,-5000), FRotator::ZeroRotator, Params);

		if (NewActor)
		{
			ReleaseActor(NewActor); // Lo enviamos directamente al pool (desactivado)
		}
	}
}

AAlsasuaCharacter* UAlsasuaActorPoolSubsystem::AcquireActor(FVector Location, FRotator Rotation)
{
	AAlsasuaCharacter* Actor = nullptr;
	if (InactivePool.Num() > 0)
	{
		Actor = InactivePool.Pop();
		Actor->SetActorLocationAndRotation(Location, Rotation);
		Actor->SetActorHiddenInGame(false);
		Actor->SetActorTickEnabled(true);
		Actor->SetActorEnableCollision(true);
	}
	return Actor;
}

void UAlsasuaActorPoolSubsystem::ReleaseActor(AAlsasuaCharacter* Actor)
{
	if (!Actor) return;
	Actor->SetActorHiddenInGame(true);
	Actor->SetActorTickEnabled(false);
	Actor->SetActorEnableCollision(false);
	Actor->SetActorLocation(FVector(0, 0, -5000)); // Teletransporte fuera del mapa
	InactivePool.Push(Actor);
}
