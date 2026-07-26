#include "World/AlsasuaWorldSubsystem.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Engine/World.h"

void UAlsasuaWorldSubsystem::SetGlobalWetness(float Wetness) {
    UMaterialParameterCollection* MPC = LoadObject<UMaterialParameterCollection>(
        nullptr, TEXT("/Game/Materials/MPC_AlsasuaGlobal.MPC_AlsasuaGlobal"));
    if (!MPC) return;

    UWorld* W = GetWorld();
    if (!W) return;

    UMaterialParameterCollectionInstance* Inst = W->GetParameterCollectionInstance(MPC);
    if (!Inst) return;

    float ClampedWetness = FMath::Clamp(Wetness, 0.f, 1.f);
    Inst->SetScalarParameterValue(FName("GlobalWetness"), ClampedWetness);
    Inst->SetScalarParameterValue(FName("RainIntensity"), ClampedWetness * 0.8f);
    Inst->SetScalarParameterValue(FName("PuddleOpacity"), FMath::Clamp(ClampedWetness * 1.2f, 0.f, 1.f));
}
