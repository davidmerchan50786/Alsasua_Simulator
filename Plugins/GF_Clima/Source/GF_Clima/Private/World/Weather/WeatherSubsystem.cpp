#include "World/Weather/WeatherSubsystem.h"
#include "AlsasuaServiceRegistry.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

void UWeatherSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Register as IWeatherService
	if (UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UAlsasuaServiceRegistry* Reg = GI->GetSubsystem<UAlsasuaServiceRegistry>())
		{
			Reg->Publicar(FName("Weather"), this);
		}
	}
}

void UWeatherSubsystem::SetWeather(EWeatherSubsystemState NewState)
{
	if (CurrentWeather != NewState)
	{
		CurrentWeather = NewState;
		OnWeatherChanged.Broadcast(CurrentWeather);
		UE_LOG(LogTemp, Log, TEXT("El clima en Altsasu ha cambiado a: %d"), (uint8)CurrentWeather);
	}
}

float UWeatherSubsystem::GetVisibilityMultiplier() const
{
	switch (CurrentWeather)
	{
		case EWeatherSubsystemState::HeavyFog: return 0.4f;
		case EWeatherSubsystemState::Thunderstorm: return 0.6f;
		case EWeatherSubsystemState::Rainy: return 0.8f;
		default: return 1.0f;
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
	return (CurrentWeather == EWeatherSubsystemState::Rainy || CurrentWeather == EWeatherSubsystemState::Thunderstorm) ? 0.5f : 1.0f;
}
