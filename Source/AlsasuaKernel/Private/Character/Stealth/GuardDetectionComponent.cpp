#include "Character/Stealth/GuardDetectionComponent.h"
#include "Interaction/AlsasuaStealthProfileInterface.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

FOnAnyGuardCombat UGuardDetectionComponent::OnAnyGuardEnterCombat;

UGuardDetectionComponent::UGuardDetectionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f; // 10Hz detection, no cada frame.
}

void UGuardDetectionComponent::BeginPlay()
{
    Super::BeginPlay();
    CachedPlayerTarget = FindPlayerPawn();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tick
// ─────────────────────────────────────────────────────────────────────────────
void UGuardDetectionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Refrescar cache del jugador cada pocos segundos.
    if (!IsValid(CachedPlayerTarget))
    {
        CachedPlayerTarget = FindPlayerPawn();
    }

    TickVisionCheck(DeltaTime);
    TickHearingCheck(DeltaTime);
    TickStateTimer(DeltaTime);
    UpdateMemory(DeltaTime);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Vision Check
// ─────────────────────────────────────────────────────────────────────────────
void UGuardDetectionComponent::TickVisionCheck(float DeltaTime)
{
    if (CurrentState == EGuardAlertState::Combat) return; // Ya está en combate.
    if (!IsValid(CachedPlayerTarget)) return;
    if (!GetOwner()) return;

    // Si el jugador está hidden in crowd, es invisible para la detección visual.
    if (CachedPlayerTarget->ActorHasTag(FName("HiddenInCrowd")))
    {
        return;
    }

    if (CanSeeTarget(CachedPlayerTarget))
    {
        LastSeenLocation = CachedPlayerTarget->GetActorLocation();
        LastKnownPlayerLocation = LastSeenLocation;

        // Record encounter in guard memory.
        RecordEncounter(LastSeenLocation);

        // Escalar estado según distancia.
        const float Distance = FVector::Dist(GetOwner()->GetActorLocation(), LastSeenLocation);

        // Apply memory aggression bonus — more encounters = lower thresholds.
        const float CombatThreshold = 900.f + MemoryAggressionBonus * 5.f;
        const float AlertThreshold = 1800.f + MemoryAggressionBonus * 10.f;

        if (Distance < CombatThreshold)
        {
            // Muy cerca → combate directo.
            TransitionToState(EGuardAlertState::Combat);
        }
        else if (Distance < AlertThreshold)
        {
            // Cerca → alert.
            TransitionToState(EGuardAlertState::Alert);
        }
        else
        {
            // Lejos pero visible → suspicious.
            TransitionToState(EGuardAlertState::Suspicious);
        }

        OnDetectedPlayer.Broadcast(GetOwner(), LastSeenLocation);
    }
    // Check for remembered encounter location.
    else if (HasRecentMemory() && CurrentState == EGuardAlertState::Idle)
    {
        // Guard remembers seeing player here recently → investigate.
        LastKnownPlayerLocation = MemoryLocation;
        TransitionToState(EGuardAlertState::Suspicious);
    }
}

bool UGuardDetectionComponent::CanSeeTarget(const AActor* Target) const
{
    return IsInVisionCone(Target) && HasLineOfSight(Target);
}

bool UGuardDetectionComponent::IsInVisionCone(const AActor* Target) const
{
    if (!Target || !GetOwner()) return false;

    const FVector OwnerLoc = GetOwner()->GetActorLocation();
    const FVector OwnerForward = GetOwner()->GetActorForwardVector();
    const FVector ToTarget = (Target->GetActorLocation() - OwnerLoc);

    // Radio de visión efectivo: se reduce si el objetivo lleva disfraz
    // (o cualquier componente que publique perfil de sigilo).
    float EffectiveRange = VisionRange * DetectionRangeMultiplier;
    if (Target->GetClass()->ImplementsInterface(UAlsasuaStealthProfile::StaticClass()))
    {
        EffectiveRange *= IAlsasuaStealthProfile::Execute_GetVisionMultiplier(Target);
    }

    const float DistSq = ToTarget.SizeSquared();
    if (DistSq > EffectiveRange * EffectiveRange) return false;

    const FVector ToTargetDir = ToTarget.GetSafeNormal();
    const float DotProduct = FVector::DotProduct(OwnerForward, ToTargetDir);
    const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(VisionConeAngle * 0.5f));

    return DotProduct >= CosHalfAngle;
}

bool UGuardDetectionComponent::HasLineOfSight(const AActor* Target) const
{
    if (!Target || !GetOwner() || !GetWorld()) return false;

    const FVector EyeLocation = GetOwner()->GetActorLocation() + FVector(0, 0, EyeHeight);
    const FVector TargetLocation = Target->GetActorLocation() + FVector(0, 0, TargetHeight);

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(GetOwner());
    Params.AddIgnoredActor(Target);
    Params.bTraceComplex = false;
    Params.bReturnPhysicalMaterial = false;

    const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
        Hit, EyeLocation, TargetLocation, ECC_Visibility, Params);

    // Si no hay bloqueo, o si el bloqueador es el propio target → se ve.
    return !bBlocked || Hit.GetActor() == Target;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Hearing
// ─────────────────────────────────────────────────────────────────────────────
void UGuardDetectionComponent::TickHearingCheck(float DeltaTime)
{
    if (HearingCooldown > 0.f)
    {
        HearingCooldown -= DeltaTime;
    }
}

