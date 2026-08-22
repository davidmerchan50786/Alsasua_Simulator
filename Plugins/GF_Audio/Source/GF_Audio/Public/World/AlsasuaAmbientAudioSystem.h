#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaAmbientAudioSystem.generated.h"

class UAudioComponent;
class USoundWave;

UENUM(BlueprintType)
enum class EAmbientLayer : uint8
{
    River,
    Wind,
    Rain,
    Birds,
    Traffic,
    Night,
    Thunder
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GF_AUDIO_API UAlsasuaAmbientAudioSystem : public UActorComponent
{
    GENERATED_BODY()
public:
    UAlsasuaAmbientAudioSystem();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Config")
    float RiverAudioDistance = 5000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Config")
    float RiverAudioMaxVolume = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Config")
    float WindAudioMaxVolume = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Config")
    float TrafficAudioDistance = 20000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Config")
    float BirdAudioDayStart = 6.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Config")
    float BirdAudioDayEnd = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Config")
    float MasterVolume = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Config")
    float FadeInSpeed = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Config")
    float FadeOutSpeed = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Paths")
    FString SoundBasePath = TEXT("/Game/AssetsImportados/Sonido/ExtractedSounds/");

    UFUNCTION(BlueprintCallable, Category = "Audio")
    void UpdateAmbientAudio(float CurrentHour, float WindSpeed, float RainIntensity, float DistanceToRiver);

    UFUNCTION(BlueprintCallable, Category = "Audio")
    void SetMonth(int32 Month);

    UFUNCTION(BlueprintCallable, Category = "Audio")
    void SetWeatherState(bool bRaining, bool bStorm, bool bSnow);

private:
    struct FAmbientLayer
    {
        UAudioComponent* AudioComp = nullptr;
        USoundWave* Sound = nullptr;
        float TargetVolume = 0.0f;
        float CurrentVolume = 0.0f;
        bool bActive = false;
    };

    FAmbientLayer RiverLayer;
    FAmbientLayer WindLayer;
    FAmbientLayer RainLayer;
    FAmbientLayer BirdLayer;
    FAmbientLayer TrafficLayer;
    FAmbientLayer NightLayer;
    FAmbientLayer ThunderLayer;

    int32 CurrentMonth = 6;
    bool bIsRaining = false;
    bool bIsStorm = false;
    bool bIsNight = false;

    void CargarSonidos();
    void CrearCapa(FAmbientLayer& Layer, const FString& SoundName);
    void ActualizarCapa(FAmbientLayer& Layer, float TargetVol, float DeltaTime);
    void ActualizarVolumen(FAmbientLayer& Layer, float DeltaTime);
};
