#include "World/AlsasuaAmbientAudioSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWave.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

UAlsasuaAmbientAudioSystem::UAlsasuaAmbientAudioSystem()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f;
}

void UAlsasuaAmbientAudioSystem::BeginPlay()
{
    Super::BeginPlay();
    CargarSonidos();
}

void UAlsasuaAmbientAudioSystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    for (FAmbientLayer* Layer : {&RiverLayer, &WindLayer, &RainLayer, &BirdLayer, &TrafficLayer, &NightLayer, &ThunderLayer})
    {
        if (Layer->AudioComp && Layer->AudioComp->IsPlaying())
            Layer->AudioComp->Stop();
    }
    Super::EndPlay(EndPlayReason);
}

void UAlsasuaAmbientAudioSystem::CargarSonidos()
{
    auto Load = [&](const FString& Name) -> USoundWave*
    {
        const FString Path = SoundBasePath + Name;
        return LoadObject<USoundWave>(nullptr, *Path);
    };

    RiverLayer.Sound = Load(TEXT("Ambiance_River_Moderate_Loop_Stereo"));
    WindLayer.Sound = Load(TEXT("Ambiance_Wind_Calm_Loop_Stereo"));
    RainLayer.Sound = Load(TEXT("Ambiance_Rain_Calm_Loop_Stereo"));
    BirdLayer.Sound = Load(TEXT("Ambiance_Forest_Birds_Loop_Stereo"));
    TrafficLayer.Sound = Load(TEXT("City Ambience - Park - Spring - With birds, cars, and planes"));
    NightLayer.Sound = Load(TEXT("Ambiance_Night_Loop_Stereo"));
    ThunderLayer.Sound = Load(TEXT("Rain - Distant thunder"));

    UE_LOG(LogTemp, Log, TEXT("AmbientAudio: River=%s Wind=%s Rain=%s Birds=%s Traffic=%s Night=%s Thunder=%s"),
        RiverLayer.Sound ? TEXT("OK") : TEXT("NULL"),
        WindLayer.Sound ? TEXT("OK") : TEXT("NULL"),
        RainLayer.Sound ? TEXT("OK") : TEXT("NULL"),
        BirdLayer.Sound ? TEXT("OK") : TEXT("NULL"),
        TrafficLayer.Sound ? TEXT("OK") : TEXT("NULL"),
        NightLayer.Sound ? TEXT("OK") : TEXT("NULL"),
        ThunderLayer.Sound ? TEXT("OK") : TEXT("NULL"));
}

void UAlsasuaAmbientAudioSystem::CrearCapa(FAmbientLayer& Layer, const FString& SoundName)
{
    if (Layer.AudioComp) return;
    if (!Layer.Sound) return;

    UWorld* World = GetWorld();
    if (!World) return;

    AActor* Owner = GetOwner();
    if (!Owner) return;

    Layer.AudioComp = UGameplayStatics::SpawnSoundAttached(
        Layer.Sound,
        Owner->GetRootComponent(),
        NAME_None,
        FVector::ZeroVector,
        EAttachLocation::SnapToTarget,
        true,
        0.0f,
        0.0f,
        0.0f,
        nullptr,
        true,
        true
    );

    if (Layer.AudioComp)
    {
        Layer.AudioComp->SetVolumeMultiplier(0.0f);
        Layer.AudioComp->bAutoActivate = true;
        Layer.bActive = true;
    }
}

void UAlsasuaAmbientAudioSystem::ActualizarCapa(FAmbientLayer& Layer, float TargetVol, float DeltaTime)
{
    Layer.TargetVolume = TargetVol * MasterVolume;

    if (!Layer.bActive && Layer.TargetVolume > 0.01f)
    {
        CrearCapa(Layer, FString());
    }
}

void UAlsasuaAmbientAudioSystem::ActualizarVolumen(FAmbientLayer& Layer, float DeltaTime)
{
    if (!Layer.AudioComp) return;

    const float Speed = (Layer.TargetVolume > Layer.CurrentVolume) ? FadeInSpeed : FadeOutSpeed;
    Layer.CurrentVolume = FMath::FInterpTo(Layer.CurrentVolume, Layer.TargetVolume, DeltaTime, Speed);
    Layer.CurrentVolume = FMath::Clamp(Layer.CurrentVolume, 0.0f, 1.0f);

    Layer.AudioComp->SetVolumeMultiplier(Layer.CurrentVolume);
}

void UAlsasuaAmbientAudioSystem::UpdateAmbientAudio(float CurrentHour, float WindSpeed, float RainIntensity, float DistanceToRiver)
{
    const bool bDaytime = (CurrentHour >= BirdAudioDayStart && CurrentHour <= BirdAudioDayEnd);
    bIsNight = !bDaytime;

    float RiverTarget = FMath::Clamp(1.0f - (DistanceToRiver / RiverAudioDistance), 0.0f, RiverAudioMaxVolume);
    ActualizarCapa(RiverLayer, RiverTarget, 0.0f);

    float WindTarget = FMath::Clamp(WindSpeed / 30.0f, 0.0f, WindAudioMaxVolume);
    ActualizarCapa(WindLayer, WindTarget, 0.0f);

    float RainTarget = bIsRaining ? FMath::Clamp(RainIntensity, 0.2f, 1.0f) : 0.0f;
    ActualizarCapa(RainLayer, RainTarget, 0.0f);

    float BirdTarget = (bDaytime && !bIsRaining) ? FMath::FRandRange(0.15f, 0.4f) : 0.0f;
    ActualizarCapa(BirdLayer, BirdTarget, 0.0f);

    float TrafficTarget = bIsNight ? 0.08f : 0.25f;
    ActualizarCapa(TrafficLayer, TrafficTarget, 0.0f);

    float NightTarget = bIsNight ? 0.35f : 0.0f;
    ActualizarCapa(NightLayer, NightTarget, 0.0f);

    float ThunderTarget = bIsStorm ? FMath::FRandRange(0.4f, 0.8f) : 0.0f;
    ActualizarCapa(ThunderLayer, ThunderTarget, 0.0f);

    UE_LOG(LogTemp, VeryVerbose, TEXT("AudioAmbient: River=%.2f Wind=%.2f Rain=%.2f Birds=%.2f Traffic=%.2f Night=%.2f Thunder=%.2f"),
        RiverLayer.TargetVolume, WindLayer.TargetVolume, RainLayer.TargetVolume,
        BirdLayer.TargetVolume, TrafficLayer.TargetVolume, NightLayer.TargetVolume,
        ThunderLayer.TargetVolume);
}

void UAlsasuaAmbientAudioSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    ActualizarVolumen(RiverLayer, DeltaTime);
    ActualizarVolumen(WindLayer, DeltaTime);
    ActualizarVolumen(RainLayer, DeltaTime);
    ActualizarVolumen(BirdLayer, DeltaTime);
    ActualizarVolumen(TrafficLayer, DeltaTime);
    ActualizarVolumen(NightLayer, DeltaTime);
    ActualizarVolumen(ThunderLayer, DeltaTime);
}

void UAlsasuaAmbientAudioSystem::SetMonth(int32 Month)
{
    CurrentMonth = FMath::Clamp(Month, 1, 12);
}

void UAlsasuaAmbientAudioSystem::SetWeatherState(bool bRaining, bool bStorm, bool bSnow)
{
    bIsRaining = bRaining;
    bIsStorm = bStorm;
}
