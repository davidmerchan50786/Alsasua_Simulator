#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AlsasuaWeatherManager.generated.h"

UCLASS()
class ALSASUASIMULATOR_API AAlsasuaWeatherManager : public AActor {
    GENERATED_BODY()
public:
    AAlsasuaWeatherManager();

    // Cambia el estado del mundo: 0=Seco, 1=Lluvia, 2=Nieve (Barro dinámico)
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Weather")
    void UpdateGlobalWeather(float RainAmount, float SnowAmount);

    UPROPERTY(EditAnywhere, Category = "Alsasua|Weather")
    class UMaterialParameterCollection* WeatherNPC;
};
