#include "World/Weather/WeatherSubsystem.h"

void UWeatherSubsystem::SetWeather(EWeatherSubsystemState NewState)
{
    if (CurrentWeather != NewState)
    {
        CurrentWeather = NewState;
        OnWeatherChanged.Broadcast(CurrentWeather);
        UE_LOG(LogTemp, Log, TEXT("El clima en Altsasu ha cambiado a: %d"), (uint8)CurrentWeather);
    }
}

float UWeatherSubsystem::GetTireGripMultiplier() const
{
    switch (CurrentWeather)
    {
        case EWeatherSubsystemState::Rainy: return 0.7f;
        case EWeatherSubsystemState::Thunderstorm: return 0.5f;
        default: return 1.0f;
    }
}

float UWeatherSubsystem::GetAIVisibilityMultiplier() const
{
    switch (CurrentWeather)
    {
        case EWeatherSubsystemState::HeavyFog: return 0.4f;
        case EWeatherSubsystemState::Thunderstorm: return 0.6f;
        default: return 1.0f;
    }
}

float UWeatherSubsystem::GetFootstepNoiseMultiplier() const
{
    // La lluvia amortigua el ruido de los pasos, ideal para sigilo
    return (CurrentWeather == EWeatherSubsystemState::Rainy || CurrentWeather == EWeatherSubsystemState::Thunderstorm) ? 0.5f : 1.0f;
}
