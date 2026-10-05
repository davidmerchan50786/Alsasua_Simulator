#include "AlsasuaEscala.h"
#include "AlsasuaCore.h"
#include "Misc/ConfigCacheIni.h"
#include "RHI.h"
#include "DynamicRHI.h"

namespace
{
	// Sección del subsistema gráfico (GF_World). Se lee aquí a mano porque
	// AlsasuaCore no puede depender de GF_World.
	const TCHAR* SeccionGraficos = TEXT("/Script/GF_World.AlsasuaGraphicsSettingsSubsystem");

	int32 PerfilPorVRAM()
	{
		// Lo que limita en un PC moderado es la VRAM: texturas, Nanite, Lumen y
		// sombras virtuales viven ahí, y cuando no caben el driver pagina a RAM.
		FTextureMemoryStats Stats;
		RHIGetTextureMemoryStats(Stats);
		const double GB = double(Stats.DedicatedVideoMemory) / (1024.0 * 1024.0 * 1024.0);
		// Cortes con margen: una "8 GB" reporta ~7,9 y una "6 GB" ~5,9.
		const int32 Perfil = GB <= 0.0 ? 2      // desconocida: High, el término medio
		                   : GB < 5.0  ? 0      // 4 GB
		                   : GB < 7.5  ? 1      // 6 GB
		                   : GB < 11.0 ? 2      // 8-10 GB
		                   :             3;     // 12 GB o más
		UE_LOG(LogAlsasua, Log, TEXT("AlsasuaEscala: VRAM dedicada %.1f GB -> perfil %d."), GB, Perfil);
		return Perfil;
	}
}

int32 AlsasuaEscala::PerfilEfectivo()
{
	static const int32 P = []()
	{
		int32 Perfil = -1;
		if (GConfig) GConfig->GetInt(SeccionGraficos, TEXT("PerfilArranque"), Perfil, GGameIni);
		return Perfil < 0 ? PerfilPorVRAM() : FMath::Clamp(Perfil, 0, 3);
	}();
	return P;
}

float AlsasuaEscala::Factor()
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
		static const float PorPerfil[] = { 0.25f, 0.5f, 0.75f, 1.f };
		return PorPerfil[PerfilEfectivo()];
	}();
	return F;
}
