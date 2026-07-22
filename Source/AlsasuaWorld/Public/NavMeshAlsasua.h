// NavMeshAlsasua.h (capa WORLD)
// Registra al jugador como navigation invoker para que la navmesh se genere de
// forma dinámica a su alrededor (mundo de 14 km). Sin esto, MoveToActor de la
// IA no tiene sobre qué navegar. Puerto del setup de SistemaNavMesh.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "NavMeshAlsasua.generated.h"

UCLASS()
class ALSASUAWORLD_API UNavMeshAlsasua : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// Radios de generación/eliminación de tiles alrededor del jugador (cm).
	UPROPERTY(EditAnywhere, Category="Nav") float RadioGeneracion = 9000.f;   // 90 m
	UPROPERTY(EditAnywhere, Category="Nav") float RadioEliminacion = 11000.f; // 110 m

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UNavMeshAlsasua, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }
	virtual bool DoesSupportWorldType(const EWorldType::Type T) const override { return T == EWorldType::Game || T == EWorldType::PIE; }

private:
	bool bHecho = false;
};
