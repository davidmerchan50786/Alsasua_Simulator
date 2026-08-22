// AlsasuaAIDriverComponent.h (capa GAMEPLAY)
// Convierte cualquier actor vehículo en conductor con ruta: sigue puntos del
// grafo de calles, frena ante semáforos en rojo y mantiene distancia con el
// vehículo de delante. El movimiento manual de UAlsasuaDynamicTrafficSystem
// muere aquí dentro.
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaAIDriverComponent.generated.h"

class UAlsasuaRedViaria;

UCLASS(ClassGroup=(Traffic), meta=(BlueprintSpawnableComponent))
class GF_TRAFICO_API UAlsasuaAIDriverComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UAlsasuaAIDriverComponent();
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Ruta desde el nodo más cercano al owner hasta el más cercano a Dest. */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Traffic")
	void SetDestination(const FVector& Dest);

	UFUNCTION(BlueprintCallable, Category = "Alsasua|Traffic")
	void FijarRuta(UAlsasuaRedViaria* Red, int32 StartNode, int32 EndNode);

	/** cm/s */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Traffic")
	float MaxSpeed = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Traffic")
	float Acceleration = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Traffic")
	float Deceleration = 400.f;

	/** cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Traffic")
	float FollowingDistance = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Traffic")
	float StopAtRedDistance = 300.f;

	float CurrentSpeed = 0.f;

private:
	/** Red viaria en uso; se resuelve del subsistema de mundo si nadie la inyectó. */
	UPROPERTY() TObjectPtr<UAlsasuaRedViaria> Grafo = nullptr;

	TArray<FVector> RoutePoints;
	int32 CurrentRouteIndex = 0;
	bool bWaitingAtLight = false;

	/** Velocidad objetivo del tick, la van recortando semáforos y vehículos. */
	float VelocidadDeseada = 0.f;

	FVector GetTargetPoint() const;
	void MoveAlongRoute(float DeltaTime);
	void CheckTrafficLights(const UWorld* World);
	void CheckVehicleAhead(const UWorld* World);
	void SlowDownForTarget(float Distance, float BrakingDist);

	UAlsasuaRedViaria* ResolverGrafo();
	void NuevaRutaAleatoria();
};
