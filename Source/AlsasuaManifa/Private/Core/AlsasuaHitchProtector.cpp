#include "Core/AlsasuaHitchProtector.h"

void UAlsasuaHitchProtector::Tick(float DeltaTime)
{
	// Si el frame anterior fue un "Hitch" (tirón grave)
	if (DeltaTime > HitchThreshold)
	{
		CurrentLODScale = 0.4f; // Reducción agresiva de carga
		PanicDuration = 3.0f;   // Mantener 3 segundos para estabilizar
		UE_LOG(LogTemp, Warning, TEXT("Hitch Protector: ¡Pánico detectado! Bajando densidad de IA..."));
	}

	if (PanicDuration > 0)
	{
		PanicDuration -= DeltaTime;
		if (PanicDuration <= 0)
		{
			// Recuperación suave del LOD
			CurrentLODScale = 1.0f;
		}
	}
}
