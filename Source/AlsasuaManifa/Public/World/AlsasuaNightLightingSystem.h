#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaNightLightingSystem.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UAlsasuaNightLightingSystem : public UActorComponent
{
    GENERATED_BODY()
public:
    UAlsasuaNightLightingSystem();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night|Timing")
    float SunsetHour = 20.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night|Timing")
    float SunriseHour = 7.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night|Timing")
    float TransitionDuration = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night|Lights")
    float FarolaIntensity = 5000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night|Lights")
    FLinearColor FarolaColor = FLinearColor(1.0f, 0.85f, 0.6f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night|Lights")
    float WindowEmissiveMin = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night|Lights")
    float WindowEmissiveMax = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night|Lights")
    FLinearColor NeonColorComercio = FLinearColor(0.2f, 0.8f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night|Ambient")
    float NightAmbientIntensity = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night|Ambient")
    FLinearColor NightAmbientColor = FLinearColor(0.05f, 0.05f, 0.15f);

    UFUNCTION(BlueprintCallable, Category = "Night")
    float GetCurrentNightFactor() const { return NightFactor; }

    UFUNCTION(BlueprintCallable, Category = "Night")
    bool IsNight() const { return NightFactor > 0.5f; }

private:
    float NightFactor = 0.0f;
    float CurrentHour = 12.0f;
    TArray<AActor*> Farolas;
    TArray<AActor*> Edificios;
    bool bFarolasCached = false;

    void UpdateNightFactor();
    void CacheNightActors();
    void UpdateFarolas();
    void UpdateEdificios();
};
