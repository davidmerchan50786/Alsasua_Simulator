// AlsasuaAIDriverComponent.cpp
#include "World/AlsasuaAIDriverComponent.h"
#include "World/AlsasuaRedViaria.h"
#include "World/AlsasuaDynamicTrafficSystem.h"
#include "World/AlsasuaTrafficLightSystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

namespace
{
	// Distancia a la que damos el waypoint por alcanzado y radio de la esfera
	// de barrido para detectar al vehículo de delante.
	constexpr float UmbralWaypoint = 150.f;
	constexpr float RadioVehiculo = 100.f;
	// Alcance del barrido hacia delante (semáforos y vehículos), en cm.
	constexpr float AlcanceBarrido = 500.f;

	/** Velocidad con la que aún se frena a tiempo dejando Margen antes del objetivo. */
	float VelocidadFrenado(float Distancia, float Margen, float Deceleracion)
	{
		const float Efectiva = Distancia - Margen;
		return Efectiva <= 0.f ? 0.f : FMath::Sqrt(2.f * Deceleracion * Efectiva);
	}
}

UAlsasuaAIDriverComponent::UAlsasuaAIDriverComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAlsasuaAIDriverComponent::BeginPlay()
{
	Super::BeginPlay();

	if (RoutePoints.Num() < 2)
		NuevaRutaAleatoria();
}

void UAlsasuaAIDriverComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner || RoutePoints.Num() < 2) return;

	VelocidadDeseada = MaxSpeed;

	CheckVehicleAhead(World);
	CheckTrafficLights(World);

	// Frenar para la curva del waypoint salvo que sea el último: al llegar
	// ahí ya se pide ruta nueva y se sigue con velocidad. Margen a mitad de
	// umbral para no arrastrarse en cada esquina.
	if (CurrentRouteIndex < RoutePoints.Num() - 1)
		SlowDownForTarget(FVector::Dist(Owner->GetActorLocation(), GetTargetPoint()), UmbralWaypoint * 0.5f);

	MoveAlongRoute(DeltaTime);
}

void UAlsasuaAIDriverComponent::SetDestination(const FVector& Dest)
{
	UAlsasuaRedViaria* G = ResolverGrafo();
	AActor* Owner = GetOwner();
	if (!G || G->NumNodos() < 2 || !Owner) return;

	FijarRuta(G, G->NodoMasCercano(Owner->GetActorLocation()), G->NodoMasCercano(Dest));
	if (RoutePoints.Num() > 0) RoutePoints.Last() = Dest;
}

void UAlsasuaAIDriverComponent::FijarRuta(UAlsasuaRedViaria* Red, int32 StartNode, int32 EndNode)
{
	RoutePoints.Empty();
	CurrentRouteIndex = 0;
	bWaitingAtLight = false;
	Grafo = Red;
	if (!Red) return;

	// Ruta() devuelve TRAMOS, no nodos: un solo tramo ya es ruta válida.
	const TArray<int32> Camino = Red->Ruta(StartNode, EndNode);
	if (Camino.Num() < 1) return;

	RoutePoints = Red->PuntosDeRuta(Camino);

	// Empezar por el punto más cercano, no desde detrás del coche.
	const FVector Loc = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
	float MejorDistSq = TNumericLimits<float>::Max();
	for (int32 i = 0; i < RoutePoints.Num(); ++i)
	{
		const float DistSq = FVector::DistSquared(RoutePoints[i], Loc);
		if (DistSq < MejorDistSq)
		{
			MejorDistSq = DistSq;
			CurrentRouteIndex = i;
		}
	}
	if (CurrentRouteIndex < RoutePoints.Num() - 1) ++CurrentRouteIndex;
}

FVector UAlsasuaAIDriverComponent::GetTargetPoint() const
{
	return RoutePoints[FMath::Min(CurrentRouteIndex, RoutePoints.Num() - 1)];
}

void UAlsasuaAIDriverComponent::MoveAlongRoute(float DeltaTime)
{
	AActor* Owner = GetOwner();
	if (!Owner || RoutePoints.Num() < 2) return;

	FVector Loc = Owner->GetActorLocation();
	FVector AlPunto = GetTargetPoint() - Loc;
	float Distancia = AlPunto.Size();

	// Waypoint alcanzado: avanzar (varios si venían muy juntos).
	while (Distancia < UmbralWaypoint && CurrentRouteIndex < RoutePoints.Num() - 1)
	{
		++CurrentRouteIndex;
		AlPunto = GetTargetPoint() - Loc;
		Distancia = AlPunto.Size();
	}

	// Último punto: destino cumplido, pedir otro.
	if (CurrentRouteIndex >= RoutePoints.Num() - 1 && Distancia < UmbralWaypoint)
	{
		NuevaRutaAleatoria();
		if (RoutePoints.Num() < 2) return;
		AlPunto = GetTargetPoint() - Loc;
		Distancia = AlPunto.Size();
	}

	const FVector Direccion = AlPunto.GetSafeNormal();

	// Acelerar / frenar hacia la velocidad deseada del tick.
	if (CurrentSpeed < VelocidadDeseada)
		CurrentSpeed = FMath::Min(CurrentSpeed + Acceleration * DeltaTime, VelocidadDeseada);
	else
		CurrentSpeed = FMath::Max(CurrentSpeed - Deceleration * DeltaTime, VelocidadDeseada);

	Loc += Direccion * FMath::Min(CurrentSpeed * DeltaTime, Distancia);
	Owner->SetActorLocation(Loc);
	Owner->SetActorRotation(Direccion.Rotation());
}

