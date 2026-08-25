// SecurityRadarComponent.cpp
// Escaneo por solapamiento de esfera filtrando etiquetas de actor. El nivel de
// amenaza sale del conteo y solo se emite el delegado cuando el nivel cambia,
// para no inundar a los suscriptores con el mismo aviso cada dos segundos.
#include "Systemics/Megasion/SecurityRadarComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
	// Umbrales de conteo → nivel: 1-3 sospechoso, 4-7 alerta, 8+ peligro.
	constexpr int32 UmbralAlerta = 4;
	constexpr int32 UmbralPeligro = 8;
}

USecurityRadarComponent::USecurityRadarComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USecurityRadarComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USecurityRadarComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TiempoDesdeScan += DeltaTime;
	if (TiempoDesdeScan >= ScanInterval)
	{
		TiempoDesdeScan = 0.f;
		ScanForThreats();
	}
}

void USecurityRadarComponent::ScanForThreats()
{
	const AActor* Propietario = GetOwner();
	UWorld* Mundo = GetWorld();
	if (!Propietario || !Mundo)
	{
		return;
	}

	Amenazas.Empty();

	FCollisionObjectQueryParams Objetos(FCollisionObjectQueryParams::InitType::AllDynamicObjects);
	FCollisionShape Forma = FCollisionShape::MakeSphere(DetectionRadius);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SecurityRadar), /*bTraceComplex*/ false);

	TArray<FOverlapResult> Solapes;
	if (Mundo->OverlapMultiByObjectType(Solapes, Propietario->GetActorLocation(), FQuat::Identity, Objetos, Forma, Params))
	{
		for (const FOverlapResult& S : Solapes)
		{
			AActor* Candidato = S.GetActor();
			if (!Candidato || Amenazas.Contains(Candidato))
			{
				continue;
			}
			for (const FName& Etiqueta : ThreatTags)
			{
				if (Candidato->ActorHasTag(Etiqueta))
				{
					Amenazas.Add(Candidato);
					break; // una coincidencia basta por actor
				}
			}
		}
	}

	const int32 NuevoNivel =
		Amenazas.Num() == 0 ? 0 :
		Amenazas.Num() < UmbralAlerta ? 1 :
		Amenazas.Num() < UmbralPeligro ? 2 : 3;

	if (NuevoNivel != ThreatLevel)
	{
		ThreatLevel = NuevoNivel;
		OnThreatDetected.Broadcast(ThreatLevel, DescribirNivel(ThreatLevel));
	}
}

AActor* USecurityRadarComponent::GetClosestThreat() const
{
	const AActor* Propietario = GetOwner();
	if (!Propietario)
	{
		return nullptr;
	}

	AActor* MasCercana = nullptr;
	float MejorDist = TNumericLimits<float>::Max();
	const FVector Origen = Propietario->GetActorLocation();

	for (const TObjectPtr<AActor>& Amenaza : Amenazas)
	{
		AActor* Candidata = Amenaza.Get();
		if (!IsValid(Candidata))
		{
			continue;
		}
		const float Dist = FVector::DistSquared(Origen, Candidata->GetActorLocation());
		if (Dist < MejorDist)
		{
			MejorDist = Dist;
			MasCercana = Candidata;
		}
	}
	return MasCercana;
}

FString USecurityRadarComponent::DescribirNivel(int32 Nivel) const
{
	switch (Nivel)
	{
	case 0: return TEXT("Calma: sin amenazas en el radar");
	case 1: return FString::Printf(TEXT("Sospechoso: %d amenaza(s) cerca"), Amenazas.Num());
	case 2: return FString::Printf(TEXT("Alerta: %d amenazas cerca"), Amenazas.Num());
	default: return FString::Printf(TEXT("PELIGRO: %d amenazas rodeándote"), Amenazas.Num());
	}
}
