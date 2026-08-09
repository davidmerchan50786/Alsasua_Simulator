// AlsasuaManifestacionManager.cpp
// ═══════════════════════════════════════════════════════════════════════════
//  Implementación del subsistema de mega-manifestaciones con líder-seguidor.
//  Port del MegaManifestacion de Unity a UE 5.4 C++.
// ═══════════════════════════════════════════════════════════════════════════

#include "AI/Crowd/AlsasuaManifestacionManager.h"
#include "AlsasuaCore.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "CollisionQueryParams.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Initialize: se llama al registrar el subsistema.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaManifestacionManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (IsRunningCommandlet() || GIsEditor)
	{
		return;
	}

	// Registrar timer de actualización (~30Hz).
	UWorld* World = GetWorld();
	if (World != nullptr)
	{
		World->GetTimerManager().SetTimer(TickTimerHandle, this,
			&UAlsasuaManifestacionManager::InternalTick, UpdateInterval, true);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  Deinitialize: limpia toda la manifestación.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaManifestacionManager::Deinitialize()
{
	// Limpiar el timer antes de destruir.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TickTimerHandle);
	}

	DetenerManifestacion();
	Super::Deinitialize();
}

// ─────────────────────────────────────────────────────────────────────────────
//  InternalTick: método registrado en el TimerManager (~30Hz).
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaManifestacionManager::InternalTick()
{
	if (Leaders.Num() == 0 && Followers.Num() == 0)
	{
		return;
	}

	const float DeltaTime = UpdateInterval;

	TickLeaders(DeltaTime);
	TickFollowers(DeltaTime);
	SyncInstancedTransforms();
}

// ─────────────────────────────────────────────────────────────────────────────
//  SetupISMCs: crea los componentes ISMC para líderes y seguidores.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaManifestacionManager::SetupISMCs()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	if (LeaderISMC == nullptr)
	{
		LeaderISMC = NewObject<UInstancedStaticMeshComponent>(World);
		LeaderISMC->RegisterComponentWithWorld(World);
		LeaderISMC->SetMobility(EComponentMobility::Movable);
		LeaderISMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		LeaderISMC->CastShadow = true;
		LeaderISMC->SetCanEverAffectNavigation(false);

		UStaticMesh* Mesh = LoadObject<UStaticMesh>(
			nullptr, TEXT("/Engine/BasicShapes/Cylinder"), nullptr, LOAD_None);
		if (Mesh != nullptr)
		{
			LeaderISMC->SetStaticMesh(Mesh);
		}
		if (LeaderMaterial != nullptr)
		{
			LeaderISMC->SetMaterial(0, LeaderMaterial);
		}
	}

	if (FollowerISMC == nullptr)
	{
		FollowerISMC = NewObject<UInstancedStaticMeshComponent>(World);
		FollowerISMC->RegisterComponentWithWorld(World);
		FollowerISMC->SetMobility(EComponentMobility::Movable);
		FollowerISMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		FollowerISMC->CastShadow = true;
		FollowerISMC->SetCanEverAffectNavigation(false);

		UStaticMesh* Mesh = LoadObject<UStaticMesh>(
			nullptr, TEXT("/Engine/BasicShapes/Cylinder"), nullptr, LOAD_None);
		if (Mesh != nullptr)
		{
			FollowerISMC->SetStaticMesh(Mesh);
		}
		if (FollowerMaterial != nullptr)
		{
			FollowerISMC->SetMaterial(0, FollowerMaterial);
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  IniciarManifestacion: crea líderes y seguidores según la configuración.
//  Port de GenerarManifestacion del Unity script.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaManifestacionManager::IniciarManifestacion(const FManifestacionConfig& Config)
{
	ActiveConfig = Config;
	Leaders.Empty(Config.NumLeaders);
	Followers.Empty(Config.TotalManifestantes - Config.NumLeaders);

	const int32 NumFollowers = Config.TotalManifestantes - Config.NumLeaders;

	// Proyección topográfica: encontrar suelo bajo el epicentro.
	FVector Epicenter = FVector::ZeroVector;
	{
		UWorld* World = GetWorld();
		if (World != nullptr)
		{
			FHitResult Hit;
			const FVector Start = Epicenter + FVector(0.f, 0.f, 50000.f);
			const FVector End = Epicenter - FVector(0.f, 0.f, 100000.f);
			if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility))
			{
				Epicenter = Hit.ImpactPoint;
			}
		}
	}

	// Crear líderes (con NavMeshAgent real).
	for (int32 i = 0; i < Config.NumLeaders; ++i)
	{
		const FVector SpawnPos = Epicenter + FVector(
			FMath::RandRange(-Config.SpawnRadius, Config.SpawnRadius),
			FMath::RandRange(-Config.SpawnRadius, Config.SpawnRadius),
			0.f);

		SpawnLeader(i, SpawnPos);
	}

	// Crear seguidores (offsets matemáticos, sin NavMesh).
	LeadersCentroid = Epicenter;
	LeadersAvgDirection = FVector::ForwardVector;

	for (int32 i = 0; i < NumFollowers; ++i)
	{
		FManifestacionFollowerData Follower;
		Follower.Position = Epicenter + FVector(
			FMath::RandRange(-Config.SpawnRadius * 1.5f, Config.SpawnRadius * 1.5f),
			FMath::RandRange(-Config.SpawnRadius * 1.5f, Config.SpawnRadius * 1.5f),
			0.f);
		Follower.Velocity = FVector::ZeroVector;
		Follower.GroundHeight = Follower.Position.Z;
		Follower.AssignedLeaderIndex = i % Config.NumLeaders;
		Follower.OffsetX = FMath::RandRange(-1000.f, 1000.f);
		Follower.OffsetZ = FMath::RandRange(-2000.f, -200.f);
		Follower.bIsActive = true;

		Followers.Add(Follower);
	}

	UE_LOG(LogAlsasuaAI, Log, TEXT("[ManifestacionManager] %d manifestantes generados (%d líderes + %d seguidores)."),
		Config.TotalManifestantes, Config.NumLeaders, NumFollowers);
}

