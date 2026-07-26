#include "Gameplay/Detention/DetentionMinigameComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "GameFramework/Actor.h"
#include "Inventory/AlsasuaInventoryComponent.h"
#include "AlsasuaCharacter.h"
#include "AlsasuaAttributeSet.h"
#include "GAS/AlsasuaAbilitySystemComponent.h"

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

    NextQTETime = FMath::RandRange(QTEIntervalRange.X, QTEIntervalRange.Y);
    bQTEActive = false;
    CurrentState = EDetentionState::Arrested;

    OnDetentionStarted.Broadcast();
}

void UDetentionMinigameComponent::StopMinigame(bool bForceFail)
{
    if (CurrentState == EDetentionState::Idle) return;
    FinishMinigame(!bForceFail && CurrentResistance >= SuccessThreshold);
}

void UDetentionMinigameComponent::RegisterInputPress()
{
    if (CurrentState != EDetentionState::Arrested && CurrentState != EDetentionState::Resisting) return;

    // Increase resistance
    CurrentResistance += MashPowerPerPress * Difficulty * 1.0f;
    CurrentState = EDetentionState::Resisting;

    // If QTE active, consider success
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

    // Try remove item as used
    if (Inv->RemoveItem(ItemID, 1))
    {
        // Apply benefit: reduce stress
        StressLevel = FMath::Max(0.f, StressLevel - 20.f);
        OnDetentionEnded.Broadcast();
    }
}

void UDetentionMinigameComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (CurrentState == EDetentionState::Idle) return;

    Elapsed += DeltaTime;
    ApplyStress(StressIncreaseRate * DeltaTime * Difficulty);

    // QTE scheduling
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
                // Stress drains Stamina proportionally.
                float StaminaDrain = Delta * 0.5f;
                float NewStamina = FMath::Max(0.f, Attr->GetStamina() - StaminaDrain);
                Attr->SetStamina(NewStamina);
            }
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
}
