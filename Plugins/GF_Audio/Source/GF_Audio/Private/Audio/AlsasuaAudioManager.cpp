#include "Audio/AlsasuaAudioManager.h"
#include "AI/AlsasuaCrowdSentiment.h"

void UAlsasuaAudioManager::Tick(float DeltaTime)
{
	UWorld* W = GetWorld();
	if (!W) return;

	UAlsasuaCrowdSentiment* Sentiment = W->GetSubsystem<UAlsasuaCrowdSentiment>();
	if (!Sentiment) return;

	// La intensidad depende de la Tensión y de la Cantidad de gente próxima
	float TargetIntensity = Sentiment->GlobalTension;

	// El caos aumenta si el humor es Hostil o Panic
	float TargetChaos = (Sentiment->GlobalTension > 0.7f) ? 1.0f : 0.0f;

	// Aplicamos suavizado (Interpolación) para que el sonido no "salte"
	CurrentIntensity = FMath::FInterpTo(CurrentIntensity, TargetIntensity, DeltaTime, 0.5f);
	CurrentChaos = FMath::FInterpTo(CurrentChaos, TargetChaos, DeltaTime, 0.2f);
}
