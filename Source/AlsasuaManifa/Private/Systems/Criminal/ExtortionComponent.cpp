#include "Systems/Criminal/ExtortionComponent.h"
#include "Kismet/GameplayStatics.h"

UExtortionComponent::UExtortionComponent() { PrimaryComponentTick.bCanEverTick = false; }

void UExtortionComponent::BeginPlay() {
    Super::BeginPlay();
    LastPaymentTime = GetWorld()->GetTimeSeconds();
}

void UExtortionComponent::ProcessPayment(float Amount) {
    // Reducir deuda o resetear timer
    LastPaymentTime = GetWorld()->GetTimeSeconds();
}
