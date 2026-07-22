#include "World/AlsasuaWeatherSystem.h"
#include "AI/AlsasuaCrowdSentiment.h"

void UAlsasuaWeatherSystem::Tick(float DeltaTime)
{
    UAlsasuaCrowdSentiment* Sentiment = GetWorld()->GetSubsystem<UAlsasuaCrowdSentiment>();
    if (Sentiment)
    {
        float TargetWetness = FMath::Clamp(Sentiment->GlobalTension, 0.f, 1.f);
        GlobalWetness = FMath::FInterpTo(GlobalWetness, TargetWetness, DeltaTime, 0.1f);
        UpdateDynamicParameters(GlobalWetness);
    }
}

void UAlsasuaWeatherSystem::UpdateDynamicParameters(float Intensity)
{
    // Aquí se actualizaría una 'MaterialParameterCollection' (MPC_AlsasuaGlobal)
    // para que la lluvia y los charcos reaccionen.
}
