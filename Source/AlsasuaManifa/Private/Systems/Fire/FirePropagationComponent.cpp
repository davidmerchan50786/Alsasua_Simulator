#include "Systems/Fire/FirePropagationComponent.h"
#include "Kismet/GameplayStatics.h"

UFirePropagationComponent::UFirePropagationComponent() {
    PrimaryComponentTick.bCanEverTick = true;
}

void UFirePropagationComponent::StartBurning(float Duration) {
    RemainingBurnTime = Duration;
    bIsBurning = true;
}

void UFirePropagationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!bIsBurning) return;

    RemainingBurnTime -= DeltaTime;
    if (RemainingBurnTime <= 0) bIsBurning = false;

    // Daño en área y propagación
    SpreadTimer += DeltaTime;
    if (SpreadTimer >= 1.0f) {
        SpreadTimer = 0.f;
        UGameplayStatics::ApplyRadialDamage(GetWorld(), DamagePerSecond, GetOwner()->GetActorLocation(), SpreadRadius, nullptr, TArray<AActor*>(), GetOwner());
    }
}
