// AlsasuaCrowdSubsystem.cpp
// ═══════════════════════════════════════════════════════════════════════════
//  Implementación del subsistema de multitud con boid flocking y GPU
//  instanced rendering para UE 5.4.
// ═══════════════════════════════════════════════════════════════════════════

#include "AI/Crowd/AlsasuaCrowdSubsystem.h"
#include "AI/Crowd/SpatialHashGrid.h"
#include "AI/Crowd/CrowdRagdollActor.h"
#include "AI/AlsasuaTacticManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MeshDescriptionBuilder.h"
#include "MeshDescription.h"
#include "AlsasuaCore.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "CollisionQueryParams.h"
#include "Engine/StaticMeshActor.h"

// Paleta de colores predefinida (port directo del Unity script).
const TArray<FLinearColor> UAlsasuaCrowdSubsystem::ColorPalette = {
	FLinearColor(0.80f, 0.08f, 0.08f),   // Rojo
	FLinearColor(0.06f, 0.06f, 0.06f),   // Negro
	FLinearColor(0.12f, 0.42f, 0.18f),   // Verde oscuro
	FLinearColor(0.88f, 0.80f, 0.08f),   // Amarillo
	FLinearColor(0.10f, 0.18f, 0.52f),   // Azul
	FLinearColor(0.88f, 0.88f, 0.88f),   // Blanco
	FLinearColor(0.42f, 0.26f, 0.12f),   // Marrón
	FLinearColor(0.35f, 0.18f, 0.45f),   // Púrpura
};

// ─────────────────────────────────────────────────────────────────────────────
//  Initialize: se llama al registrar el subsistema.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaCrowdSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Sin mundo de juego (commandlets/cook) no hay multitud: saltar todo.
	if (IsRunningCommandlet() || GIsEditor)
	{
		return;
	}

	UpdateInterval = 1.f / FMath::Max(UpdateFrequency, 1.f);

	SetupInstancedRendering();

	// Inicializar el spatial hash grid.
	SpatialGrid.Init(GlobalFlockingParams.SeparationRadius * 2.4f, MaxAgents);

	// Register update timer (~30Hz).
	UWorld* World = GetWorld();
	if (World != nullptr)
	{
		World->GetTimerManager().SetTimer(TickTimerHandle, this,
			&UAlsasuaCrowdSubsystem::InternalTick, UpdateInterval, true);

		// Timer de limpieza periódica de agentes muertos.
		if (DeadAgentCleanupInterval > 0.f)
		{
			World->GetTimerManager().SetTimer(DeadAgentCleanupTimerHandle, this,
				&UAlsasuaCrowdSubsystem::OnDeadAgentCleanupTick,
				DeadAgentCleanupInterval, true);
		}

		// Wire TacticManager → crowd behavior.
		if (UAlsasuaTacticManager* TacticMgr = World->GetSubsystem<UAlsasuaTacticManager>())
			TacticMgr->OnTacticChanged.AddDynamic(this, &UAlsasuaCrowdSubsystem::HandleTacticChanged);

		// Demo de multitud: marcha que arranca sola alrededor de Herriko Plaza.
		if (bDemoCrowdAutoStart)
		{
			DemoSpawnHerrikoPlaza();
		}
	}
}

// ── Demo: multitud que arranca sola ────────────────────────────────────────
void UAlsasuaCrowdSubsystem::DemoSpawnHerrikoPlaza()
{
	FVector Plaza(191800.f, 857000.f, 0.f); // Herriko Plaza (mismo centro que el spawn del jugador)

	// Ruta circular alrededor de la plaza para que la marcha dé vueltas.
	TArray<FVector> Ruta;
	const int32 Seg = 8;
	const float Radio = 1200.f;
	for (int32 i = 0; i < Seg; ++i)
	{
		const float Ang = (float)i / Seg * 2.f * PI;
		Ruta.Add(Plaza + FVector(FMath::Cos(Ang) * Radio, FMath::Sin(Ang) * Radio, 0.f));
	}

	FCrowdSpawnRequest Req;
	Req.RoutePoints = Ruta;
	Req.SpawnCenter = Plaza + FVector(0.f, 0.f, 40.f);
	Req.SpawnRadius = 400.f;
	Req.NumAgents = 500;
	Req.FlockingParams.FormationWidth = 20;

	SpawnCrowdAgents(Req);
	UE_LOG(LogAlsasuaAI, Log, TEXT("[AlsasuaCrowdSubsystem] Demo de multitud arrancada: %d agentes en Herriko Plaza."), GetAliveAgentCount());
}

