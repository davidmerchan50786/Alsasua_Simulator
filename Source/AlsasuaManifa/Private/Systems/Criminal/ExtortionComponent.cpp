#include "Systems/Criminal/ExtortionComponent.h"
#include "Engine/World.h"
#include "Engine/TimerHandle.h"

UExtortionComponent::UExtortionComponent() { PrimaryComponentTick.bCanEverTick = false; }

void UExtortionComponent::BeginPlay() {
    Super::BeginPlay();
    if (GetWorld())
    {
        LastPaymentTime = GetWorld()->GetTimeSeconds();
        GetWorld()->GetTimerManager().SetTimer(TimerHandle_Due, this, &UExtortionComponent::OnPaymentOverdue,
            DueFrequencySeconds, true);
    }
}

void UExtortionComponent::ProcessPayment(float Amount) {
    if (GetWorld()) LastPaymentTime = GetWorld()->GetTimeSeconds();
    PaidAmount += Amount;
    RemainingDebt = FMath::Max(0.f, RemainingDebt - Amount);

    UE_LOG(LogTemp, Log, TEXT("Extorsión: pago de %f registrado. Deuda restante: %f"), Amount, RemainingDebt);
}

void UExtortionComponent::OnPaymentOverdue() {
    if (RemainingDebt <= 0.f) return;

    UE_LOG(LogTemp, Warning, TEXT("Extorsión: ¡PLAZO VENCIDO! Deuda pendiente: %f"), RemainingDebt);

    // Increase wanted level as consequence of overdue debt.
    if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
    {
        // Could broadcast a delegate here for UI notification.
    }
}
