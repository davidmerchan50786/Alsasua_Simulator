#include "AI/Crowd/AlsasuaCrowdSoundComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"

UAlsasuaCrowdSoundComponent::UAlsasuaCrowdSoundComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.5f;  // Low freq — visual only
}

void UAlsasuaCrowdSoundComponent::Activar(float InIntensity)
{
    bActivo = true;
    Intensity = FMath::Clamp(InIntensity, 0.f, 1.f);
    TimerAnillos = 0.f;
    SetComponentTickEnabled(true);
}

void UAlsasuaCrowdSoundComponent::Desactivar()
{
    bActivo = false;
    Intensity = 0.f;
    SetComponentTickEnabled(false);
}

void UAlsasuaCrowdSoundComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!bActivo || Intensity <= 0.f) return;

    TimerAnillos += DeltaTime;
    if (TimerAnillos >= FrecuenciaAnillos)
    {
        TimerAnillos = 0.f;
        EmitirAnilloVisual();
    }
}

void UAlsasuaCrowdSoundComponent::EmitirAnilloVisual()
{
    UWorld* W = GetWorld();
    if (!W) return;

    // Sound wave ring VFX
    UNiagaraSystem* RingNS = LoadObject<UNiagaraSystem>(nullptr,
        TEXT("/Game/VFX/NS_SoundWave.NS_SoundWave"));
    if (RingNS)
    {
        UNiagaraComponent* NC = UNiagaraFunctionLibrary::SpawnSystemAttached(
            RingNS, GetOwner()->GetRootComponent(), NAME_None,
            FVector::ZeroVector, FRotator::ZeroRotator,
            EAttachLocation::KeepRelativeOffset, true);

        if (NC)
        {
            NC->SetFloatParameter(TEXT("Radius"), Radio * Intensity);
            NC->SetFloatParameter(TEXT("Intensity"), Intensity);
        }
    }

    // Play shout sound
    USoundBase* ShoutSound = LoadObject<USoundBase>(nullptr,
        TEXT("/Game/Audio/SC_GritoMultitud.SC_GritoMultitud"));
    if (ShoutSound)
    {
        UGameplayStatics::PlaySoundAtLocation(W, ShoutSound,
            GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector, Intensity * 1.0f);
    }
}
