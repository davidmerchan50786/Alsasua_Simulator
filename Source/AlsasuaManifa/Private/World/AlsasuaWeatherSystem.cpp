#include "World/AlsasuaWeatherSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

UAlsasuaWeatherSystem::UAlsasuaWeatherSystem()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.25f;
}

void UAlsasuaWeatherSystem::BeginPlay()
{
    Super::BeginPlay();
    LoadWeatherData();

    FDateTime Now = FDateTime::Now();
    CurrentMonth = Now.GetMonth();
    TimeSinceWeatherChange = WeatherDuration;

    UE_LOG(LogTemp, Log, TEXT("WeatherSystem: Mes actual=%d, datos=%s"),
        CurrentMonth, bDataLoaded ? TEXT("CARGADO") : TEXT("PREDETERMINADO"));
}

void UAlsasuaWeatherSystem::LoadWeatherData()
{
    MonthlyData.Empty();

    FString JsonStr;
    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/weather_alsasua.json");
    if (!FFileHelper::LoadFileToString(JsonStr, *JsonPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("WeatherSystem: No se pudo cargar weather_alsasua.json, usando datos por defecto"));
        for (int32 i = 1; i <= 12; i++)
        {
            FMonthlyWeather M;
            M.Month = i;
            M.AvgTempC = 13.0f;
            M.RainProbability = 0.4f;
            M.FogProbability = 0.2f;
            M.SnowProbability = (i >= 12 || i <= 2) ? 0.08f : 0.0f;
            M.FrostProbability = (i >= 11 || i <= 3) ? 0.2f : 0.0f;
            M.StormProbability = (i >= 6 && i <= 9) ? 0.1f : 0.03f;
            M.WindSpeedKmh = 12.0f;
            M.PrevailingWindDir = TEXT("SW");
            MonthlyData.Add(M);
        }
        bDataLoaded = false;
        return;
    }

    TSharedPtr<FJsonValue> RootVal;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, RootVal) || !RootVal.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("WeatherSystem: Error parseando weather_alsasua.json"));
        return;
    }

    const TSharedPtr<FJsonObject>& Root = RootVal->AsObject();
    if (!Root) return;

    const TArray<TSharedPtr<FJsonValue>>* MonthlyArr;
    if (!Root->TryGetArrayField(TEXT("monthly"), MonthlyArr)) return;

    for (const auto& MonthVal : *MonthlyArr)
    {
        const TSharedPtr<FJsonObject>& M = MonthVal->AsObject();
        if (!M) continue;

        FMonthlyWeather Entry;
        Entry.Month = M->GetIntegerField(TEXT("month"));
        Entry.AvgTempC = M->GetNumberField(TEXT("avg_temp_c"));
        Entry.WindSpeedKmh = M->GetNumberField(TEXT("wind_speed_kmh"));
        Entry.PrevailingWindDir = M->GetStringField(TEXT("prevailing_wind_dir"));
        Entry.FogProbability = M->GetNumberField(TEXT("fog_intensity"));
        Entry.FrostProbability = M->GetNumberField(TEXT("frost_probability"));

        const TArray<TSharedPtr<FJsonValue>>* TypicalArr;
        if (M->TryGetArrayField(TEXT("typical_weather"), TypicalArr))
        {
            Entry.RainProbability = 0.3f;
            Entry.StormProbability = 0.05f;
            for (const auto& TW : *TypicalArr)
            {
                FString S = TW->AsString();
                if (S.Contains(TEXT("lluvia"))) Entry.RainProbability = FMath::Max(Entry.RainProbability, 0.5f);
                if (S.Contains(TEXT("niebla"))) Entry.FogProbability = FMath::Max(Entry.FogProbability, 0.3f);
                if (S.Contains(TEXT("tormenta"))) Entry.StormProbability = 0.15f;
                if (S.Contains(TEXT("heladas"))) Entry.FrostProbability = FMath::Max(Entry.FrostProbability, 0.3f);
            }
        }

        const TArray<TSharedPtr<FJsonValue>>* TypicalWeather;
        if (M->TryGetArrayField(TEXT("typical_weather"), TypicalWeather))
        {
            for (const auto& W : *TypicalWeather)
            {
                FString S = W->AsString();
                if (S.Contains(TEXT("niebla"))) Entry.FogProbability = FMath::Max(Entry.FogProbability, 0.4f);
                if (S.Contains(TEXT("heladas"))) Entry.FrostProbability = FMath::Max(Entry.FrostProbability, 0.4f);
                if (S.Contains(TEXT("nieve"))) Entry.SnowProbability = 0.1f;
            }
        }

        if (Entry.Month >= 6 && Entry.Month <= 9)
            Entry.StormProbability = 0.12f;

        MonthlyData.Add(Entry);
    }

    bDataLoaded = MonthlyData.Num() == 12;
    if (!bDataLoaded)
    {
        UE_LOG(LogTemp, Warning, TEXT("WeatherSystem: Solo %d meses cargados"), MonthlyData.Num());
        return;
    }

    FString ClimateStr;
    const FString ClimPath = FPaths::ProjectContentDir() + TEXT("Datos/climate_data.json");
    if (FFileHelper::LoadFileToString(ClimateStr, *ClimPath))
    {
        TSharedPtr<FJsonObject> ClimRoot;
        TSharedRef<TJsonReader<>> ClimRd = TJsonReaderFactory<>::Create(ClimateStr);
        if (FJsonSerializer::Deserialize(ClimRd, ClimRoot) && ClimRoot.IsValid())
        {
            const TArray<TSharedPtr<FJsonValue>>* DatosArr;
            if (ClimRoot->TryGetArrayField(TEXT("datos_mensuales"), DatosArr))
            {
                for (const auto& DV : *DatosArr)
                {
                    const TSharedPtr<FJsonObject>& DO = DV->AsObject();
                    if (!DO) continue;
                    const int32 Mes = DO->GetIntegerField(TEXT("mes"));
                    for (FMonthlyWeather& M : MonthlyData)
                    {
                        if (M.Month == Mes)
                        {
                            const float DiasLluvia = DO->GetNumberField(TEXT("dias_lluvia"));
                            const float NieveDias = DO->GetNumberField(TEXT("nieve_dias"));
                            const float Humedad = DO->GetNumberField(TEXT("humedad"));
                            M.RainProbability = FMath::Clamp(DiasLluvia / 30.0f, 0.0f, 1.0f);
                            M.SnowProbability = FMath::Clamp(NieveDias / 30.0f, 0.0f, 1.0f);
                            break;
                        }
                    }
                }
                UE_LOG(LogTemp, Log, TEXT("WeatherSystem: climate_data.json cargado, %d meses refinados"), DatosArr->Num());
            }
        }
    }
}

