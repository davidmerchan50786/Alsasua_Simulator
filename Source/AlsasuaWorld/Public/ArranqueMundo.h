// ArranqueMundo.h (capa WORLD)
// Estado global del arranque del mundo. Puerto de Runtime/ArranqueMundo
// (flag BaselineListo) + gate de PantallaCarga / congelado del jugador.
#pragma once

#include "CoreMinimal.h"

namespace ArranqueMundo
{
	// true cuando hay un DirectorArranque gobernando: los cargadores no auto-cargan.
	ALSASUAWORLD_API extern bool HayDirector;

	// true cuando el mundo mínimo jugable está listo (terreno + cercanía).
	// Lo leen el ControladorJugador (congela input) y la pantalla de carga (gate).
	ALSASUAWORLD_API extern bool BaselineListo;

	// 0..1 progreso de arranque (para la barra de carga).
	ALSASUAWORLD_API extern float Progreso;

	ALSASUAWORLD_API FString GetDebugSummary();
	ALSASUAWORLD_API void SetProgress(float InProgress);
	ALSASUAWORLD_API void MarkPhaseComplete(const FString& PhaseName, float InProgress);
}
