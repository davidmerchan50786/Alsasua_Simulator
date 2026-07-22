#include "Systems/Debug/AlsasuaDebugSteering.h"
#include "Items/Explosives/IncendiaryCharge.h"
#include "Systems/Fire/FirePropagationComponent.h"
#include "AlsasuaCharacter.h"
#include "AlsasuaAttributeSet.h"
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
            A->Destroy(); // O simplemente apagar el componente
        }
    }
}

void UAlsasuaDebugSteering::Alsasua_SetSupport(float NewValue) {
    if (AAlsasuaCharacter* Player = Cast<AAlsasuaCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0))) {
        if (UAlsasuaAttributeSet* Attr = Player->GetAttributeSet()) {
            Attr->SetPopularSupport(NewValue);
        }
    }
}

void UAlsasuaDebugSteering::Alsasua_ToggleDebugVisuals() {
    bShowDebugVisuals = !bShowDebugVisuals;
}
