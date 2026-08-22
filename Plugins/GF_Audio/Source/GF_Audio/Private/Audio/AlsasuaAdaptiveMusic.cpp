#include "Audio/AlsasuaAdaptiveMusic.h"
#include "AI/AlsasuaCrowdSentiment.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

UAlsasuaAdaptiveMusic::UAlsasuaAdaptiveMusic()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f;
}

void UAlsasuaAdaptiveMusic::BeginPlay()
{
    Super::BeginPlay();
    // Start ambient layer immediately.
    if (AmbientLayer.MusicAsset)
    {
        AmbientLayer.TargetVolume = 0.7f;
    }
}

void UAlsasuaAdaptiveMusic::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bForced) return;

    UWorld* World = GetWorld();
    if (!World) return;

    // Evaluate game state.
    float Tension = 0.f;
    int32 WantedLevel = 0;

    if (UAlsasuaCrowdSentiment* Sentiment = World->GetSubsystem<UAlsasuaCrowdSentiment>())
    {
        Tension = Sentiment->GlobalTension;
    }

    // Ambient: always on, quieter when tense.
    AmbientLayer.TargetVolume = FMath::Lerp(0.7f, 0.3f, Tension);

    // Tension layer: fades in as tension rises.
    TensionLayer.TargetVolume = (Tension > TensionThreshold)
        ? FMath::Lerp(0.f, 0.8f, (Tension - TensionThreshold) / (1.f - TensionThreshold))
        : 0.f;

    // Combat layer: active during high wanted level.
    CombatLayer.TargetVolume = (WantedLevel >= CombatWantedLevel) ? 0.6f : 0.f;

    // Social layer: active during protests.
    SocialLayer.TargetVolume = 0.f; // Driven externally.

    // Update all layers.
    UpdateLayerVolume(AmbientLayer, DeltaTime);
    UpdateLayerVolume(TensionLayer, DeltaTime);
    UpdateLayerVolume(CombatLayer, DeltaTime);
    UpdateLayerVolume(SocialLayer, DeltaTime);
}

void UAlsasuaAdaptiveMusic::UpdateLayerVolume(FMusicLayer& Layer, float DeltaTime)
{
    // Smooth crossfade.
    Layer.CurrentVolume = FMath::FInterpTo(Layer.CurrentVolume, Layer.TargetVolume, DeltaTime, Layer.FadeSpeed);

    // Start audio component if needed.
    if (Layer.TargetVolume > 0.01f && !Layer.AudioComp && Layer.MusicAsset)
    {
        StartLayer(Layer, Layer.CurrentVolume);
    }

    // Stop audio component if silent.
    if (Layer.TargetVolume < 0.01f && Layer.AudioComp)
    {
        StopLayer(Layer);
    }

    // Update volume.
    if (Layer.AudioComp && Layer.AudioComp->IsPlaying())
    {
        Layer.AudioComp->SetVolumeMultiplier(Layer.CurrentVolume);
    }
}

void UAlsasuaAdaptiveMusic::StartLayer(FMusicLayer& Layer, float Volume)
{
    if (!Layer.MusicAsset || !GetWorld()) return;
    Layer.AudioComp = UGameplayStatics::SpawnSound2D(GetWorld(), Layer.MusicAsset, Volume, 1.0f, 0.f, nullptr, true);
}

void UAlsasuaAdaptiveMusic::StopLayer(FMusicLayer& Layer)
{
    if (Layer.AudioComp)
    {
        Layer.AudioComp->Stop();
        Layer.AudioComp = nullptr;
    }
}

void UAlsasuaAdaptiveMusic::ForceLayer(FName LayerName, float Volume)
{
    bForced = true;
    FMusicLayer* Layer = nullptr;
    if (LayerName == FName("Ambient")) Layer = &AmbientLayer;
    else if (LayerName == FName("Tension")) Layer = &TensionLayer;
    else if (LayerName == FName("Combat")) Layer = &CombatLayer;
    else if (LayerName == FName("Social")) Layer = &SocialLayer;

    if (Layer) Layer->TargetVolume = Volume;
}

void UAlsasuaAdaptiveMusic::ReleaseAllLayers()
{
    bForced = false;
}
