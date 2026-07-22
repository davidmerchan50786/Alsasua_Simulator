#include "Audio/AlsasuaAudioAmbisonics.h"
#include "AI/AlsasuaCrowdSentiment.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

UAlsasuaAudioAmbisonics::UAlsasuaAudioAmbisonics()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UAlsasuaAudioAmbisonics::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    UAlsasuaCrowdSentiment* Sentiment = GetWorld()->GetSubsystem<UAlsasuaCrowdSentiment>();
    if (Sentiment)
    {
        // El volumen del murmullo sube con la tensión global
        CrowdHumIntensity = FMath::FInterpTo(CrowdHumIntensity, Sentiment->GlobalTension, DeltaTime, 0.5f);
    }

    HandleAcousticOcclusion(DeltaTime);
}

void UAlsasuaAudioAmbisonics::TriggerDynamicChant(FString ChantID, FVector Origin)
{
    // Simula la propagación del sonido: se reproducen instancias con delay 
    // según la distancia al origen para crear el efecto de "eco en la plaza"
    UE_LOG(LogTemp, Log, TEXT("AUDIO: Cántico '%s' propagándose desde %s"), *ChantID, *Origin.ToString());
}

void UAlsasuaAudioAmbisonics::HandleAcousticOcclusion(float DeltaTime)
{
    // Lógica para detectar si el jugador está tras una pared (portales de Alsasua)
    // Aplicaría un LPF (Low Pass Filter) al AudioMixer global de ser una implementación completa.
}
