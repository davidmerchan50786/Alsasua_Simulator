#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "AlsasuaBudgetManager.generated.h"

/** Categorías de tareas con prioridades AAA */
UENUM(BlueprintType)
enum class EBudgetCategory : uint8
{
	Critical,    // Jugador, colisiones inmediatas
	Simulation,  // Manifestantes cercanos, IA Guardia Civil
	Background,  // Carga de texturas, manifestantes lejanos (Proxies)
	Visuals      // Efectos de partículas, detalles estéticos
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaBudgetManager : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;

	// FTickableGameObject
	virtual bool IsAllowedToTick() const override { return true; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(AlsasuaBudgetManager, STATGROUP_Game); }
	virtual bool IsTickable() const override { return !IsTemplate(); }   // no ticar el CDO (CLAUDE.md §9)

	/** Consulta si queda presupuesto de tiempo para ejecutar una tarea en este frame */
	UFUNCTION(BlueprintCallable, Category = "AAA|Budget")
	bool CanExecute(EBudgetCategory Category) const;

	/** Registra el tiempo consumido por una tarea (en microsegundos) */
	void ReportPerformance(EBudgetCategory Category, double TimeSpentMS);

private:
	// Presupuesto total por frame (ej: 16.6ms para 60fps)
	double TotalFrameBudgetMS = 16.6;

	// Mapa de tiempo consumido en el frame actual por categoría
	TMap<EBudgetCategory, double> CurrentFrameUsage;

	// Media móvil de rendimiento para predecir el siguiente frame
	double AvgFrameTime = 16.6;
};