void UAlsasuaAIDriverComponent::CheckTrafficLights(const UWorld* World)
{
	bWaitingAtLight = false;

	UGameInstance* GI = World->GetGameInstance();
	if (!GI) return;
	const UAlsasuaTrafficLightSystem* Luces = GI->GetSubsystem<UAlsasuaTrafficLightSystem>();
	if (!Luces || Luces->GetSemaforos().Num() == 0) return;

	AActor* Owner = GetOwner();
	if (!Owner) return;

	const FVector Loc = Owner->GetActorLocation();
	const FVector Adelante = Owner->GetActorForwardVector().GetSafeNormal2D();

	// El semáforo más cercano por delante dentro del alcance.
	const FTrafficLight* Cercano = nullptr;
	float MejorDistSq = AlcanceBarrido * AlcanceBarrido;
	for (const FTrafficLight& L : Luces->GetSemaforos())
	{
		const FVector HaciaLuz = L.Posicion - Loc;
		const float DistSq = HaciaLuz.SizeSquared();
		if (DistSq > MejorDistSq) continue;
		if (FVector::DotProduct(Adelante, HaciaLuz.GetSafeNormal2D()) < 0.5f) continue; // no está delante

		Cercano = &L;
		MejorDistSq = DistSq;
	}
	if (!Cercano) return;

	const float Distancia = FMath::Sqrt(MejorDistSq);

	// La fase vigente la mantiene Aplicar() a 4 Hz; no recalculamos FaseEn.
	if (Cercano->bActivo && Cercano->Fase == EFaseSemaforo::Rojo)
	{
		SlowDownForTarget(Distancia, StopAtRedDistance);
		bWaitingAtLight = CurrentSpeed < 10.f && Distancia < StopAtRedDistance * 1.5f;
	}
}

void UAlsasuaAIDriverComponent::CheckVehicleAhead(const UWorld* World)
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	const FVector Inicio = Owner->GetActorLocation();
	const FVector Fin = Inicio + Owner->GetActorForwardVector().GetSafeNormal2D() * AlcanceBarrido;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(AIDriverAhead));
	Params.AddIgnoredActor(Owner);

	FHitResult Hit;
	const bool bGolpe = World->SweepSingleByObjectType(
		Hit, Inicio, Fin,
		FQuat::Identity,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllDynamicObjects),
		FCollisionShape::MakeSphere(RadioVehiculo),
		Params);

	if (!bGolpe) return;

	// ponytail: sin memoria de atasco; dos coches cara a cara en el mismo
	// carril pueden quedar bloqueados. Añadir re-ruta si pasa en juego.
	SlowDownForTarget(Hit.Distance, FollowingDistance);
}

void UAlsasuaAIDriverComponent::SlowDownForTarget(float Distance, float BrakingDist)
{
	VelocidadDeseada = FMath::Min(VelocidadDeseada, VelocidadFrenado(Distance, BrakingDist, Deceleration));
}

UAlsasuaRedViaria* UAlsasuaAIDriverComponent::ResolverGrafo()
{
	if (Grafo) return Grafo;

	UWorld* World = GetWorld();
	if (!World) return nullptr;

	// La red es un UWorldSubsystem, así que se pide al mundo y no al sistema de
	// tráfico: el de peatones necesita la misma red y no pasa por él.
	Grafo = World->GetSubsystem<UAlsasuaRedViaria>();
	if (Grafo) Grafo->Construir();   // idempotente
	return Grafo;
}

void UAlsasuaAIDriverComponent::NuevaRutaAleatoria()
{
	UAlsasuaRedViaria* G = ResolverGrafo();
	AActor* Owner = GetOwner();
	if (!G || G->NumNodos() < 2 || !Owner) return;

	const int32 Inicio = G->NodoMasCercano(Owner->GetActorLocation());

	// Sembrado por el nodo de partida, no por FMath::RandRange: con azar de
	// reloj el tráfico sale distinto en cada arranque y el CSV de perfilado
	// deja de ser comparable entre sesiones.
	FRandomStream Sorteo(Inicio * 2654435761u + 0xD5E7u);
	for (int32 Intento = 0; Intento < 8; ++Intento)
	{
		int32 Fin = Sorteo.RandRange(0, G->NumNodos() - 1);
		if (Fin == Inicio) { Fin = (Inicio + 1) % G->NumNodos(); }
		FijarRuta(G, Inicio, Fin);
		if (RoutePoints.Num() >= 2) return;
	}
}
