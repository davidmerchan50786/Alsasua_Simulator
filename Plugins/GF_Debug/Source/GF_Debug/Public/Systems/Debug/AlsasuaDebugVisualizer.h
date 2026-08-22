#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "AlsasuaDebugVisualizer.generated.h"

class UAlsasuaDynamicTrafficSystem;
class UAlsasuaTrafficLightSystem;

/**
 * Visualización de depuración a 2 Hz sobre el mundo con DrawDebug*.
 * Todo se activa por consola: DebugShowTraffic / DebugHideTraffic,
 * DebugShowLights / DebugHideLights, DebugShowBiomes / DebugHideBiomes,
 * DebugShowChunks / DebugHideChunks y DebugShowVehicles / DebugHideVehicles.
 *
 * Los subsistemas de tráfico se consultan en soft: si no existen, esa capa
 * simplemente no dibuja nada.
 */
UCLASS()
class GF_DEBUG_API UAlsasuaDebugVisualizer : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// --- FTickableGameObject ---
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UAlsasuaDebugVisualizer, STATGROUP_Tickables); }
	virtual bool IsTickable() const override;

private:
	bool AlgoActivo();

	void DibujarTrafico();
	void DibujarSemaforos();
	void DibujarBiomas();
	void DibujarChunks();
	void DibujarVehiculos();

	UAlsasuaDynamicTrafficSystem* ObtenerTrafico() const;

	/** Segundos acumulados desde el último volcado; dibuja a 2 Hz. */
	float DesdeUltimaPasada = 0.f;
};
