#include "Gameplay/Detention/DetentionMinigameComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "GameFramework/Actor.h"
#include "Inventory/AlsasuaInventoryComponent.h"
#include "AlsasuaCharacter.h"
#include "AlsasuaAttributeSet.h"
#include "GAS/AlsasuaAbilitySystemComponent.h"
#include "Character/GameplayPostProcessComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

UDetentionMinigameComponent::UDetentionMinigameComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UDetentionMinigameComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UDetentionMinigameComponent::StartMinigame(float InDuration, float DifficultyMultiplier)
{
    if (CurrentState != EDetentionState::Idle) return;
    Duration = InDuration;
    Difficulty = DifficultyMultiplier;
    Elapsed = 0.f;
    CurrentResistance = 0.f;
    StressLevel = 0.f;
    ActiveMethod = EInterrogationMethod::None;
    BeatingTimer = 0.f;
    bStunned = false;
    StunTimer = 0.f;
    SleepHallucinationTimer = 0.f;
    ElectrodeFlashTimer = 0.f;
    WaterDmgTimer = 0.f;

    NextQTETime = FMath::RandRange(QTEIntervalRange.X, QTEIntervalRange.Y);
    bQTEActive = false;
    CurrentState = EDetentionState::Interrogating;

    OnDetentionStarted.Broadcast();
}

void UDetentionMinigameComponent::StopMinigame(bool bForceFail)
{
    if (CurrentState == EDetentionState::Idle) return;
    FinishMinigame(!bForceFail && CurrentResistance >= SuccessThreshold);
}

void UDetentionMinigameComponent::ApplyTortureMethod(EInterrogationMethod Method)
{
    ActiveMethod = Method;
    if (CurrentState == EDetentionState::Arrested)
        CurrentState = EDetentionState::Interrogating;
}

void UDetentionMinigameComponent::RegisterInputPress()
{
    if (CurrentState != EDetentionState::Arrested && CurrentState != EDetentionState::Resisting) return;
    if (bStunned) return; // Beating stun blocks input.

    CurrentResistance += MashPowerPerPress * Difficulty * 1.0f;
    CurrentState = EDetentionState::Resisting;

    if (bQTEActive)
    {
        ResolveQTE(true);
    }
}

void UDetentionMinigameComponent::UseInventoryItemDuringMinigame(FName ItemID)
{
    AActor* Owner = GetOwner();
    if (!Owner) return;
    UAlsasuaInventoryComponent* Inv = Owner->FindComponentByClass<UAlsasuaInventoryComponent>();
    if (!Inv) return;

    if (Inv->RemoveItem(ItemID, 1))
    {
        StressLevel = FMath::Max(0.f, StressLevel - 20.f);
        OnDetentionEnded.Broadcast();
    }
}

void UDetentionMinigameComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (CurrentState == EDetentionState::Idle || CurrentState == EDetentionState::Escaped || CurrentState == EDetentionState::Surrendered) return;

    Elapsed += DeltaTime;

    // Stun recovery
    if (bStunned)
    {
        StunTimer -= DeltaTime;
        if (StunTimer <= 0.f) bStunned = false;
        return; // Can't do anything while stunned.
    }

    // Base stress from interrogation
    ApplyStress(StressIncreaseRate * DeltaTime * Difficulty);

    // Per-method effects
    switch (ActiveMethod)
    {
    case EInterrogationMethod::Electrodes:    TickElectrodes(DeltaTime); break;
    case EInterrogationMethod::WaterBoarding: TickWaterBoarding(DeltaTime); break;
    case EInterrogationMethod::SleepDeprivation: TickSleepDeprivation(DeltaTime); break;
    case EInterrogationMethod::Beating:       TickBeating(DeltaTime); break;
    case EInterrogationMethod::Threats:       TickThreats(DeltaTime); break;
    default: break;
    }

    // QTE scheduling (not during stun)
    if (!bQTEActive && Elapsed >= NextQTETime && Elapsed <= Duration)
    {
        StartQTEWindow();
    }

    if (bQTEActive)
    {
        QTEWindowDuration -= DeltaTime;
        if (QTEWindowDuration <= 0.f)
        {
            ResolveQTE(false);
        }
    }

    // Success check
    if (CurrentResistance >= SuccessThreshold)
    {
        FinishMinigame(true);
    }

    // Time out
    if (Elapsed >= Duration)
    {
        FinishMinigame(CurrentResistance >= SuccessThreshold);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  Torture methods
// ═══════════════════════════════════════════════════════════════════════════

void UDetentionMinigameComponent::TickElectrodes(float DeltaTime)
{
    // Direct health damage + stress spike every 2s + screen flash.
    ApplyDamageToPlayer(FMath::RoundToInt32(ElectrodeDamagePerSec * DeltaTime));
    ElectrodeFlashTimer += DeltaTime;
    if (ElectrodeFlashTimer >= 2.f)
    {
        ElectrodeFlashTimer = 0.f;
        StressLevel = FMath::Min(100.f, StressLevel + ElectrodeStressSpike);
        // Screen shake via post-process
        if (AActor* O = GetOwner())
            if (UGameplayPostProcessComponent* PP = O->FindComponentByClass<UGameplayPostProcessComponent>())
                PP->TriggerDamageFlash(0.8f);
    }
}

void UDetentionMinigameComponent::TickWaterBoarding(float DeltaTime)
{
    // Heavy stamina drain + slow health drain (asphyxiation) + speed lines (breathlessness).
    if (AActor* O = GetOwner())
    {
        if (AAlsasuaCharacter* Ch = Cast<AAlsasuaCharacter>(O))
        {
            if (UAlsasuaAttributeSet* Attr = Ch->GetAttributeSet())
            {
                float NewStamina = FMath::Max(0.f, Attr->GetStamina() - WaterStaminaDrain * DeltaTime);
                Attr->SetStamina(NewStamina);
            }
        }
        if (UGameplayPostProcessComponent* PP = O->FindComponentByClass<UGameplayPostProcessComponent>())
            PP->SetSpeedLines(true, 0.6f);
    }
    // Slow drowning damage every 3s.
    WaterDmgTimer += DeltaTime;
    if (WaterDmgTimer >= 3.f)
    {
        WaterDmgTimer = 0.f;
        ApplyDamageToPlayer(3);
    }
}

void UDetentionMinigameComponent::TickSleepDeprivation(float DeltaTime)
{
    // Slow stress accumulation + visual hallucination timer.
    StressLevel = FMath::Min(100.f, StressLevel + SleepStressPerSec * DeltaTime);
    SleepHallucinationTimer += DeltaTime;
    // Hallucination effect: trigger drug vision at low intensity every 5s.
    if (SleepHallucinationTimer >= 5.f)
    {
        SleepHallucinationTimer = 0.f;
        if (AActor* O = GetOwner())
            if (UGameplayPostProcessComponent* PP = O->FindComponentByClass<UGameplayPostProcessComponent>())
                PP->SetDrugVision(true, 0.3f);
    }
    // Slow stamina drain (exhaustion).
    if (AActor* O = GetOwner())
        if (AAlsasuaCharacter* Ch = Cast<AAlsasuaCharacter>(O))
            if (UAlsasuaAttributeSet* Attr = Ch->GetAttributeSet())
                Attr->SetStamina(FMath::Max(0.f, Attr->GetStamina() - 3.f * DeltaTime));
}

void UDetentionMinigameComponent::TickBeating(float DeltaTime)
{
    // Direct damage + periodic stun + red flash.
    BeatingTimer += DeltaTime;
    if (BeatingTimer >= BeatingStunInterval)
    {
        BeatingTimer = 0.f;
        ApplyDamageToPlayer(BeatingDamage);
        bStunned = true;
        StunTimer = 1.2f; // 1.2s stun — can't input.
        if (AActor* O = GetOwner())
            if (UGameplayPostProcessComponent* PP = O->FindComponentByClass<UGameplayPostProcessComponent>())
                PP->TriggerDamageFlash(1.0f);
    }
}

void UDetentionMinigameComponent::TickThreats(float DeltaTime)
{
    // Psychological only — stress increases, no physical damage.
    // Threats also drain popular support (fear).
    StressLevel = FMath::Min(100.f, StressLevel + ThreatsStressPerSec * DeltaTime);
}

// ═══════════════════════════════════════════════════════════════════════════
//  Helpers
// ═══════════════════════════════════════════════════════════════════════════

void UDetentionMinigameComponent::StartQTEWindow()
{
    bQTEActive = true;
    QTEWindowDuration = 1.5f;
    OnQTEWindow.Broadcast(QTEWindowDuration);
}

void UDetentionMinigameComponent::ResolveQTE(bool bSuccess)
{
    bQTEActive = false;
    NextQTETime = Elapsed + FMath::RandRange(QTEIntervalRange.X, QTEIntervalRange.Y);
    OnQTEResult.Broadcast(bSuccess);

    if (bSuccess)
    {
        CurrentResistance += 10.f * Difficulty;
    }
    else
    {
        StressLevel += 10.f * Difficulty;
        // QTE fail under torture: extra pain damage.
        if (ActiveMethod == EInterrogationMethod::Electrodes)
            ApplyDamageToPlayer(5);
        else if (ActiveMethod == EInterrogationMethod::Beating)
            ApplyDamageToPlayer(3);
    }
}

void UDetentionMinigameComponent::ApplyStress(float Delta)
{
    StressLevel = FMath::Clamp(StressLevel + Delta, 0.f, 100.f);

    if (AActor* Owner = GetOwner())
    {
        if (AAlsasuaCharacter* Character = Cast<AAlsasuaCharacter>(Owner))
        {
            if (UAlsasuaAttributeSet* Attr = Character->GetAttributeSet())
            {
                float StaminaDrain = Delta * 0.5f;
                float NewStamina = FMath::Max(0.f, Attr->GetStamina() - StaminaDrain);
                Attr->SetStamina(NewStamina);
            }
        }
    }
}

void UDetentionMinigameComponent::ApplyDamageToPlayer(int32 Amount)
{
    if (AActor* Owner = GetOwner())
    {
        if (IDamageable* Dmg = Cast<IDamageable>(Owner))
        {
            Dmg->RecibirDano(Amount, Owner->GetActorLocation(), ETipoDano::Impacto);
        }
    }
}

void UDetentionMinigameComponent::FinishMinigame(bool bEscaped)
{
    CurrentState = bEscaped ? EDetentionState::Escaped : EDetentionState::Surrendered;
    OnDetentionResult.Broadcast(bEscaped);
    OnDetentionEnded.Broadcast();

    // Reset timers
    Elapsed = 0.f;
    bQTEActive = false;
    bStunned = false;
    ActiveMethod = EInterrogationMethod::None;

    // Clean up post-process effects
    if (AActor* O = GetOwner())
    {
        if (UGameplayPostProcessComponent* PP = O->FindComponentByClass<UGameplayPostProcessComponent>())
        {
            PP->SetSpeedLines(false);
            PP->SetDrugVision(false);
        }
    }
}