const FMonthlyWeather& UAlsasuaWeatherSystem::GetCurrentMonthData() const
{
    static const FMonthlyWeather DefaultMonth;
    for (const FMonthlyWeather& M : MonthlyData)
    {
        if (M.Month == CurrentMonth) return M;
    }
    return DefaultMonth;
}

float UAlsasuaWeatherSystem::GetCurrentTemperatureC() const
{
    const FMonthlyWeather& Data = GetCurrentMonthData();
    float TempBase = Data.AvgTempC;

    float HourFactor = FMath::Sin((GameTimeHour - 6.0f) / 24.0f * 2.0f * PI) * 5.0f;
    float RainPenalty = IsRaining() ? -3.0f : 0.0f;
    float NightPenalty = (GameTimeHour < 6.0f || GameTimeHour > 21.0f) ? -4.0f : 0.0f;

    return TempBase + HourFactor + RainPenalty + NightPenalty;
}

FLinearColor UAlsasuaWeatherSystem::GetSkyTintForWeather() const
{
    switch (CurrentWeather)
    {
    case EWeatherState::Clear:    return FLinearColor(0.5f, 0.6f, 0.9f);
    case EWeatherState::Cloudy:   return FLinearColor(0.6f, 0.65f, 0.75f);
    case EWeatherState::Overcast: return FLinearColor(0.5f, 0.52f, 0.55f);
    case EWeatherState::LightRain: return FLinearColor(0.4f, 0.43f, 0.5f);
    case EWeatherState::HeavyRain: return FLinearColor(0.3f, 0.32f, 0.38f);
    case EWeatherState::Fog:      return FLinearColor(0.6f, 0.62f, 0.65f);
    case EWeatherState::Storm:    return FLinearColor(0.25f, 0.27f, 0.33f);
    case EWeatherState::Snow:     return FLinearColor(0.8f, 0.82f, 0.88f);
    default: return FLinearColor(0.5f, 0.6f, 0.9f);
    }
}

void UAlsasuaWeatherSystem::SetWeather(EWeatherState NewWeather)
{
    TargetWeather = NewWeather;
    TimeSinceWeatherChange = 0.0f;

    switch (NewWeather)
    {
    case EWeatherState::Clear:
        TargetFogDensity = FogDensityClear;
        TargetRainIntensity = 0.0f;
        TargetCloudDensity = 0.1f;
        WindSpeed = 2.0f;
        break;
    case EWeatherState::Cloudy:
        TargetFogDensity = 0.005f;
        TargetRainIntensity = 0.0f;
        TargetCloudDensity = 0.5f;
        WindSpeed = 5.0f;
        break;
    case EWeatherState::Overcast:
        TargetFogDensity = 0.01f;
        TargetRainIntensity = 0.0f;
        TargetCloudDensity = 0.8f;
        WindSpeed = 8.0f;
        break;
    case EWeatherState::LightRain:
        TargetFogDensity = 0.015f;
        TargetRainIntensity = 0.3f;
        TargetCloudDensity = 0.9f;
        WindSpeed = 10.0f;
        break;
    case EWeatherState::HeavyRain:
        TargetFogDensity = 0.025f;
        TargetRainIntensity = 0.8f;
        TargetCloudDensity = 1.0f;
        WindSpeed = 15.0f;
        break;
    case EWeatherState::Fog:
        TargetFogDensity = FogDensityFog;
        TargetRainIntensity = 0.0f;
        TargetCloudDensity = 0.3f;
        WindSpeed = 1.0f;
        break;
    case EWeatherState::Storm:
        TargetFogDensity = 0.03f;
        TargetRainIntensity = 1.0f;
        TargetCloudDensity = 1.0f;
        WindSpeed = 25.0f;
        break;
    case EWeatherState::Snow:
        TargetFogDensity = 0.02f;
        TargetRainIntensity = 0.0f;
        TargetCloudDensity = 0.9f;
        WindSpeed = 6.0f;
        break;
    }

    UE_LOG(LogTemp, Log, TEXT("WeatherSystem: Cambiando a estado %d, mes=%d"),
        (int32)NewWeather, CurrentMonth);
}

