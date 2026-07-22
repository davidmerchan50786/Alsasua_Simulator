#include "CrowdAudioManager.h"

float UCrowdAudioManager::GetCrowdVolumeMultiplier(int32 ActiveProtesters) const
{
	// Lógica logarítmica para que el sonido no sea lineal (realismo acústico)
	if (ActiveProtesters <= 0) return 0.0f;
	return FMath::Clamp(FMath::Loge(ActiveProtesters + 1.f) / 5.0f, 0.0f, 1.0f);
}
