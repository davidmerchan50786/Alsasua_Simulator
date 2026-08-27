#include "Character/GameplayPostProcessComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/BillboardComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Character.h"
#include "AlsasuaCharacter.h"

UGameplayPostProcessComponent::UGameplayPostProcessComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.02f; // 50Hz, no cada frame.
}

void UGameplayPostProcessComponent::BeginPlay()
{
    Super::BeginPlay();

    // Crear PostProcessComponent si el owner no tiene uno.
    AActor* Owner = GetOwner();
    if (!Owner) return;

    PostProcessComponent = Owner->FindComponentByClass<UPostProcessComponent>();
    if (!PostProcessComponent)
    {
        PostProcessComponent = NewObject<UPostProcessComponent>(Owner, TEXT("GameplayPostProcess"));
        PostProcessComponent->SetupAttachment(Owner->GetRootComponent());
        PostProcessComponent->RegisterComponent();
    }

    if (PostProcessComponent)
    {
        PostProcessComponent->Settings.bOverride_ColorGamma = true;
        PostProcessComponent->Settings.bOverride_MotionBlurAmount = true;
        PostProcessComponent->Settings.bOverride_SceneFringeIntensity = true;
        PostProcessComponent->Settings.bOverride_VignetteIntensity = true;
        PostProcessComponent->Settings.bOverride_AutoExposureBias = true;
        PostProcessComponent->Settings.bOverride_DepthOfFieldFocalDistance = true;
        PostProcessComponent->Settings.bOverride_DepthOfFieldDepthBlurAmount = true;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tick
// ─────────────────────────────────────────────────────────────────────────────
void UGameplayPostProcessComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    UpdateHealthVignette(DeltaTime);
    UpdateDamageFlash(DeltaTime);
    UpdateSpeedLines(DeltaTime);
    UpdateCrowdDust(DeltaTime);
    UpdateParanoiaVision(DeltaTime);

    // Actualizar vision de drogas.
    if (bDrugVisionActive)
    {
        DrugVisionTargetIntensity = 1.f;
    }
    else
    {
        DrugVisionTargetIntensity = 0.f;
    }
    DrugVisionIntensity = FMath::FInterpTo(DrugVisionIntensity, DrugVisionTargetIntensity, DeltaTime, 2.f);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Health Vignette
// ─────────────────────────────────────────────────────────────────────────────
void UGameplayPostProcessComponent::UpdateHealthVignette(float DeltaTime)
{
    if (!PostProcessComponent) return;
    UWorld* W = GetWorld();
    if (!W) return;

    const float HealthFrac = GetHealthFraction();
    float VignetteTarget = 0.f;

    if (HealthFrac < HealthVignetteThreshold && HealthFrac > 0.f)
    {
        // Escalar intensidad: a menos vida, más viñeta.
        const float Normalized = 1.f - (HealthFrac / HealthVignetteThreshold);
        VignetteTarget = Normalized * MaxHealthVignetteIntensity;

        // Pulso lento para baja vida (sensación de peligro).
        const float Pulse = FMath::Sin(W->GetTimeSeconds() * 3.f) * 0.1f * Normalized;
        VignetteTarget = FMath::Clamp(VignetteTarget + Pulse, 0.f, MaxHealthVignetteIntensity);
    }

    // Mezclar con otros efectos de viñeta.
    const float FlashContribution = DamageFlashIntensity * 0.3f;
    const float FinalVignette = FMath::Clamp(VignetteTarget + FlashContribution, 0.f, 1.f);

    // Sin base fija: este componente es el de gameplay (vida baja, impactos) y
    // se mezcla encima del volumen. El 0.4 constante se sumaba a la viñeta del
    // volumen incluso a vida llena y dejaba las esquinas casi negras siempre.
    PostProcessComponent->Settings.VignetteIntensity = FMath::FInterpTo(
        PostProcessComponent->Settings.VignetteIntensity, FinalVignette, DeltaTime, 5.f);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Damage Flash
// ─────────────────────────────────────────────────────────────────────────────
void UGameplayPostProcessComponent::TriggerDamageFlash(float Intensity)
{
    DamageFlashTimer = DamageFlashDuration;
    DamageFlashIntensity = FMath::Clamp(Intensity, 0.f, 1.f);
}

void UGameplayPostProcessComponent::UpdateDamageFlash(float DeltaTime)
{
    if (!PostProcessComponent || DamageFlashTimer <= 0.f) return;

    DamageFlashTimer -= DeltaTime;
    float Alpha = FMath::Clamp(DamageFlashTimer / DamageFlashDuration, 0.f, 1.f);
    Alpha *= DamageFlashIntensity;

    // Flash blanco rápido (aumentar auto-exposure).
    PostProcessComponent->Settings.AutoExposureBias = FMath::Lerp(1.f, 2.5f, Alpha);

    // Chromatic aberration temporal.
    PostProcessComponent->Settings.SceneFringeIntensity = FMath::Lerp(0.f, 5.f, Alpha);

    if (DamageFlashTimer <= 0.f)
    {
        DamageFlashIntensity = 0.f;
        // 0, no 1: dejarlo a 1 metía +1 EV permanente tras el primer golpe y
        // anulaba la compensación de exposición nocturna del volumen.
        PostProcessComponent->Settings.AutoExposureBias = 0.f;
        PostProcessComponent->Settings.SceneFringeIntensity = 0.f;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Speed Lines
// ─────────────────────────────────────────────────────────────────────────────
void UGameplayPostProcessComponent::SetSpeedLines(bool bActive, float Intensity)
{
    bSpeedLinesActive = bActive;
    SpeedLinesTargetIntensity = bActive ? FMath::Clamp(Intensity, 0.f, 1.f) : 0.f;
}

void UGameplayPostProcessComponent::UpdateSpeedLines(float DeltaTime)
{
    SpeedLinesCurrentIntensity = FMath::FInterpTo(
        SpeedLinesCurrentIntensity, SpeedLinesTargetIntensity, DeltaTime, 4.f);

    if (!PostProcessComponent) return;

    // Motion blur proporcional a la velocidad.
    const float MotionBlur = SpeedLinesCurrentIntensity * 0.8f;
    PostProcessComponent->Settings.MotionBlurAmount = FMath::FInterpTo(
        PostProcessComponent->Settings.MotionBlurAmount, MotionBlur, DeltaTime, 3.f);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Crowd Dust
// ─────────────────────────────────────────────────────────────────────────────
void UGameplayPostProcessComponent::TriggerCrowdDust(float Intensity)
{
    CrowdDustTimer = 3.0f; // 3 segundos de efecto.
    CrowdDustIntensity = FMath::Clamp(Intensity, 0.f, 1.f);
}

void UGameplayPostProcessComponent::UpdateCrowdDust(float DeltaTime)
{
    if (CrowdDustTimer > 0.f)
    {
        CrowdDustTimer -= DeltaTime;

        if (PostProcessComponent)
        {
            // Se asigna, no se acumula con Max: así el efecto se va con el
            // temporizador en vez de quedarse pegado para el resto de partida.
            const float Alpha = FMath::Clamp(CrowdDustTimer / 3.0f, 0.f, 1.f);
            PostProcessComponent->Settings.SceneFringeIntensity = CrowdDustIntensity * Alpha * 2.f;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  ADS Bloom
// ─────────────────────────────────────────────────────────────────────────────
void UGameplayPostProcessComponent::SetADSBloom(bool bActive)
{
    bADSBloomActive = bActive;
    if (PostProcessComponent)
    {
        // AADS reduce motion blur y depth of field para claridad.
        PostProcessComponent->Settings.DepthOfFieldDepthBlurAmount = bActive ? 0.2f : 0.f;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Drug Vision
// ─────────────────────────────────────────────────────────────────────────────
void UGameplayPostProcessComponent::SetDrugVision(bool bActive, float Intensity)
{
    bDrugVisionActive = bActive;
    DrugVisionTargetIntensity = bActive ? FMath::Clamp(Intensity, 0.f, 1.f) : 0.f;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Explosion Reaction
// ─────────────────────────────────────────────────────────────────────────────
void UGameplayPostProcessComponent::TriggerExplosionReaction(float Distance)
{
    const float Intensity = FMath::Clamp(1.f - Distance / 3000.f, 0.1f, 1.f);
    TriggerDamageFlash(Intensity);
    // TODO: camera shake via PlayerCameraManager when shake assets are created.
}

// ─────────────────────────────────────────────────────────────────────────────
//  Torture Methods
// ─────────────────────────────────────────────────────────────────────────────
void UGameplayPostProcessComponent::TriggerTorturePulse(float Intensity)
{
    TriggerDamageFlash(Intensity * 0.5f);
}

void UGameplayPostProcessComponent::TriggerElectrodeFlash()
{
    if (!PostProcessComponent) return;
    PostProcessComponent->Settings.AutoExposureBias = 3.f;
    PostProcessComponent->Settings.SceneFringeIntensity = 8.f;
    DamageFlashTimer = 0.1f;
    DamageFlashIntensity = 1.f;
}

void UGameplayPostProcessComponent::SetDrowningVision(bool bActive, float Intensity)
{
    // Reuse drug vision channel with drowning label — different intensity curve.
    SetDrugVision(bActive, Intensity * 0.8f);
}

void UGameplayPostProcessComponent::TriggerBeatingImpact()
{
    TriggerDamageFlash(0.8f);
}

void UGameplayPostProcessComponent::SetSleepDeprivationVision(bool bActive, float Intensity)
{
    // Blur + slight chromatic aberration for double vision feel.
    if (!PostProcessComponent) return;
    if (bActive)
    {
        PostProcessComponent->Settings.DepthOfFieldDepthBlurAmount = Intensity * 0.5f;
        PostProcessComponent->Settings.SceneFringeIntensity = Intensity * 3.f;
    }
    else
    {
        PostProcessComponent->Settings.DepthOfFieldDepthBlurAmount = 0.f;
        PostProcessComponent->Settings.SceneFringeIntensity = 0.f;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Paranoia Vision
// ─────────────────────────────────────────────────────────────────────────────
void UGameplayPostProcessComponent::SetParanoiaLevel(float Paranoia01)
{
    ParanoiaLevel01 = FMath::Clamp(Paranoia01, 0.f, 1.f);
}

void UGameplayPostProcessComponent::UpdateParanoiaVision(float DeltaTime)
{
    if (!PostProcessComponent || ParanoiaLevel01 <= 0.01f)
    {
        // Fade out if paranoia dropped.
        ParanoiaDesaturation = FMath::FInterpTo(ParanoiaDesaturation, 0.f, DeltaTime, 3.f);
        ParanoiaChromatic = FMath::FInterpTo(ParanoiaChromatic, 0.f, DeltaTime, 3.f);
        ParanoiaVignetteAmount = FMath::FInterpTo(ParanoiaVignetteAmount, 0.f, DeltaTime, 3.f);
        if (ParanoiaDesaturation < 0.01f) return;
    }

    const float P = ParanoiaLevel01;

    // Desaturation: world goes grey. Reduces color gamma.
    const float TargetDesat = FMath::Lerp(0.f, 0.65f, P);
    ParanoiaDesaturation = FMath::FInterpTo(ParanoiaDesaturation, TargetDesat, DeltaTime, 2.f);

    // Apply desaturation via color gamma (lower = more grey).
    const float Gamma = FMath::Lerp(1.f, 0.4f, ParanoiaDesaturation);
    PostProcessComponent->Settings.ColorGamma = FLinearColor(Gamma, Gamma, Gamma, 1.f);

    // Chromatic aberration: things look "wrong" at edges.
    const float TargetCA = FMath::Lerp(0.f, 6.f, P);
    ParanoiaChromatic = FMath::FInterpTo(ParanoiaChromatic, TargetCA, DeltaTime, 3.f);
    PostProcessComponent->Settings.SceneFringeIntensity = ParanoiaChromatic;

    // Vignette: dark edges close in.
    const float TargetVig = FMath::Lerp(0.f, 0.55f, P);
    ParanoiaVignetteAmount = FMath::FInterpTo(ParanoiaVignetteAmount, TargetVig, DeltaTime, 2.f);
    PostProcessComponent->Settings.VignetteIntensity = ParanoiaVignetteAmount;

    // Screen tear flicker at extreme paranoia (>85%).
    if (P >= 0.85f)
    {
        ParanoiaTearTimer -= DeltaTime;
        if (ParanoiaTearTimer <= 0.f)
        {
            // Random flicker: briefly spike chromatic aberration.
            PostProcessComponent->Settings.SceneFringeIntensity = ParanoiaChromatic + FMath::FRandRange(0.f, 10.f);
            ParanoiaTearTimer = FMath::FRandRange(0.1f, 0.5f);
        }
    }

    // Motion blur increase: world feels unstable.
    PostProcessComponent->Settings.MotionBlurAmount = FMath::FInterpTo(
        PostProcessComponent->Settings.MotionBlurAmount,
        P * 0.4f, DeltaTime, 2.f);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Utilities
// ─────────────────────────────────────────────────────────────────────────────
float UGameplayPostProcessComponent::GetHealthFraction() const
{
    if (const AAlsasuaCharacter* Character = Cast<AAlsasuaCharacter>(GetOwner()))
    {
        const float Health = Character->GetHealth();
        const float MaxHealth = Character->GetVidaMax();
        return (MaxHealth > 0.f) ? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 1.f;
    }
    return 1.f;
}
