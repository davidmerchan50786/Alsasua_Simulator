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

    // TEMP debug: dump PP state every 2s
    if (GetWorld())
    {
        static double UltLog = 0.0;
        const double T = GetWorld()->GetTimeSeconds();
        if (T - UltLog > 2.0)
        {
            UltLog = T;
            UE_LOG(LogTemp, Warning, TEXT("[PPDBG] drug=%d(%.2f) paranoia=%.2f vig=%.2f fringe=%.2f exposure=%.2f speed=%d motblur=%.2f gamma=%s"),
                (int)bDrugVisionActive, DrugVisionIntensity, ParanoiaLevel01,
                PostProcessComponent ? PostProcessComponent->Settings.VignetteIntensity : -1.f,
                PostProcessComponent ? PostProcessComponent->Settings.SceneFringeIntensity : -1.f,
                PostProcessComponent ? PostProcessComponent->Settings.AutoExposureBias : -1.f,
                (int)bSpeedLinesActive,
                PostProcessComponent ? PostProcessComponent->Settings.MotionBlurAmount : -1.f,
                *PostProcessComponent->Settings.ColorGamma.ToString());
        }
    }

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

    // Camera shake via PlayerCameraManager — procedural, no asset needed.
    if (APawn* Pawn = Cast<APawn>(GetOwner()))
    {
        if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
        {
            if (APlayerCameraManager* Cam = PC->PlayerCameraManager)
            {
                Cam->StartCameraShake(ExplosionShakeClass, Intensity);
            }
        }
    }
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
    if (!PostProcessComponent)
    {
        ParanoiaDesaturation = FMath::FInterpTo(ParanoiaDesaturation, 0.f, DeltaTime, 3.f);
        ParanoiaChromatic = FMath::FInterpTo(ParanoiaChromatic, 0.f, DeltaTime, 3.f);
        ParanoiaVignetteAmount = FMath::FInterpTo(ParanoiaVignetteAmount, 0.f, DeltaTime, 3.f);
        return;
    }

    if (ParanoiaLevel01 <= 0.01f && ParanoiaDesaturation < 0.01f) return;

    const float P = FMath::Clamp(ParanoiaLevel01, 0.f, 1.f);
    UWorld* W = GetWorld();
    const float Time = W ? W->GetTimeSeconds() : 0.f;

    // ── Desaturation: world goes grey ──────────────────────────────────────
    const float TargetDesat = FMath::Lerp(0.f, 0.7f, P);
    ParanoiaDesaturation = FMath::FInterpTo(ParanoiaDesaturation, TargetDesat, DeltaTime, 2.f);
    const float Gamma = FMath::Lerp(1.f, 0.35f, ParanoiaDesaturation);
    PostProcessComponent->Settings.ColorGamma = FVector4(Gamma, Gamma, Gamma, 1.f);

    // ── Chromatic aberration: things look "wrong" at edges ─────────────────
    const float BaseCA = FMath::Lerp(0.f, 7.f, P);
    // Subtle pulsing CA for living/unnerving feel.
    const float CAPulse = FMath::Sin(Time * 2.f) * P * 1.5f;
    ParanoiaChromatic = FMath::FInterpTo(ParanoiaChromatic, BaseCA + CAPulse, DeltaTime, 3.f);
    PostProcessComponent->Settings.SceneFringeIntensity = ParanoiaChromatic;

    // ── Vignette: dark edges close in ──────────────────────────────────────
    const float TargetVig = FMath::Lerp(0.f, 0.6f, P);
    ParanoiaVignetteAmount = FMath::FInterpTo(ParanoiaVignetteAmount, TargetVig, DeltaTime, 2.f);
    // Add breathing pulse to vignette.
    const float VignettePulse = FMath::Sin(Time * 1.5f) * P * 0.08f;
    PostProcessComponent->Settings.VignetteIntensity = ParanoiaVignetteAmount + VignettePulse;

    // ── Depth of Field: background blurs, world feels suffocating ──────────
    PostProcessComponent->Settings.DepthOfFieldFocalDistance = FMath::FInterpTo(
        PostProcessComponent->Settings.DepthOfFieldFocalDistance,
        FMath::Lerp(1000.f, 200.f, P), DeltaTime, 1.5f);
    PostProcessComponent->Settings.DepthOfFieldDepthBlurAmount = FMath::FInterpTo(
        PostProcessComponent->Settings.DepthOfFieldDepthBlurAmount,
        P * 0.4f, DeltaTime, 2.f);

    // ── Exposure: world gets slightly darker ───────────────────────────────
    PostProcessComponent->Settings.AutoExposureBias = FMath::FInterpTo(
        PostProcessComponent->Settings.AutoExposureBias,
        FMath::Lerp(0.f, -0.8f, P), DeltaTime, 2.f);

    // ── Motion blur: world feels unstable ──────────────────────────────────
    PostProcessComponent->Settings.MotionBlurAmount = FMath::FInterpTo(
        PostProcessComponent->Settings.MotionBlurAmount,
        P * 0.5f, DeltaTime, 2.f);

    // ── Screen tear / flicker at extreme paranoia (>85%) ──────────────────
    if (P >= 0.85f)
    {
        ParanoiaTearTimer -= DeltaTime;
        if (ParanoiaTearTimer <= 0.f)
        {
            // Random heavy flicker: spike everything.
            PostProcessComponent->Settings.SceneFringeIntensity = ParanoiaChromatic + FMath::FRandRange(5.f, 15.f);
            PostProcessComponent->Settings.AutoExposureBias = FMath::FRandRange(-2.f, 1.f);
            PostProcessComponent->Settings.ColorGamma = FVector4(
                FMath::FRandRange(0.2f, 0.5f), FMath::FRandRange(0.2f, 0.5f),
                FMath::FRandRange(0.2f, 0.5f), 1.f);
            ParanoiaTearTimer = FMath::FRandRange(0.05f, 0.3f);
        }
    }

    // ── Color tint: slight cold/blue shift at high paranoia ────────────────
    if (P > 0.3f)
    {
        const float ColdShift = FMath::Lerp(0.f, 0.15f, (P - 0.3f) / 0.7f);
        FVector4 CurrentGamma = PostProcessComponent->Settings.ColorGamma;
        CurrentGamma.Z = FMath::Min(CurrentGamma.Z + ColdShift, 1.f);
        CurrentGamma.X = FMath::Max(CurrentGamma.X - ColdShift * 0.5f, 0.2f);
        PostProcessComponent->Settings.ColorGamma = CurrentGamma;
    }
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
