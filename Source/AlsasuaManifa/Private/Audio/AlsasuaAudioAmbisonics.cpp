#include "Audio/AlsasuaAudioAmbisonics.h"
#include "AI/AlsasuaCrowdSentiment.h"
#include "Core/AlsasuaProfiling.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

UAlsasuaAudioAmbisonics::UAlsasuaAudioAmbisonics()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UAlsasuaAudioAmbisonics::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    UWorld* W = GetWorld();
    if (!W) return;

    UAlsasuaCrowdSentiment* Sentiment = W->GetSubsystem<UAlsasuaCrowdSentiment>();
    if (Sentiment)
    {
        // El volumen del murmullo sube con la tensión global
        CrowdHumIntensity = FMath::FInterpTo(CrowdHumIntensity, Sentiment->GlobalTension, DeltaTime, 0.5f);
    }

    HandleAcousticOcclusion(DeltaTime);
}

void UAlsasuaAudioAmbisonics::TriggerDynamicChant(FString ChantID, FVector Origin)
{
    UWorld* W = GetWorld();
    if (!W) return;

    USoundBase* ChantSound = nullptr;
    if (USoundBase** Found = ChantSounds.Find(ChantID))
    {
        ChantSound = *Found;
    }

    if (!ChantSound)
    {
        // Fallback: try to load from a conventional path.
        FString Path = FString::Printf(TEXT("/Game/Audio/Chants/SC_%s.SC_%s"), *ChantID, *ChantID);
        ChantSound = LoadObject<USoundBase>(nullptr, *Path);
    }

    if (ChantSound)
    {
        float Volume = FMath::Clamp(CrowdHumIntensity * 1.2f, 0.1f, 2.0f);
        UGameplayStatics::SpawnSoundAtLocation(W, ChantSound, Origin, FRotator::ZeroRotator, Volume, 1.0f, 0.0f, ChantAttenuation);
    }
}

void UAlsasuaAudioAmbisonics::HandleAcousticOcclusion(float DeltaTime)
{
    AActor* Owner = GetOwner();
    if (!Owner) return;

    // Trace from the listener (player camera) to this component to detect wall occlusion.
    UWorld* W2 = GetWorld();
    if (!W2) return;

    APlayerController* PC = UGameplayStatics::GetPlayerController(W2, 0);
    if (!PC) return;

    FVector ListenerLoc;
    FRotator ListenerRot;
    PC->GetPlayerViewPoint(ListenerLoc, ListenerRot);

    FVector ComponentLoc = Owner->GetActorLocation();
    FHitResult Hit;
    FCollisionQueryParams Q;
    Q.AddIgnoredActor(Owner);

    if (W2->LineTraceSingleByChannel(Hit, ListenerLoc, ComponentLoc, ECC_Visibility, Q))
    {
        // Wall detected — apply occlusion factor (0.0 = fully occluded, 1.0 = clear).
        float Distance = FVector::Dist(ListenerLoc, ComponentLoc);
        OcclusionFactor = FMath::FInterpTo(OcclusionFactor, 0.3f, DeltaTime, 2.0f);
    }
    else
    {
        // Clear line of sight — no occlusion.
        OcclusionFactor = FMath::FInterpTo(OcclusionFactor, 1.0f, DeltaTime, 2.0f);
    }

    // Apply LPF attenuation to the crowd hum volume.
    CrowdHumIntensity *= OcclusionFactor;
}