// ─────────────────────────────────────────────────────────────────────────────
//  SpawnLeader: crea un Pawn/AIController con NavMeshAgent.
//  Port de la creación de líderes del Unity script.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaManifestacionManager::SpawnLeader(int32 LeaderIndex, const FVector& SpawnPos)
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	// Crear un pawn simple para el líder.
	FManifestacionLeaderData LeaderData;
	LeaderData.Position = SpawnPos;
	LeaderData.Velocity = FVector::ForwardVector * ActiveConfig.MarchSpeed;
	LeaderData.NavDestination = SpawnPos;
	LeaderData.CurrentWaypoint = 0;
	LeaderData.bIsActive = true;

	// Asignar ruta de waypoints si existe.
	if (ActiveConfig.RouteWaypoints.Num() >= 2)
	{
		LeaderData.RouteWaypoints = ActiveConfig.RouteWaypoints;
		LeaderData.NavDestination = ActiveConfig.RouteWaypoints[0];
	}

	Leaders.Add(LeaderData);
}

// ─────────────────────────────────────────────────────────────────────────────
//  AssignNewRouteToLeader: asigna un nuevo destino NavMesh aleatorio.
//  Port de AsignarNuevaRuta del LiderRutaIA de Unity.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaManifestacionManager::AssignNewRouteToLeader(int32 LeaderIndex)
{
	if (!Leaders.IsValidIndex(LeaderIndex))
	{
		return;
	}

	// Usar la ruta de waypoints de la configuración si existe.
	FManifestacionLeaderData& Leader = Leaders[LeaderIndex];

	if (Leader.RouteWaypoints.Num() >= 2)
	{
		Leader.CurrentWaypoint = (Leader.CurrentWaypoint + 1) % Leader.RouteWaypoints.Num();
		Leader.NavDestination = Leader.RouteWaypoints[Leader.CurrentWaypoint];
	}
	else
	{
		// Generar un punto aleatorio dentro de un radio amplio.
		const FVector RandomOffset = FVector(
			FMath::RandRange(-10000.f, 10000.f),
			FMath::RandRange(-10000.f, 10000.f),
			0.f);
		Leader.NavDestination = Leader.Position + RandomOffset;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  DetenerManifestacion: limpia todos los datos.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaManifestacionManager::DetenerManifestacion()
{
	Leaders.Empty();
	Followers.Empty();

	if (LeaderISMC != nullptr)
	{
		LeaderISMC->ClearInstances();
	}
	if (FollowerISMC != nullptr)
	{
		FollowerISMC->ClearInstances();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  SetTension: cambia el nivel de tensión y emite el delegado.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaManifestacionManager::SetTension(EManifestacionTension NuevaTension)
{
	if (TensionActual != NuevaTension)
	{
		TensionActual = NuevaTension;
		OnTensionChanged.Broadcast(NuevaTension);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  AddRoutePoint: añade un punto a la ruta en runtime.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaManifestacionManager::AddRoutePoint(const FVector& Point)
{
	ActiveConfig.RouteWaypoints.Add(Point);

	// Actualizar los líderes que ya existen.
	for (int32 i = 0; i < Leaders.Num(); ++i)
	{
		Leaders[i].RouteWaypoints = ActiveConfig.RouteWaypoints;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  TickLeaders: actualiza la posición de los líderes.
//  Port de la lógica NavMesh del LiderRutaIA de Unity.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaManifestacionManager::TickLeaders(float DeltaTime)
{
	FVector SumPos = FVector::ZeroVector;
	FVector SumDir = FVector::ZeroVector;
	int32 ActiveCount = 0;

	for (int32 i = 0; i < Leaders.Num(); ++i)
	{
		FManifestacionLeaderData& Leader = Leaders[i];
		if (!Leader.bIsActive)
		{
			continue;
		}

		// Mover hacia el destino.
		const FVector ToDest = Leader.NavDestination - Leader.Position;
		const float DistToDest = ToDest.Size2D();

		if (DistToDest < 200.f) // 2m → waypoint alcanzado
		{
			AssignNewRouteToLeader(i);
		}

		if (DistToDest > 10.f)
		{
			const FVector MoveDir = ToDest.GetSafeNormal2D();
			const float Speed = ActiveConfig.MarchSpeed;

			Leader.Velocity = FMath::VInterpTo(
				Leader.Velocity,
				MoveDir * Speed,
				DeltaTime,
				5.f);

			Leader.Position += Leader.Velocity * DeltaTime;
		}

		SumPos += Leader.Position;
		SumDir += Leader.Velocity.GetSafeNormal();
		++ActiveCount;
	}

	// Calcular centroid y dirección promedio.
	if (ActiveCount > 0)
	{
		LeadersCentroid = SumPos / static_cast<float>(ActiveCount);
		LeadersAvgDirection = (SumDir / static_cast<float>(ActiveCount)).GetSafeNormal();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  TickFollowers: actualiza seguidores con lerp matricial.
//  Port directo del Update() del MegaManifestacion de Unity.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaManifestacionManager::TickFollowers(float DeltaTime)
{
	for (int32 i = 0; i < Followers.Num(); ++i)
	{
		FManifestacionFollowerData& Follower = Followers[i];
		if (!Follower.bIsActive)
		{
			continue;
		}

		// Verificar que su líder sigue activo.
		if (!Leaders.IsValidIndex(Follower.AssignedLeaderIndex) ||
			!Leaders[Follower.AssignedLeaderIndex].bIsActive)
		{
			// Buscar un líder vivo alternativo (port de V23 FIX: Zombie Followers).
			bool bReassigned = false;
			for (int32 k = 0; k < Leaders.Num(); ++k)
			{
				if (Leaders[k].bIsActive)
				{
					Follower.AssignedLeaderIndex = k;
					bReassigned = true;
					break;
				}
			}

			if (!bReassigned)
			{
				// Sin líderes vivos → despawn.
				Follower.bIsActive = false;
				continue;
			}
		}

		const FManifestacionLeaderData& Leader = Leaders[Follower.AssignedLeaderIndex];

		// Offset matricial: posición del líder + right * offsetX + forward * offsetZ.
		// Usamos la dirección de velocidad del líder como forward.
		const FVector LeaderForward = Leader.Velocity.GetSafeNormal2D();
		const FVector LeaderRight = FVector::CrossProduct(FVector::UpVector, LeaderForward).GetSafeNormal2D();

		const FVector Target = Leader.Position
			+ LeaderRight * Follower.OffsetX
			+ LeaderForward * Follower.OffsetZ;

		// Interpolación suave (Lerp) — rompe la rigidez matricial.
		const float LerpAlpha = FMath::Clamp(ActiveConfig.FollowLerpSpeed * DeltaTime, 0.f, 1.f);
		Follower.Position = FMath::Lerp(Follower.Position, Target, LerpAlpha);

		// Rotación continua suave.
		const FVector MoveDir = (Target - Follower.Position).GetSafeNormal2D();
		if (MoveDir.SizeSquared() > 0.01f)
		{
			const FRotator TargetRot = MoveDir.ToOrientationRotator();
			const FRotator CurrentRot = Follower.Velocity.GetSafeNormal2D().ToOrientationRotator();
			const FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, 5.f);
			Follower.Velocity = NewRot.Vector() * Follower.Velocity.Size();
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  ReassignOrphanedFollowers: reasigna seguidores cuando un líder muere.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaManifestacionManager::ReassignOrphanedFollowers(int32 DeadLeaderIndex)
{
	for (int32 i = 0; i < Followers.Num(); ++i)
	{
		if (Followers[i].AssignedLeaderIndex == DeadLeaderIndex)
		{
			// Buscar un líder vivo.
			bool bReassigned = false;
			for (int32 k = 0; k < Leaders.Num(); ++k)
			{
				if (k != DeadLeaderIndex && Leaders[k].bIsActive)
				{
					Followers[i].AssignedLeaderIndex = k;
					bReassigned = true;
					break;
				}
			}

			if (!bReassigned)
			{
				Followers[i].bIsActive = false;
			}
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  SyncInstancedTransforms: actualiza las transformaciones de los ISMCs.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaManifestacionManager::SyncInstancedTransforms()
{
	// ── Líderes ──────────────────────────────────────────────────────────────
	if (LeaderISMC != nullptr)
	{
		const int32 NumLeaders = Leaders.Num();

		LeaderISMC->ClearInstances();
		for (int32 i = 0; i < NumLeaders; ++i)
		{
			if (!Leaders[i].bIsActive)
			{
				continue;
			}

			const FVector Loc = Leaders[i].Position + FVector(0.f, 0.f, 87.5f);
			const FRotator Rot = Leaders[i].Velocity.GetSafeNormal2D().ToOrientationRotator();
			const FTransform T(Rot, Loc, FVector(0.35f, 0.35f, 0.875f));
			LeaderISMC->AddInstance(T, true);
		}
	}

	// ── Seguidores ───────────────────────────────────────────────────────────
	if (FollowerISMC != nullptr)
	{
		const int32 NumActiveFollowers = Followers.Num();

		FollowerISMC->ClearInstances();
		for (int32 i = 0; i < NumActiveFollowers; ++i)
		{
			if (!Followers[i].bIsActive)
			{
				continue;
			}

			const FVector Loc = Followers[i].Position + FVector(0.f, 0.f, 87.5f);
			const FRotator Rot = Followers[i].Velocity.GetSafeNormal2D().ToOrientationRotator();
			const FTransform T(Rot, Loc, FVector(0.35f, 0.35f, 0.875f));
			FollowerISMC->AddInstance(T, true);
		}
	}
}
