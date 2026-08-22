#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaWeatherSystem.generated.h"

UENUM(BlueprintType)
enum class EWeatherState : uint8
{
    Clear       UMETA(DisplayName = "Despejado"),
    Cloudy      UMETA(DisplayName = "Nublado"),
    Overcast    UMETA(DisplayName = "Cubierto"),
    LightRain   UMETA(DisplayName = "Lluvia Ligera"),
    HeavyRain   UMETA(DisplayName = "Lluvia Fuerte"),
    Fog         UMETA(DisplayName = "Niebla"),
    Storm       UMETA(DisplayName = "Tormenta"),
    Snow        UMETA(DisplayName = "Nieve"),
};

USTRUCT(BlueprintType)
struct FMonthlyWeather
{
    GENERATED_BODY()
    int32 Month = 1;
    float AvgTempC = 10.0f;
    float RainProbability = 0.4f;
    float FogProbability = 0.2f;
    float SnowProbability = 0.0f;
    float FrostProbability = 0.1f;
    float StormProbability = 0.05f;
    float WindSpeedKmh = 12.0f;
    FString PrevailingWindDir = TEXT("SW");
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GF_CLIMA_API UAlsasuaWeatherSystem : public UActorComponent
{
    GENERATED_BODY()
public:
    UAlsasuaWeatherSystem();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
    EWeatherState CurrentWeather = EWeatherState::Clear;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
    float WeatherDuration = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Transition")
    float TransitionSpeed = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Fog")
    float FogDensityClear = 0.002f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Fog")
    float FogDensityFog = 0.05f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Fog")
    FLinearColor FogColorDay = FLinearColor(0.7f, 0.75f, 0.85f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Fog")
    FLinearColor FogColorRain = FLinearColor(0.4f, 0.42f, 0.48f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Rain")
    float RainIntensity = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Wind")
    float WindSpeed = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Wind")
    FVector WindDirection = FVector(1.0f, 0.0f, 0.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Sky")
    float CloudDensity = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Climate")
    int32 CurrentMonth = 7;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Climate")
    float GameTimeHour = 12.0f;

    UFUNCTION(BlueprintCallable, Category = "Weather")
    void SetWeather(EWeatherState NewWeather);

    UFUNCTION(BlueprintCallable, Category = "Weather")
    void SetMonth(int32 Month);

    UFUNCTION(BlueprintCallable, Category = "Weather")
    EWeatherState GetWeather() const { return CurrentWeather; }

    UFUNCTION(BlueprintCallable, Category = "Weather")
    bool IsRaining() const { return RainIntensity > 0.1f; }

    UFUNCTION(BlueprintCallable, Category = "Weather")
    bool IsFoggy() const { return CurrentWeather == EWeatherState::Fog; }

    UFUNCTION(BlueprintCallable, Category = "Weather")
    float GetCurrentTemperatureC() const;

    UFUNCTION(BlueprintCallable, Category = "Weather")
    FLinearColor GetSkyTintForWeather() const;

    UFUNCTION(BlueprintCallable, Category = "Weather")
    FString GetDebugWeatherSummary() const;

private:
    EWeatherState TargetWeather = EWeatherState::Clear;
    float TimeSinceWeatherChange = 0.0f;
    float TargetFogDensity = 0.002f;
    float TargetRainIntensity = 0.0f;
    float TargetCloudDensity = 0.0f;
    float CurrentFogDensity = 0.002f;

    TArray<FMonthlyWeather> MonthlyData;
    bool bDataLoaded = false;

    void LoadWeatherData();
    void UpdateWeatherTransition(float DeltaTime);
    void ApplyWeatherEffects();
    void PickNextWeather();
    const FMonthlyWeather& GetCurrentMonthData() const;
};