void UGuardDetectionComponent::ReportNoise(FVector NoiseLocation, float Loudness)
{
    if (HearingCooldown > 0.f) return;
    if (!GetOwner()) return;

    // Factor de reducción de ruido por disfraz del jugador que hizo el ruido.
    float EffectiveLoudness = Loudness;
    if (IsValid(CachedPlayerTarget) &&
        CachedPlayerTarget->GetClass()->ImplementsInterface(UAlsasuaStealthProfile::StaticClass()))
    {
        EffectiveLoudness *= IAlsasuaStealthProfile::Execute_GetNoiseDampening(CachedPlayerTarget.Get());
    }

    const float Dist = FVector::Dist(GetOwner()->GetActorLocation(), NoiseLocation);
    const float EffectiveRange = HearingRange * EffectiveLoudness;

    if (Dist > EffectiveRange) return;

    const bool bLoudNoise = EffectiveLoudness > 0.7f;

    LastKnownPlayerLocation = NoiseLocation;
    HearingCooldown = 1.0f;

    if (bLoudNoise)
    {
        TransitionToState(EGuardAlertState::Alert);
    }
    else
    {
        if (CurrentState == EGuardAlertState::Idle)
        {
            TransitionToState(EGuardAlertState::Suspicious);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  State Machine
// ─────────────────────────────────────────────────────────────────────────────
void UGuardDetectionComponent::TransitionToState(EGuardAlertState NewState)
{
    if (NewState == CurrentState) return;

    const EGuardAlertState OldState = CurrentState;
    CurrentState = NewState;
    StateTimer = 0.f;

    switch (NewState)
    {
    case EGuardAlertState::Idle:
        bIsChasing = false;
        break;

    case EGuardAlertState::Suspicious:
        bIsChasing = false;
        break;

    case EGuardAlertState::Alert:
        bIsChasing = true;
        break;

    case EGuardAlertState::Combat:
        bIsChasing = true;
        break;
    }

    OnAlertStateChanged.Broadcast(GetOwner(), NewState, OldState);

    if (NewState == EGuardAlertState::Combat && OldState != EGuardAlertState::Combat)
        OnAnyGuardEnterCombat.Broadcast(GetOwner());
}

void UGuardDetectionComponent::TickStateTimer(float DeltaTime)
{
    StateTimer += DeltaTime;

    switch (CurrentState)
    {
    case EGuardAlertState::Suspicious:
        if (StateTimer >= SuspiciousDuration)
        {
            TransitionToState(EGuardAlertState::Idle);
        }
        break;

    case EGuardAlertState::Alert:
        if (StateTimer >= AlertDuration)
        {
            // Deescalar: si aún ve al jugador, mantener alert; si no, bajar a suspicious.
            if (IsValid(CachedPlayerTarget) && CanSeeTarget(CachedPlayerTarget))
            {
                StateTimer = 0.f; // Resetear timer, mantener alert.
            }
            else
            {
                TransitionToState(EGuardAlertState::Suspicious);
            }
        }
        break;

    case EGuardAlertState::Combat:
        if (StateTimer >= CombatDuration)
        {
            if (IsValid(CachedPlayerTarget) && CanSeeTarget(CachedPlayerTarget))
            {
                StateTimer = 0.f; // Resetear timer, mantener combat.
            }
            else
            {
                // Perdió de vista → bajar a alert.
                TransitionToState(EGuardAlertState::Alert);
            }
        }
        break;

    default:
        break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Public API
// ─────────────────────────────────────────────────────────────────────────────
void UGuardDetectionComponent::ForceAlertState(EGuardAlertState NewState)
{
    TransitionToState(NewState);
}

void UGuardDetectionComponent::ResetToIdle()
{
    bIsChasing = false;
    LastKnownPlayerLocation = FVector::ZeroVector;
    LastSeenLocation = FVector::ZeroVector;
    StateTimer = 0.f;

    // Clear memory on full reset (e.g., player respawn).
    bHasMemory = false;
    MemoryTimer = 0.f;
    EncounterCount = 0;
    MemoryAggressionBonus = 0.f;

    TransitionToState(EGuardAlertState::Idle);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Utilities
// ─────────────────────────────────────────────────────────────────────────────
AActor* UGuardDetectionComponent::FindPlayerPawn() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    return PC ? Cast<AActor>(PC->GetPawn()) : nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
//  NPC Memory — guards remember previous encounters
// ─────────────────────────────────────────────────────────────────────────────
void UGuardDetectionComponent::UpdateMemory(float DeltaTime)
{
    if (!bHasMemory) return;

    MemoryTimer += DeltaTime;

    // Memory fades over time.
    if (MemoryTimer >= MemoryDuration)
    {
        bHasMemory = false;
        MemoryAggressionBonus = FMath::Max(0.f, MemoryAggressionBonus - 0.2f); // Gradual fade.
    }
}

void UGuardDetectionComponent::RecordEncounter(FVector Location)
{
    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

    // First encounter or fresh memory.
    if (!bHasMemory || (Now - LastEncounterTime) > MemoryDuration * 0.5f)
    {
        EncounterCount++;
        MemoryAggressionBonus = FMath::Min(30.f, MemoryAggressionBonus + 10.f); // Up to +30 suspicion.
    }

    bHasMemory = true;
    MemoryTimer = 0.f;
    MemoryLocation = Location;
    LastEncounterTime = Now;
}

bool UGuardDetectionComponent::HasRecentMemory() const
{
    return bHasMemory && MemoryTimer < MemoryDuration;
}