// ─────────────────────────────────────────────────────────────────────────────
//  Deinitialize: limpia memoria y componentes.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaCrowdSubsystem::Deinitialize()
{
	// Unwire TacticManager.
	if (UWorld* W = GetWorld())
	{
		if (UAlsasuaTacticManager* TacticMgr = W->GetSubsystem<UAlsasuaTacticManager>())
			TacticMgr->OnTacticChanged.RemoveDynamic(this, &UAlsasuaCrowdSubsystem::HandleTacticChanged);

		// Limpiar timers antes de destruir.
		W->GetTimerManager().ClearTimer(TickTimerHandle);
		W->GetTimerManager().ClearTimer(DeadAgentCleanupTimerHandle);
	}

	DespawnAllAgents();

	for (UInstancedStaticMeshComponent* ISMC : ColorISMCs)
	{
		if (ISMC != nullptr)
		{
			ISMC->ClearInstances();
		}
	}

	Super::Deinitialize();
}

// ─────────────────────────────────────────────────────────────────────────────
//  SetupInstancedRendering: crea el ISMC si no existe.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaCrowdSubsystem::SetupInstancedRendering()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	if (!ColorISMCs.IsEmpty())
	{
		return;
	}

	// Asignar malla (humanoid procedural por defecto).
	UStaticMesh* Mesh = AgentMesh;
	if (Mesh == nullptr)
	{
		Mesh = CreateDefaultHumanoidMesh();
		if (Mesh != nullptr)
		{
			AgentMesh = Mesh;
		}
	}
	if (Mesh == nullptr)
	{
		return;
	}

	// Un ISMC por color de la paleta; cada uno con una MID teñida de su color.
	// Cada agente se instancia en el ISMC de su AgentColorIdx. Esto convierte la
	// multitud de un único color (cilindros grises) en una variada (8 colores).
	ColorISMCs.SetNum(ColorPalette.Num());
	for (int32 C = 0; C < ColorPalette.Num(); ++C)
	{
		UInstancedStaticMeshComponent* ISMC = NewObject<UInstancedStaticMeshComponent>(World);
		ISMC->RegisterComponentWithWorld(World);
		ISMC->SetMobility(EComponentMobility::Movable);
		ISMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ISMC->CastShadow = true;
		ISMC->SetCanEverAffectNavigation(false);
		ISMC->SetStaticMesh(Mesh);
		ColorISMCs[C] = ISMC;

		if (AgentMaterial != nullptr)
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(AgentMaterial, this);
			// Los materiales con parámetro "BaseColor" o "Tint" se tiñen; si no existe,
			// el parámetro simplemente se ignora y el MID mantiene el color del material.
			const FLinearColor& Tint = ColorPalette[C];
			MID->SetVectorParameterValue(TEXT("BaseColor"), Tint);
			MID->SetVectorParameterValue(FName("Tint"), Tint);
			ISMC->SetMaterial(0, MID);
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  CreateDefaultCapsuleMesh: crea una cápsula procedural como malla estática.
//  Port de ObtenerMeshCapsula del Unity script.
// ─────────────────────────────────────────────────────────────────────────────
UStaticMesh* UAlsasuaCrowdSubsystem::CreateDefaultHumanoidMesh() const
{
	// Humanoid procedural (sin assets): torso + cabeza + brazos + piernas de cajas,
	// con origen en el centro (suelo en Z=-87.5, coronilla en Z=+87.5 → ~175cm).
	// Reemplaza el cilindro por defecto para que la multitud lea como gente.
	FMeshDescription MeshDesc;
	FMeshDescriptionBuilder Builder;
	Builder.SetMeshDescription(&MeshDesc);
	Builder.SetNumUVLayers(1);

	const FPolygonGroupID Group = Builder.AppendPolygonGroup();

	auto AppendVertexInstance = [&](FVertexID Vid, const FVector& Normal, const FVector2D& UV)
	{
		const FVertexInstanceID Iid = Builder.AppendInstance(Vid);
		Builder.SetInstanceNormal(Iid, Normal);
		Builder.SetInstanceUV(Iid, UV, 0);
		return Iid;
	};

	auto AppendBox = [&](const FVector& Center, const FVector& Half)
	{
		const FVector C0 = Center + FVector(-Half.X, -Half.Y, -Half.Z);
		const FVector C1 = Center + FVector(+Half.X, -Half.Y, -Half.Z);
		const FVector C2 = Center + FVector(+Half.X, +Half.Y, -Half.Z);
		const FVector C3 = Center + FVector(-Half.X, +Half.Y, -Half.Z);
		const FVector C4 = Center + FVector(-Half.X, -Half.Y, +Half.Z);
		const FVector C5 = Center + FVector(+Half.X, -Half.Y, +Half.Z);
		const FVector C6 = Center + FVector(+Half.X, +Half.Y, +Half.Z);
		const FVector C7 = Center + FVector(-Half.X, +Half.Y, +Half.Z);
		const FVertexID V[8] = {
			Builder.AppendVertex(C0), Builder.AppendVertex(C1), Builder.AppendVertex(C2), Builder.AppendVertex(C3),
			Builder.AppendVertex(C4), Builder.AppendVertex(C5), Builder.AppendVertex(C6), Builder.AppendVertex(C7),
		};
		// Caras (normal hacia fuera, 4 esquinas en orden CCW visto desde fuera).
		const int32 Faces[6][4] = {
			{ 3, 2, 6, 7 },   // +Y
			{ 0, 4, 5, 1 },   // -Y
			{ 0, 3, 7, 4 },   // -X
			{ 1, 5, 6, 2 },   // +X
			{ 0, 1, 2, 3 },   // -Z
			{ 4, 7, 6, 5 },   // +Z
		};
		const FVector Normals[6] = {
			FVector(0, 1, 0), FVector(0, -1, 0),
			FVector(-1, 0, 0), FVector(1, 0, 0),
			FVector(0, 0, -1), FVector(0, 0, 1),
		};
		for (int32 F = 0; F < 6; ++F)
		{
			FVertexInstanceID Tri[4];
			const FVector2D UVs[4] = { FVector2D(0, 0), FVector2D(1, 0), FVector2D(1, 1), FVector2D(0, 1) };
			for (int32 K = 0; K < 4; ++K)
			{
				Tri[K] = AppendVertexInstance(V[Faces[F][K]], Normals[F], UVs[K]);
			}
			Builder.AppendTriangle(Tri[0], Tri[1], Tri[2], Group);
			Builder.AppendTriangle(Tri[0], Tri[2], Tri[3], Group);
		}
	};

	// Piernas (Z 0..45).
	AppendBox(FVector(-8.f, 0.f, 22.5f), FVector(7.f, 7.f, 22.5f));
	AppendBox(FVector( 8.f, 0.f, 22.5f), FVector(7.f, 7.f, 22.5f));
	// Torso (Z 40..130).
	AppendBox(FVector(0.f, 0.f, 85.f), FVector(17.f, 10.f, 45.f));
	// Brazos (Z 42..125).
	AppendBox(FVector(-25.f, 0.f, 83.5f), FVector(6.f, 8.f, 41.5f));
	AppendBox(FVector( 25.f, 0.f, 83.5f), FVector(6.f, 8.f, 41.5f));
	// Cabeza (Z 130..175).
	AppendBox(FVector(0.f, 0.f, 152.5f), FVector(8.f, 8.f, 22.5f));

	const TArray<const FMeshDescription*> Descs = { &MeshDesc };
	UStaticMesh::FBuildMeshDescriptionsParams Params;
	Params.bFastBuild = true;
	Params.bBuildSimpleCollision = false;
	Params.bCommitMeshDescription = false;

	UStaticMesh* Mesh = NewObject<UStaticMesh>(const_cast<UAlsasuaCrowdSubsystem*>(this));
	if (!Mesh->BuildFromMeshDescriptions(Descs, Params))
	{
		// Fallback: cilindro del motor si falla la construcción.
		return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder"), nullptr, LOAD_None);
	}
	return Mesh;
}

// ─────────────────────────────────────────────────────────────────────────────
//  SpawnCrowdAgents: crea un grupo de agentes según la solicitud.
// ─────────────────────────────────────────────────────────────────────────────
int32 UAlsasuaCrowdSubsystem::SpawnCrowdAgents(const FCrowdSpawnRequest& Request)
{
	const int32 Count = FMath::Min(Request.NumAgents, MaxAgents - Agents.Num());
	if (Count <= 0)
	{
		UE_LOG(LogAlsasuaAI, Warning, TEXT("[AlsasuaCrowdSubsystem] No se pueden spawnear más agentes (límite alcanzado)."));
		return -1;
	}

	const int32 StartIndex = Agents.Num();
	Agents.Reserve(Agents.Num() + Count);
	AgentColors.Reserve(AgentColors.Num() + Count);

	// Calcular dirección forward de la ruta.
	FVector ForwardDir = FVector::ForwardVector;
	if (Request.RoutePoints.Num() >= 2)
	{
		ForwardDir = (Request.RoutePoints[1] - Request.RoutePoints[0]).GetSafeNormal();
	}

	// Actualizar waypoints globales si se proporcionaron.
	if (Request.RoutePoints.Num() > 0)
	{
		RouteWaypoints = Request.RoutePoints;
	}

	// Crear agentes en formación (port de InicializarAgentes del Unity script).
	InitializeAgentsInFormation(StartIndex, Count, Request.SpawnCenter,
		ForwardDir, Request.FlockingParams);

	// Reconstruir el grid espacial.
	RebuildSpatialGrid();

	// Emitir delegado.
	OnAgentCountChanged.Broadcast(Agents.Num());

	return StartIndex;
}

// ─────────────────────────────────────────────────────────────────────────────
//  InitializeAgentsInFormation: posiciona agentes en formación de columna.
//  Port directo de InicializarAgentes del Unity script.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaCrowdSubsystem::InitializeAgentsInFormation(int32 StartIndex, int32 Count,
	const FVector& Center, const FVector& ForwardDir, const FCrowdFlockingParams& Params)
{
	const FVector RightDir = FVector::CrossProduct(FVector::UpVector, ForwardDir).GetSafeNormal();
	const float HalfWidth = Params.FormationWidth * 0.5f;

	for (int32 i = 0; i < Count; ++i)
	{
		const int32 Row = i / Params.FormationWidth;
		const int32 Col = i % Params.FormationWidth;

		const float OffsetX = (Col - HalfWidth) * Params.FormationSpacingX
			+ FMath::RandRange(-12.f, 12.f);
		const float OffsetZ = -Row * Params.FormationSpacingZ
			+ FMath::RandRange(-10.f, 10.f);

		const FVector SpawnPos = Center + RightDir * OffsetX + ForwardDir * OffsetZ;

		FAcrowdAgentData Agent;
		Agent.Position = SpawnPos;
		Agent.Velocity = ForwardDir * Params.MarchSpeed;
		Agent.CurrentWaypoint = 0;
		Agent.GroundHeight = SpawnPos.Z;

		Agents.Add(Agent);

		// Color aleatorio de la paleta.
		const int32 ColorIdx = FMath::RandRange(0, ColorPalette.Num() - 1);
		AgentColors.Add(ColorPalette[ColorIdx]);
		AgentColorIdx.Add(ColorIdx);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  DespawnAllAgents: elimina todos los agentes.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaCrowdSubsystem::DespawnAllAgents()
{
	Agents.Empty();
	AgentColors.Empty();
	AgentColorIdx.Empty();
	RouteWaypoints.Empty();
	SpatialGrid.Clear();

	for (UInstancedStaticMeshComponent* ISMC : ColorISMCs)
	{
		if (ISMC != nullptr)
		{
			ISMC->ClearInstances();
		}
	}

	OnAgentCountChanged.Broadcast(0);
}

// ─────────────────────────────────────────────────────────────────────────────
//  GetAgentPosition: devuelve la posición de un agente por índice.
// ─────────────────────────────────────────────────────────────────────────────
FVector UAlsasuaCrowdSubsystem::GetAgentPosition(int32 AgentIndex) const
{
	if (Agents.IsValidIndex(AgentIndex))
	{
		return Agents[AgentIndex].Position;
	}
	return FVector::ZeroVector;
}

// ─────────────────────────────────────────────────────────────────────────────
//  SetGlobalFlockingParams: actualiza los parámetros de flocking.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaCrowdSubsystem::SetGlobalFlockingParams(const FCrowdFlockingParams& InParams)
{
	GlobalFlockingParams = InParams;
	UpdateInterval = 1.f / FMath::Max(UpdateFrequency, 1.f);
}

// ─────────────────────────────────────────────────────────────────────────────
//  TickFlocking: actualiza la lógica de boid flocking para todos los agentes.
//  Port directo del FlockingJob del Unity script.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaCrowdSubsystem::TickFlocking(float DeltaTime)
{
	const int32 NumAgents = Agents.Num();
	if (NumAgents == 0)
	{
		return;
	}

	const FCrowdFlockingParams& P = GlobalFlockingParams;
	const float RadSepSq = P.SeparationRadius * P.SeparationRadius;
	const float RadCohSq = P.CohesionRadius * P.CohesionRadius;
	const float RadAliSq = P.AlignmentRadius * P.AlignmentRadius;
	const float MaxSpeed = P.MarchSpeed * 1.8f;

	// Copiar posiciones actuales para double-buffer.
	TArray<FAcrowdAgentData> PreviousState = Agents;

	TArray<int32> Neighbors;
	Neighbors.Reserve(64);

	for (int32 i = 0; i < NumAgents; ++i)
	{
		FAcrowdAgentData& Ag = Agents[i];

		// Saltar agentes muertos.
		if (!Ag.bAlive)
		{
			continue;
		}

		const FVector& Pos = PreviousState[i].Position;

		// ── 1. Fuerza de ruta ───────────────────────────────────────────────
		FVector ForceRoute = FVector::ZeroVector;
		FVector WpPos = Pos;

		if (RouteWaypoints.IsValidIndex(Ag.CurrentWaypoint))
		{
			WpPos = RouteWaypoints[Ag.CurrentWaypoint];
			WpPos.Z = Pos.Z;
			const FVector DirWP = WpPos - Pos;
			const float DistWP = DirWP.Size();

			if (DistWP < 300.f) // 3m → waypoint alcanzado
			{
				Ag.CurrentWaypoint = (Ag.CurrentWaypoint + 1) % RouteWaypoints.Num();
			}

			if (DistWP > 5.f)
			{
				ForceRoute = (DirWP / DistWP) * P.MarchSpeed * P.RouteWeight;
			}
		}

		// ── 2. Spatial Hash Query: Separación / Cohesión / Alineación ───────
		FVector ForceSep = FVector::ZeroVector;
		FVector CohSum = FVector::ZeroVector;
		FVector AliSum = FVector::ZeroVector;
		int32 NCoh = 0;
		int32 NAli = 0;

		SpatialGrid.QueryNeighbors(Neighbors, Pos, P.AlignmentRadius);

		for (int32 Idx : Neighbors)
		{
			if (Idx == i) continue;

			const FVector Delta = FVector(Pos.X - PreviousState[Idx].Position.X,
				Pos.Y - PreviousState[Idx].Position.Y, 0.f);
			const float D2 = Delta.SizeSquared();

			if (D2 < RadSepSq && D2 > 0.01f)
			{
				ForceSep += Delta / FMath::Sqrt(D2);
			}
			if (D2 < RadCohSq)
			{
				CohSum += PreviousState[Idx].Position;
				++NCoh;
			}
			if (D2 < RadAliSq)
			{
				AliSum += PreviousState[Idx].Velocity;
				++NAli;
			}
		}

		const FVector ForceCoh = (NCoh > 0)
			? ((CohSum / static_cast<float>(NCoh)) - Pos).GetSafeNormal() * P.CohesionWeight
			: FVector::ZeroVector;

		const FVector ForceAli = (NAli > 0)
			? (AliSum / static_cast<float>(NAli)).GetSafeNormal() * P.AlignmentWeight
			: FVector::ZeroVector;

		// ── 3. Integración de física ────────────────────────────────────────
		FVector Acceleration = ForceRoute + ForceSep * P.SeparationWeight + ForceCoh + ForceAli;
		Acceleration.Z = 0.f;

		Ag.Velocity += Acceleration * DeltaTime;
		Ag.Velocity.Z = 0.f;

		const float Speed = Ag.Velocity.Size();
		if (Speed > MaxSpeed)
		{
			Ag.Velocity *= (MaxSpeed / Speed);
		}
		else if (Speed < 5.f && RouteWaypoints.IsValidIndex(Ag.CurrentWaypoint))
		{
			const FVector ToWp = (RouteWaypoints[Ag.CurrentWaypoint] - Pos).GetSafeNormal();
			Ag.Velocity = ToWp * P.MarchSpeed * 0.5f;
		}

		Ag.Position = Pos + Ag.Velocity * DeltaTime;
		Ag.Position.Z = Ag.GroundHeight;
	}

	// Reconstruir el grid con las nuevas posiciones.
	RebuildSpatialGrid();
}

// ─────────────────────────────────────────────────────────────────────────────
//  RebuildSpatialGrid: limpia y reinserta todos los agentes.
//  Port de ActualizarGridSpatialMainThread del Unity script.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaCrowdSubsystem::RebuildSpatialGrid()
{
	SpatialGrid.Clear();

	for (int32 i = 0; i < Agents.Num(); ++i)
	{
		SpatialGrid.Insert(i, Agents[i].Position);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  SampleGroundHeight: line trace hacia abajo para encontrar el suelo.
//  Port de MuestrearSueloPorTurno del Unity script (un agente por tick).
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaCrowdSubsystem::SampleGroundHeight(int32 AgentIndex)
{
	UWorld* World = GetWorld();
	if (World == nullptr || !Agents.IsValidIndex(AgentIndex))
	{
		return;
	}

	FAcrowdAgentData& Ag = Agents[AgentIndex];

	// Saltar agentes muertos.
	if (!Ag.bAlive)
	{
		return;
	}

	const FVector Origen = Ag.Position + FVector(0.f, 0.f, 800.f); // 8m arriba
	const FVector Destino = Ag.Position - FVector(0.f, 0.f, 3000.f); // 30m abajo

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(nullptr);

	if (World->LineTraceSingleByChannel(Hit, Origen, Destino, ECC_Visibility, Params))
	{
		Ag.GroundHeight = Hit.ImpactPoint.Z;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  SyncInstancedTransforms: actualiza las transformaciones del ISMC.
//  Port de RenderizarGPUInstanced / ActualizarMatricesYPancarta.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaCrowdSubsystem::SyncInstancedTransforms()
{
	if (Agents.Num() == 0 || ColorISMCs.IsEmpty())
	{
		return;
	}

	// Reconstruir si el número de agentes cambió (cada agente va a su ISMC de color).
	if (AgentInstanceIndex.Num() != Agents.Num())
	{
		RebuildAllInstances();
		return;
	}

	// Actualización incremental: mover cada agente dentro de su ISMC de color.
	for (int32 i = 0; i < Agents.Num(); ++i)
	{
		const FAcrowdAgentData& Ag = Agents[i];
		const int32 ColorIdx = AgentColorIdx.IsValidIndex(i) ? AgentColorIdx[i] : 0;
		UInstancedStaticMeshComponent* ISMC =
			ColorISMCs.IsValidIndex(ColorIdx) ? ColorISMCs[ColorIdx] : nullptr;
		if (ISMC == nullptr)
		{
			continue;
		}

		if (!Ag.bAlive)
		{
			const FTransform HiddenTransform(FRotator::ZeroRotator, FVector(0, 0, -10000), FVector::ZeroVector);
			ISMC->UpdateInstanceTransform(AgentInstanceIndex[i], HiddenTransform, true, true, true);
			continue;
		}

		FRotator Rotation = FRotator::ZeroRotator;
		if (Ag.Velocity.SizeSquared() > 0.01f)
		{
			Rotation = Ag.Velocity.ToOrientationRotator();
		}

		const FVector Location = Ag.Position + FVector(0.f, 0.f, VisualOffsetZ);
		const FTransform Transform(Rotation, Location, AgentScale);

		ISMC->UpdateInstanceTransform(AgentInstanceIndex[i], Transform, true, true, true);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  RebuildAllInstances: reconstruye los ISMCs de color (clear + re-add).
//  Cada agente se instancia en el ISMC de su color con su índice de color.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaCrowdSubsystem::RebuildAllInstances()
{
	for (UInstancedStaticMeshComponent* ISMC : ColorISMCs)
	{
		if (ISMC != nullptr)
		{
			ISMC->ClearInstances();
		}
	}

	AgentInstanceIndex.SetNum(Agents.Num());

	for (int32 i = 0; i < Agents.Num(); ++i)
	{
		const FAcrowdAgentData& Ag = Agents[i];
		const int32 ColorIdx = AgentColorIdx.IsValidIndex(i) ? AgentColorIdx[i] : 0;
		UInstancedStaticMeshComponent* ISMC =
			ColorISMCs.IsValidIndex(ColorIdx) ? ColorISMCs[ColorIdx] : nullptr;
		if (ISMC == nullptr)
		{
			AgentInstanceIndex[i] = INDEX_NONE;
			continue;
		}

		FTransform Transform;
		if (!Ag.bAlive)
		{
			Transform = FTransform(FRotator::ZeroRotator, FVector(0, 0, -10000), FVector::ZeroVector);
		}
		else
		{
			FRotator Rotation = FRotator::ZeroRotator;
			if (Ag.Velocity.SizeSquared() > 0.01f)
			{
				Rotation = Ag.Velocity.ToOrientationRotator();
			}
			const FVector Location = Ag.Position + FVector(0.f, 0.f, VisualOffsetZ);
			Transform = FTransform(Rotation, Location, AgentScale);
		}

		AgentInstanceIndex[i] = ISMC->AddInstance(Transform, true);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  AssignAgentColors: asigna colores de la paleta a cada agente.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaCrowdSubsystem::AssignAgentColors()
{
	// Los colores ya se asignan en InitializeAgentsInFormation.
	// Este método existe para re-asignar si es necesario (ej. al cambio de escenario).
	AgentColors.SetNum(Agents.Num());
	AgentColorIdx.SetNum(Agents.Num());
	for (int32 i = 0; i < AgentColors.Num(); ++i)
	{
		const int32 ColorIdx = FMath::RandRange(0, ColorPalette.Num() - 1);
		AgentColors[i] = ColorPalette[ColorIdx];
		AgentColorIdx[i] = ColorIdx;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  InternalTick: método registrado en el TimerManager (~30Hz).
//  Orquesta el flocking, muestreo de suelo e ISMC.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaCrowdSubsystem::InternalTick()
{
	if (Agents.Num() == 0)
	{
		return;
	}

	// 1. Muestrear suelo por turno (solo agentes vivos).
	const int32 NumAlive = GetAliveAgentCount();
	if (NumAlive > 0)
	{
		GroundRaycastIndex = (GroundRaycastIndex + 1) % Agents.Num();
		while (Agents.IsValidIndex(GroundRaycastIndex) && !Agents[GroundRaycastIndex].bAlive)
		{
			GroundRaycastIndex = (GroundRaycastIndex + 1) % Agents.Num();
		}
		if (Agents.IsValidIndex(GroundRaycastIndex))
		{
			SampleGroundHeight(GroundRaycastIndex);
		}
	}

	// 2. Ejecutar flocking.
	TickFlocking(UpdateInterval);

	// 3. Sincronizar ISMC.
	SyncInstancedTransforms();
}

// ─────────────────────────────────────────────────────────────────────────────
//  KillAgent: mata un agente, spawnea ragdoll pooled según distancia al jugador
//  y lo elimina del flocking.
//
//  3 tiers de ragdoll:
//   · Full   (< RagdollFullDistance)   → física completa con skeletal mesh
//   · Frozen (< RagdollFrozenDistance) → impulso breve + freeze en pose
//   · None   (> RagdollFrozenDistance) → sin ragdoll, solo desaparece ISMC
//
//  Si MaxActiveRagdolls se supera, se recicla el ragdoll más antiguo.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaCrowdSubsystem::KillAgent(int32 AgentIndex, FVector DeathImpulse)
{
	if (!Agents.IsValidIndex(AgentIndex))
	{
		return;
	}

	FAcrowdAgentData& Ag = Agents[AgentIndex];
	if (!Ag.bAlive)
	{
		return;
	}

	Ag.bAlive = false;
	Ag.Velocity = FVector::ZeroVector;

	// ── Determinar distancia al jugador y tier de ragdoll ───────────────────
	ERagdollQuality Quality = ERagdollQuality::None;

	if (RagdollActorClass != nullptr)
	{
		float DistToPlayer = MAX_FLT;

		if (const UWorld* World = GetWorld())
		{
			if (const APlayerController* PC = World->GetFirstPlayerController())
			{
				if (const APawn* P = PC->GetPawn())
				{
					DistToPlayer = FVector::Dist(Ag.Position, P->GetActorLocation());
				}
			}
		}

		if (DistToPlayer < RagdollFullDistance)
		{
			Quality = ERagdollQuality::Full;
		}
		else if (DistToPlayer < RagdollFrozenDistance)
		{
			Quality = ERagdollQuality::Frozen;
		}
	}

	// ── Reciclar ragdoll más antiguo si superamos el límite ─────────────────
	if (Quality != ERagdollQuality::None && ActiveRagdolls.Num() >= MaxActiveRagdolls)
	{
		ACrowdRagdollActor* Oldest = nullptr;
		for (int32 i = 0; i < ActiveRagdolls.Num(); ++i)
		{
			if (ActiveRagdolls[i] != nullptr)
			{
				Oldest = ActiveRagdolls[i];
				ActiveRagdolls.RemoveAt(i);
				break;
			}
		}
		if (Oldest != nullptr)
		{
			Oldest->DeactivateForPool();
		}
	}

	// ── Spawn y activación del ragdoll ──────────────────────────────────────
	if (Quality != ERagdollQuality::None)
	{
		UWorld* World = GetWorld();
		if (World != nullptr)
		{
			AActor* Ragdoll = nullptr;

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Ragdoll = World->SpawnActor<AActor>(RagdollActorClass, Ag.Position,
				DeathImpulse.ToOrientationRotator(), Params);

			if (ACrowdRagdollActor* RagdollChar = Cast<ACrowdRagdollActor>(Ragdoll))
			{
				const float Strength = FMath::Clamp(DeathImpulse.Size(), 300.f, 1200.f);
				RagdollChar->ActivateRagdoll(DeathImpulse.GetSafeNormal(), Strength, Quality);
				ActiveRagdolls.Add(RagdollChar);
			}
		}
	}

	// ── Ocultar ISMC inmediatamente ─────────────────────────────────────────
	{
		const int32 ColorIdx = AgentColorIdx.IsValidIndex(AgentIndex) ? AgentColorIdx[AgentIndex] : 0;
		const int32 InstanceIdx = AgentInstanceIndex.IsValidIndex(AgentIndex) ? AgentInstanceIndex[AgentIndex] : INDEX_NONE;
		UInstancedStaticMeshComponent* ISMC =
			ColorISMCs.IsValidIndex(ColorIdx) ? ColorISMCs[ColorIdx] : nullptr;
		if (ISMC != nullptr && InstanceIdx != INDEX_NONE && ISMC->GetInstanceCount() > InstanceIdx)
		{
			ISMC->UpdateInstanceTransform(InstanceIdx,
				FTransform(FRotator::ZeroRotator, FVector(0, 0, -10000), FVector::ZeroVector),
				true, true, true);
		}
	}

	OnAgentKilled.Broadcast(AgentIndex, Ag.Position);
	OnAgentCountChanged.Broadcast(GetAliveAgentCount());
}

// ─────────────────────────────────────────────────────────────────────────────
//  DespawnAgent: elimina un agente silenciosamente (sin ragdoll).
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaCrowdSubsystem::DespawnAgent(int32 AgentIndex)
{
	if (!Agents.IsValidIndex(AgentIndex))
	{
		return;
	}

	Agents[AgentIndex].bAlive = false;
	Agents[AgentIndex].Velocity = FVector::ZeroVector;

	{
		const int32 ColorIdx = AgentColorIdx.IsValidIndex(AgentIndex) ? AgentColorIdx[AgentIndex] : 0;
		const int32 InstanceIdx = AgentInstanceIndex.IsValidIndex(AgentIndex) ? AgentInstanceIndex[AgentIndex] : INDEX_NONE;
		UInstancedStaticMeshComponent* ISMC =
			ColorISMCs.IsValidIndex(ColorIdx) ? ColorISMCs[ColorIdx] : nullptr;
		if (ISMC != nullptr && InstanceIdx != INDEX_NONE && ISMC->GetInstanceCount() > InstanceIdx)
		{
			ISMC->UpdateInstanceTransform(InstanceIdx,
				FTransform(FRotator::ZeroRotator, FVector(0, 0, -10000), FVector::ZeroVector),
				true, true, true);
		}
	}

	OnAgentCountChanged.Broadcast(GetAliveAgentCount());
}

// ─────────────────────────────────────────────────────────────────────────────
//  GetAliveAgentCount: devuelve cuántos agentes vivos hay.
// ─────────────────────────────────────────────────────────────────────────────
int32 UAlsasuaCrowdSubsystem::GetAliveAgentCount() const
{
	int32 Count = 0;
	for (const FAcrowdAgentData& Ag : Agents)
	{
		if (Ag.bAlive)
		{
			++Count;
		}
	}
	return Count;
}

// ─────────────────────────────────────────────────────────────────────────────
//  CleanupDeadAgents: elimina los agentes muertos del array y compacta.
//  Se ejecuta periódicamente para evitar degradación del ISMC y spatial grid.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaCrowdSubsystem::CleanupDeadAgents()
{
	const int32 BeforeCount = Agents.Num();
	int32 WriteIdx = 0;

	for (int32 ReadIdx = 0; ReadIdx < BeforeCount; ++ReadIdx)
	{
		if (Agents[ReadIdx].bAlive)
		{
			if (WriteIdx != ReadIdx)
			{
				Agents[WriteIdx] = Agents[ReadIdx];
				if (AgentColors.IsValidIndex(ReadIdx))
				{
					AgentColors[WriteIdx] = AgentColors[ReadIdx];
				}
				if (AgentColorIdx.IsValidIndex(ReadIdx))
				{
					AgentColorIdx[WriteIdx] = AgentColorIdx[ReadIdx];
				}
			}
			++WriteIdx;
		}
	}

	const int32 RemovedCount = BeforeCount - WriteIdx;
	if (RemovedCount > 0)
	{
		Agents.SetNum(WriteIdx);
		AgentColors.SetNum(WriteIdx);
		AgentColorIdx.SetNum(WriteIdx);

		// Reconstruir spatial grid con los agentes restantes.
		RebuildSpatialGrid();

		// Reconstruir ISMCs de color con los agentes compactados.
		RebuildAllInstances();

		OnAgentCountChanged.Broadcast(Agents.Num());
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  OnDeadAgentCleanupTick: callback del timer de limpieza periódica.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaCrowdSubsystem::OnDeadAgentCleanupTick()
{
	// Solo limpiar si hay suficientes muertos para justificar la compactación.
	const int32 Total = Agents.Num();
	int32 DeadCount = 0;
	for (const FAcrowdAgentData& Ag : Agents)
	{
		if (!Ag.bAlive)
		{
			++DeadCount;
		}
	}

	// Compactar si hay más de 10 muertos o >20% muertos.
	if (DeadCount > 10 || (Total > 0 && DeadCount > Total * 0.2f))
	{
		CleanupDeadAgents();
	}
}

void UAlsasuaCrowdSubsystem::HandleTacticChanged(EAlsasuaTactic NewTactic)
{
	FCrowdFlockingParams P = GlobalFlockingParams;
	switch (NewTactic)
	{
	case EAlsasuaTactic::March:
		P.RouteWeight = 3.0f;
		P.SeparationWeight = 2.2f;
		P.MarchSpeed = 130.f;
		break;
	case EAlsasuaTactic::Blockade:
		P.RouteWeight = 0.0f;
		P.MarchSpeed = 0.f;
		P.SeparationWeight = 1.0f;
		P.CohesionWeight = 1.5f;
		break;
	case EAlsasuaTactic::Scatter:
		P.RouteWeight = 0.0f;
		P.SeparationWeight = 5.0f;
		P.CohesionWeight = 0.0f;
		P.MarchSpeed = 200.f;
		break;
	case EAlsasuaTactic::SitIn:
		P.RouteWeight = 0.0f;
		P.MarchSpeed = 0.f;
		P.AlignmentWeight = 0.5f;
		P.CohesionWeight = 1.2f;
		break;
	}
	SetGlobalFlockingParams(P);
}