void UAlsasuaWeatherSystem::SetMonth(int32 Month)
{
    CurrentMonth = FMath::Clamp(Month, 1, 12);
    PickNextWeather();
    UE_LOG(LogTemp, Log, TEXT("WeatherSystem: Mes cambiado a %d"), CurrentMonth);
}

void UAlsasuaWeatherSystem::PickNextWeather()
{
    const FMonthlyWeather& Data = GetCurrentMonthData();

    const float R = FMath::FRand();

    float Cumulative = 0.0f;

    float ClearProb = FMath::Max(0.0f, 1.0f - Data.RainProbability - Data.FogProbability - Data.SnowProbability - Data.StormProbability);
    Cumulative += ClearProb;
    if (R < Cumulative) { SetWeather(EWeatherState::Clear); return; }

    Cumulative += Data.FogProbability * 0.5f;
    if (R < Cumulative) { SetWeather(EWeatherState::Fog); return; }

    Cumulative += 0.15f;
    if (R < Cumulative) { SetWeather(EWeatherState::Cloudy); return; }

    Cumulative += 0.10f;
    if (R < Cumulative) { SetWeather(EWeatherState::Overcast); return; }

    Cumulative += Data.RainProbability * 0.3f;
    if (R < Cumulative) { SetWeather(EWeatherState::LightRain); return; }

    Cumulative += Data.RainProbability * 0.15f;
    if (R < Cumulative) { SetWeather(EWeatherState::HeavyRain); return; }

    Cumulative += Data.StormProbability;
    if (R < Cumulative) { SetWeather(EWeatherState::Storm); return; }

    Cumulative += Data.SnowProbability;
    if (R < Cumulative) { SetWeather(EWeatherState::Snow); return; }

    SetWeather(EWeatherState::Clear);
}

void UAlsasuaWeatherSystem::UpdateWeatherTransition(float DeltaTime)
{
    CurrentFogDensity = FMath::FInterpTo(CurrentFogDensity, TargetFogDensity, DeltaTime, TransitionSpeed);
    RainIntensity = FMath::FInterpTo(RainIntensity, TargetRainIntensity, DeltaTime, TransitionSpeed);
    CloudDensity = FMath::FInterpTo(CloudDensity, TargetCloudDensity, DeltaTime, TransitionSpeed);

    const FMonthlyWeather& Data = GetCurrentMonthData();
    FVector TargetWindDir = FVector::ForwardVector;
    if (Data.PrevailingWindDir == TEXT("SW")) TargetWindDir = FVector(-1.0f, -1.0f, 0.0f).GetSafeNormal();
    else if (Data.PrevailingWindDir == TEXT("W")) TargetWindDir = FVector(0.0f, -1.0f, 0.0f);
    else if (Data.PrevailingWindDir == TEXT("NW")) TargetWindDir = FVector(-1.0f, 1.0f, 0.0f).GetSafeNormal();
    WindDirection = FMath::VInterpTo(WindDirection, TargetWindDir, DeltaTime, 0.5f);
}

void UAlsasuaWeatherSystem::ApplyWeatherEffects()
{
    UWorld* World = GetWorld();
    if (!World) return;

    TArray<AActor*> Fogs;
    UGameplayStatics::GetAllActorsOfClass(World, AExponentialHeightFog::StaticClass(), Fogs);
    for (AActor* FogActor : Fogs)
    {
        UExponentialHeightFogComponent* Fog = FogActor->FindComponentByClass<UExponentialHeightFogComponent>();
        if (Fog)
        {
            Fog->SetFogDensity(CurrentFogDensity);
            Fog->SetFogInscatteringColor(IsRaining() ? FogColorRain : FogColorDay);
        }
    }
}

void UAlsasuaWeatherSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    GameTimeHour += DeltaTime / 60.0f;
    if (GameTimeHour >= 24.0f) GameTimeHour -= 24.0f;

    TimeSinceWeatherChange += DeltaTime;
    if (TimeSinceWeatherChange >= WeatherDuration)
    {
        PickNextWeather();
    }

    UpdateWeatherTransition(DeltaTime);
    ApplyWeatherEffects();
}
