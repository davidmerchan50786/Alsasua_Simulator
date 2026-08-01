#include "CrowdAudioManager.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"

void UCrowdAudioManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UWorld* World = GetWorld();
    if (!World) return;

    // Crear capas de audio (componentes 3D).
    auto CreateLayer = [&](USoundBase* Sound, const FName& Name) -> UAudioComponent*
    {
        if (!Sound) return nullptr;
        UAudioComponent* Comp = NewObject<UAudioComponent>(World, Name);
        Comp->SetSound(Sound);
        Comp->bAutoActivate = false;
        Comp->SetVolumeMultiplier(0.f);
        if (CrowdAttenuation)
        {
            Comp->AttenuationSettings = CrowdAttenuation;
        }
        Comp->RegisterComponentWithWorld(World);
        return Comp;
    };

    MurmurLayer = CreateLayer(AmbientMurmurSound, TEXT("MurmurLayer"));
    ChantLayer = CreateLayer(ChantSound, TEXT("ChantLayer"));
    PanicLayer = CreateLayer(PanicScreamSound, TEXT("PanicLayer"));
    SirenLayer = CreateLayer(SirenSound, TEXT("SirenLayer"));
}

void UCrowdAudioManager::Deinitialize()
{
    auto DestroyLayer = [](UAudioComponent* Comp)
    {
        if (Comp)
        {
            Comp->FadeOut(0.3f, 0.f);
            Comp->DestroyComponent();
        }
    };

    DestroyLayer(MurmurLayer);
    DestroyLayer(ChantLayer);
    DestroyLayer(PanicLayer);
    DestroyLayer(SirenLayer);

    MurmurLayer = nullptr;
    ChantLayer = nullptr;
    PanicLayer = nullptr;
    SirenLayer = nullptr;

    Super::Deinitialize();
}

// ─────────────────────────────────────────────────────────────────────────────
//  GetCrowdVolumeMultiplier: logarítmico para realismo acústico.
// ─────────────────────────────────────────────────────────────────────────────
float UCrowdAudioManager::GetCrowdVolumeMultiplier(int32 ActiveProtesters) const
{
    if (ActiveProtesters <= 0) return 0.0f;
    return FMath::Clamp(FMath::Loge(ActiveProtesters + 1.f) / 5.0f, 0.0f, 1.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
//  UpdateCrowdAudio: lógica principal de capas de sonido.
// ─────────────────────────────────────────────────────────────────────────────
void UCrowdAudioManager::UpdateCrowdAudio(int32 AgentCount, float CrowdTension, float DistanceToPlayer)
{
    UWorld* W = GetWorld();
    if (!W) return;
    CurrentAgentCount = AgentCount;
    CurrentTension = FMath::Clamp(CrowdTension, 0.f, 1.f);

    // Atenuación por distancia.
    const float DistanceFactor = FMath::Clamp(1.f - (DistanceToPlayer / MaxAudibleDistance), 0.f, 1.f);
    const float BaseVolume = GetCrowdVolumeMultiplier(AgentCount) * DistanceFactor;

    // ── Murmullo: siempre presente si hay agentes ──────────────────────────
    MurmurTargetVol = (AgentCount > 0) ? BaseVolume * 0.6f : 0.f;

    // ── Cánticos: aparecen con tensión media ───────────────────────────────
    if (CurrentTension >= ChantTensionThreshold && AgentCount > 10)
    {
        // A mayor tensión, más fuerte el cántico.
        const float ChantIntensity = FMath::Clamp(
            (CurrentTension - ChantTensionThreshold) / (PanicTensionThreshold - ChantTensionThreshold),
            0.f, 1.f);
        ChantTargetVol = BaseVolume * ChantIntensity * 0.8f;
    }
    else
    {
        ChantTargetVol = 0.f;
    }

    // ── Pánico: solo con tensión alta ──────────────────────────────────────
    if (CurrentTension >= PanicTensionThreshold)
    {
        const float PanicIntensity = FMath::Clamp(
            (CurrentTension - PanicTensionThreshold) / (1.f - PanicTensionThreshold),
            0.f, 1.f);
        PanicTargetVol = BaseVolume * PanicIntensity * 0.9f;
    }
    else
    {
        PanicTargetVol = 0.f;
    }

    // ── Aplicar volúmenes con fade suave ───────────────────────────────────
    const float FadeSpeed = 3.f;
    if (MurmurLayer)
    {
        MurmurLayer->SetVolumeMultiplier(FMath::FInterpTo(
            MurmurLayer->VolumeMultiplier, MurmurTargetVol, W->GetDeltaSeconds(), FadeSpeed));
    }
    if (ChantLayer)
    {
        ChantLayer->SetVolumeMultiplier(FMath::FInterpTo(
            ChantLayer->VolumeMultiplier, ChantTargetVol, W->GetDeltaSeconds(), FadeSpeed));
    }
    if (PanicLayer)
    {
        PanicLayer->SetVolumeMultiplier(FMath::FInterpTo(
            PanicLayer->VolumeMultiplier, PanicTargetVol, W->GetDeltaSeconds(), FadeSpeed));
    }

    // Sirenas.
    if (SirenLayer)
    {
        const float SirenVol = bSirensActive ? BaseVolume * 0.7f : 0.f;
        SirenLayer->SetVolumeMultiplier(FMath::FInterpTo(
            SirenLayer->VolumeMultiplier, SirenVol, W->GetDeltaSeconds(), 2.f));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  PlayCrowdEvent: sonido puntual (disparo, explosión, grito).
// ─────────────────────────────────────────────────────────────────────────────
void UCrowdAudioManager::PlayCrowdEvent(FVector Location, float Intensity)
{
    UWorld* World = GetWorld();
    if (!World) return;

    USoundBase* SoundToPlay = nullptr;

    if (Intensity > 0.8f && DistantGunshotSound)
    {
        SoundToPlay = DistantGunshotSound;
    }
    else if (ImpactProtestSound)
    {
        SoundToPlay = ImpactProtestSound;
    }

    if (SoundToPlay)
    {
        const float Volume = FMath::Clamp(Intensity, 0.1f, 1.f);
        UGameplayStatics::PlaySoundAtLocation(World, SoundToPlay, Location, Volume, 1.f, 0.f, CrowdAttenuation);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  SetSirensActive
// ─────────────────────────────────────────────────────────────────────────────
void UCrowdAudioManager::SetSirensActive(bool bActive)
{
    bSirensActive = bActive;

    if (SirenLayer)
    {
        if (bActive && !SirenLayer->IsPlaying())
        {
            SirenLayer->Play();
        }
        else if (!bActive)
        {
            // Fade out ya se encarga UpdateCrowdAudio.
        }
    }
}
