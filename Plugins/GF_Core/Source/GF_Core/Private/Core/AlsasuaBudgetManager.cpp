#include "Core/AlsasuaBudgetManager.h"

void UAlsasuaBudgetManager::Tick(float DeltaTime)
{
	// Reset de contadores para el nuevo frame
	CurrentFrameUsage.Empty();

	// Calculamos el presupuesto disponible basándonos en la salud real del motor
	AvgFrameTime = FMath::Lerp(AvgFrameTime, (double)DeltaTime * 1000.0, 0.1);

	// Si el motor va lento, reducimos agresivamente el presupuesto de tareas Background
	if (AvgFrameTime > TotalFrameBudgetMS * 0.9)
	{
		// Modo Estrangulamiento: Reservamos el 80% del tiempo solo para lo CRÍTICO
		CurrentFrameUsage.Add(EBudgetCategory::Background, TotalFrameBudgetMS); 
	}
}

bool UAlsasuaBudgetManager::CanExecute(EBudgetCategory Category) const
{
	if (Category == EBudgetCategory::Critical) return true;

	double Used = CurrentFrameUsage.Contains(Category) ? CurrentFrameUsage[Category] : 0.0;

	// Lógica de prioridad: Las tareas de simulación pueden usar hasta el 40% del frame
	if (Category == EBudgetCategory::Simulation) return Used < (TotalFrameBudgetMS * 0.4);

	return Used < (TotalFrameBudgetMS * 0.15);
}

void UAlsasuaBudgetManager::ReportPerformance(EBudgetCategory Category, double TimeSpentMS)
{
	CurrentFrameUsage.FindOrAdd(Category) += TimeSpentMS;
}
