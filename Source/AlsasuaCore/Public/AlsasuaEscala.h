// AlsasuaEscala.h (capa CORE)
// Factor de escala de la multitud y el tráfico según el perfil gráfico, para que
// una GPU justa (8 GB) no cargue con los mismos 600 peatones y 25 coches que una
// de gama alta. Sin tocar nada coincide con el comportamiento de siempre: el
// perfil por defecto (Ultra) da 1.0.
#pragma once

#include "CoreMinimal.h"
#include "Misc/ConfigCacheIni.h"

namespace AlsasuaEscala
{
	// Perfil 0..3 (Low, Med, High, Ultra). Es el mismo PerfilArranque que lee
	// UAlsasuaGraphicsSettingsSubsystem; la sección es la de ese subsistema (GF_World).
	// Se lee aquí a mano porque AlsasuaCore no puede depender de GF_World.
	inline float Factor()
	{
		static const float F = []()
		{
			// Override explícito: [/Script/AlsasuaCore.AlsasuaEscala] EscalaMultitud=0.5
			float Manual = -1.f;
			if (GConfig && GConfig->GetFloat(TEXT("/Script/AlsasuaCore.AlsasuaEscala"),
				TEXT("EscalaMultitud"), Manual, GGameIni) && Manual > 0.f)
			{
				return FMath::Clamp(Manual, 0.05f, 1.f);
			}
			int32 Perfil = 3;
			if (GConfig)
			{
				GConfig->GetInt(TEXT("/Script/GF_World.AlsasuaGraphicsSettingsSubsystem"),
					TEXT("PerfilArranque"), Perfil, GGameIni);
			}
			static const float PorPerfil[] = { 0.25f, 0.5f, 0.75f, 1.f };
			return PorPerfil[FMath::Clamp(Perfil, 0, 3)];
		}();
		return F;
	}

	// Escala un máximo. Un máximo positivo nunca baja de 1: 0 sí se queda en 0.
	inline int32 Escalar(int32 Max)
	{
		return Max > 0 ? FMath::Max(1, FMath::RoundToInt32(Max * Factor())) : Max;
	}
}
