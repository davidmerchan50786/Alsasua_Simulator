#include "World/Weather/WeatherSubsystem.h"
#include "AlsasuaServiceRegistry.h"

void UWeatherSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (UAlsasuaServiceRegistry* Registro = UAlsasuaServiceRegistry::Get(this))
    {
        Registro->Publicar(TEXT("Clima.Meteorologia"), this);
    }
}

void UWeatherSubsystem::Deinitialize()
{
    if (UAlsasuaServiceRegistry* Registro = UAlsasuaServiceRegistry::Get(this))
    {
        Registro->Retirar(TEXT("Clima.Meteorologia"));
    }
    Super::Deinitialize();
}

void UWeatherSubsystem::SetWeather(EAlsasuaWeatherState NewState)
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
        case EAlsasuaWeatherState::Rainy: return 0.7f;
        case EAlsasuaWeatherState::Thunderstorm: return 0.5f;
        default: return 1.0f;
    }
}

float UWeatherSubsystem::GetAIVisibilityMultiplier() const
{
    switch (CurrentWeather)
    {
        case EAlsasuaWeatherState::HeavyFog: return 0.4f;
        case EAlsasuaWeatherState::Thunderstorm: return 0.6f;
        default: return 1.0f;
    }
}

float UWeatherSubsystem::GetFootstepNoiseMultiplier() const
{
    // La lluvia amortigua el ruido de los pasos, ideal para sigilo
    return (CurrentWeather == EAlsasuaWeatherState::Rainy || CurrentWeather == EAlsasuaWeatherState::Thunderstorm) ? 0.5f : 1.0f;
}

// ponytail: constantes por estado; curva por hora del dia si el clima dinamico pide mas.
float UWeatherSubsystem::GetRainIntensity() const
{
    switch (CurrentWeather)
    {
        case EAlsasuaWeatherState::Rainy: return 0.6f;
        case EAlsasuaWeatherState::Thunderstorm: return 1.0f;
        default: return 0.0f;
    }
}

float UWeatherSubsystem::GetWindSpeedKmh() const
{
    switch (CurrentWeather)
    {
        case EAlsasuaWeatherState::Clear: return 5.0f;
        case EAlsasuaWeatherState::Rainy: return 20.0f;
        case EAlsasuaWeatherState::HeavyFog: return 8.0f;
        case EAlsasuaWeatherState::Thunderstorm: return 40.0f;
        default: return 5.0f;
    }
}

float UWeatherSubsystem::GetTemperatureCelsius() const
{
    // Alsasua, dia de verano tipico: la niebla y el agua refrescan.
    switch (CurrentWeather)
    {
        case EAlsasuaWeatherState::Clear: return 22.0f;
        case EAlsasuaWeatherState::Rainy: return 16.0f;
        case EAlsasuaWeatherState::HeavyFog: return 14.0f;
        case EAlsasuaWeatherState::Thunderstorm: return 15.0f;
        default: return 22.0f;
    }
}
