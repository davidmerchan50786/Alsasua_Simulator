#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "VehicleAIController.generated.h"

/**
 * Controlador AI para vehículos NPC.
 * Soporta:
 *   · Persecución de objetivo (pursuit)
 *   · Seguimiento de waypoints (autopilot)
 *   · Frenado por obstáculos
 */
UCLASS()
class ALSASUAMANIFA_API AVehicleAIController : public AAIController
{
    GENERATED_BODY()

public:
    AVehicleAIController();

    virtual void OnPossess(APawn* InPawn) override;

    /** Iniciar persecución de un objetivo. */
    UFUNCTION(BlueprintCallable, Category = "AI|Vehicle")
    void StartPursuit(APawn* TargetPawn);

    /** Detener persecución. */
    UFUNCTION(BlueprintCallable, Category = "AI|Vehicle")
    void StopPursuit();

    /** Establecer ruta de waypoints para autopilot. */
    UFUNCTION(BlueprintCallable, Category = "AI|Vehicle")
    void SetWaypoints(const TArray<FVector>& InWaypoints);

    /** Detener el vehículo en la posición actual. */
    UFUNCTION(BlueprintCallable, Category = "AI|Vehicle")
    void StopVehicle();

    /** Velocidad máxima del vehículo (se sincroniza con BaseVehicle). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Vehicle", meta = (ClampMin = "200"))
    float MaxAISpeed = 1500.f;

    /** Distancia de frenado por obstáculos (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Vehicle", meta = (ClampMin = "100"))
    float ObstacleStopDistance = 400.f;

    /** Radio de detección de obstáculos (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Vehicle", meta = (ClampMin = "100"))
    float ObstacleDetectionRadius = 150.f;

protected:
    virtual void Tick(float DeltaTime) override;

private:
    /** Bajo UPROPERTY, y no por formalismo: sin él el GC no ve este puntero,
     *  así que si el peón perseguido se destruye no se pone a null — se queda
     *  colgando. El IsValid() que hace Tick sobre un puntero ya liberado es
     *  comportamiento indefinido, y TickPursuit lo desreferencia después con
     *  sólo comprobar que no es nulo. CLAUDE.md §9. */
    UPROPERTY()
    TObjectPtr<APawn> PursuitTarget = nullptr;
    float PursuitAggression = 1.0f;

    TArray<FVector> Waypoints;
    int32 CurrentWaypointIndex = 0;

    bool bVehicleStopped = false;

    /** Lógica de persecución. */
    void TickPursuit(float DeltaTime);

    /** Lógica de autopilot por waypoints. */
    void TickAutopilot(float DeltaTime);

    /** Detección de obstáculos frente al vehículo. */
    bool ScanForObstacles() const;
};
