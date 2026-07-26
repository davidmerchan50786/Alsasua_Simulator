#include "Optimization/AlsasuaActorPoolSubsystem.h"
#include "TimerManager.h"

void UAlsasuaActorPoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UAlsasuaActorPoolSubsystem::Deinitialize()
{
	ReleaseAll();
	Pools.Empty();
	TotalCounts.Empty();
	AutoReturnTimers.Empty();
	Super::Deinitialize();
}

void UAlsasuaActorPoolSubsystem::WarmUpPool(TSubclassOf<AActor> Class, int32 Size)
{
	if (!Class || Size <= 0)
	{
		return;
	}

	TArray<AActor*>& Pool = Pools.FindOrAdd(Class);
	const int32 Existing = Pool.Num();

	Pool.Reserve(Existing + Size);
	int32& Total = TotalCounts.FindOrAdd(Class);
	Total = Existing;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (int32 i = 0; i < Size; ++i)
	{
		AActor* NewActor = World->SpawnActor<AActor>(Class, FVector(0, 0, -10000), FRotator::ZeroRotator, Params);
		if (NewActor)
		{
			DeactivateActor(NewActor);
			Pool.Add(NewActor);
			++Total;
		}
	}
}

AActor* UAlsasuaActorPoolSubsystem::AcquireActor(TSubclassOf<AActor> Class, FVector Location, FRotator Rotation)
{
	if (!Class)
	{
		return nullptr;
	}

	TArray<AActor*>& Pool = Pools.FindOrAdd(Class);

	// Buscar un actor inactivo válido.
	while (Pool.Num() > 0)
	{
		AActor* Actor = Pool.Pop();
		if (IsValid(Actor))
		{
			ActivateActor(Actor, Location, Rotation);
			return Actor;
		}
	}

	// Fallback: crear uno nuevo si no hay disponibles.
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	AActor* NewActor = World->SpawnActor<AActor>(Class, Location, Rotation, Params);
	if (NewActor)
	{
		TotalCounts.FindOrAdd(Class)++;
		ActivateActor(NewActor, Location, Rotation);
	}

	return NewActor;
}

void UAlsasuaActorPoolSubsystem::ReleaseActor(AActor* Actor, float AutoReturnTime)
{
	if (!IsValid(Actor))
	{
		return;
	}

	// Si ya tiene un timer de auto-return pendiente, cancelarlo.
	if (FTimerHandle* ExistingHandle = AutoReturnTimers.Find(Actor))
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(*ExistingHandle);
		}
		AutoReturnTimers.Remove(Actor);
	}

	// Si se pide auto-return, configurar timer.
	if (AutoReturnTime > 0.f)
	{
		UWorld* W = GetWorld();
		if (!W)
		{
			DeactivateActor(Actor);
			return;
		}

		FTimerHandle NewHandle;
		W->GetTimerManager().SetTimer(NewHandle,
			FTimerDelegate::CreateUObject(this, &UAlsasuaActorPoolSubsystem::OnAutoReturnTimerExpired, Actor),
			AutoReturnTime, false);
		AutoReturnTimers.Add(Actor, NewHandle);

		// También desactivar inmediatamente.
		DeactivateActor(Actor);
		return;
	}

	DeactivateActor(Actor);

	// Añadir al pool.
	UClass* Class = Actor->GetClass();
	TArray<TObjectPtr<AActor>>& Pool = Pools.FindOrAdd(Class);
	Pool.Add(Actor);
}

void UAlsasuaActorPoolSubsystem::ReleaseAll()
{
	for (auto& Pair : Pools)
	{
		for (TObjectPtr<AActor>& ActorPtr : Pair.Value)
		{
			if (IsValid(ActorPtr))
			{
				DeactivateActor(ActorPtr);
			}
		}
	}

	// Cancelar todos los timers.
	if (UWorld* World = GetWorld())
	{
		for (auto& Pair : AutoReturnTimers)
		{
			World->GetTimerManager().ClearTimer(Pair.Value);
		}
	}
	AutoReturnTimers.Empty();
}

int32 UAlsasuaActorPoolSubsystem::GetInactiveCount(TSubclassOf<AActor> Class) const
{
	if (const TArray<TObjectPtr<AActor>>* Pool = Pools.Find(Class))
	{
		return Pool->Num();
	}
	return 0;
}

int32 UAlsasuaActorPoolSubsystem::GetTotalCount(TSubclassOf<AActor> Class) const
{
	if (const int32* Count = TotalCounts.Find(Class))
	{
		return *Count;
	}
	return 0;
}

int32 UAlsasuaActorPoolSubsystem::GetTotalAllClasses() const
{
	int32 Total = 0;
	for (const auto& Pair : TotalCounts)
	{
		Total += Pair.Value;
	}
	return Total;
}

void UAlsasuaActorPoolSubsystem::DeactivateActor(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	Actor->SetActorHiddenInGame(true);
	Actor->SetActorEnableCollision(false);
	Actor->SetActorTickEnabled(false);

	// Si es un pawn, desactivar movimiento.
	if (APawn* Pawn = Cast<APawn>(Actor))
	{
		if (UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(Pawn->GetRootComponent()))
		{
			Root->SetSimulatePhysics(false);
			Root->SetPhysicsLinearVelocity(FVector::ZeroVector);
			Root->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		}
	}
}

void UAlsasuaActorPoolSubsystem::ActivateActor(AActor* Actor, FVector Location, FRotator Rotation)
{
	if (!IsValid(Actor))
	{
		return;
	}

	Actor->SetActorLocationAndRotation(Location, Rotation);
	Actor->SetActorHiddenInGame(false);
	Actor->SetActorEnableCollision(true);
	Actor->SetActorTickEnabled(true);
}

void UAlsasuaActorPoolSubsystem::OnAutoReturnTimerExpired(AActor* Actor)
{
	AutoReturnTimers.Remove(Actor);

	if (!IsValid(Actor))
	{
		return;
	}

	DeactivateActor(Actor);

	UClass* Class = Actor->GetClass();
	TArray<TObjectPtr<AActor>>& Pool = Pools.FindOrAdd(Class);
	Pool.Add(Actor);
}
