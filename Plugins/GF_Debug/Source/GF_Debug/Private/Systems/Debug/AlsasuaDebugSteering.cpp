#include "Systems/Debug/AlsasuaDebugSteering.h"
#include "Items/Explosives/IncendiaryCharge.h"
#include "Systemics/Fire/FirePropagationComponent.h"

#include "Kismet/GameplayStatics.h"

void UAlsasuaDebugSteering::Alsasua_DetonateAll() {
    TArray<AActor*> Charges;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AIncendiaryCharge::StaticClass(), Charges);
    for(AActor* C : Charges) {
        if(AIncendiaryCharge* Charge = Cast<AIncendiaryCharge>(C)) Charge->Detonate();
    }
}

void UAlsasuaDebugSteering::Alsasua_ClearAllFire() {
    TArray<AActor*> FireComps;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), FireComps);
    for(AActor* A : FireComps) {
        if(auto* Comp = A->FindComponentByClass<UFirePropagationComponent>()) {
            A->Destroy();
        }
    }
}

void UAlsasuaDebugSteering::Alsasua_SetSupport(float NewValue) {
    UE_LOG(LogTemp, Warning, TEXT("DEBUG: SetSupport(%.1f) — reconnect to Manifa AttributeSet"), NewValue);
}

void UAlsasuaDebugSteering::Alsasua_ToggleDebugVisuals() {
    bShowDebugVisuals = !bShowDebugVisuals;
}
