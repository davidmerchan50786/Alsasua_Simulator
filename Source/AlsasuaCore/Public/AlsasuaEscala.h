// AlsasuaEscala.h (capa CORE)
// Perfil gráfico efectivo y factor de escala de la multitud y el tráfico. El
// perfil sale de PerfilArranque en DefaultGame.ini; con -1 (Auto) lo decide la
// VRAM dedicada de la tarjeta. Es la única fuente: lo usan el subsistema gráfico
// (GF_World) y los sistemas de multitud y tráfico, para que no discrepen.
#pragma once

#include "CoreMinimal.h"

namespace AlsasuaEscala
{
	/** 0 Low, 1 Med, 2 High, 3 Ultra. Calculado una vez por proceso. */
	ALSASUACORE_API int32 PerfilEfectivo();

	/** 0.25 / 0.5 / 0.75 / 1.0 según el perfil; override EscalaMultitud en ini. */
	ALSASUACORE_API float Factor();

	// Escala un máximo. Un máximo positivo nunca baja de 1: 0 sí se queda en 0.
	inline int32 Escalar(int32 Max)
	{
		return Max > 0 ? FMath::Max(1, FMath::RoundToInt32(Max * Factor())) : Max;
	}
}
