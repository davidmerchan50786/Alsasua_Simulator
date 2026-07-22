#include "Environment/AlsasuaWeatherManager.h"
#include "Kismet/KismetMaterialLibrary.h"

AAlsasuaWeatherManager::AAlsasuaWeatherManager() { PrimaryActorTick.bCanEverTick = false; }

void AAlsasuaWeatherManager::UpdateGlobalWeather(float RainAmount, float SnowAmount) {
    if (WeatherNPC) {
        UWorld* World = GetWorld();
        UKismetMaterialLibrary::SetScalarParameterValue(World, WeatherNPC, TEXT("GlobalRain"), RainAmount);
        UKismetMaterialLibrary::SetScalarParameterValue(World, WeatherNPC, TEXT("GlobalSnow"), SnowAmount);
    }
}
