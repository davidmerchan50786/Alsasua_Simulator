#include "Social/Interrogatorio/InterrogationManager.h"

void UInterrogationManager::StartInterrogation(FName TargetNPCId, float NPCResistance) {
    CurrentTarget = TargetNPCId;
    CurrentResistance = NPCResistance;
    UE_LOG(LogTemp, Log, TEXT("Interrogando a %s. Resistencia: %f"), *TargetNPCId.ToString(), CurrentResistance);
}

void UInterrogationManager::ApplyTactic(EInterrogationTactic Tactic) {
    float SuccessChance = 0.5f;
    if(Tactic == EInterrogationTactic::Intimidation) SuccessChance = 0.7f;
    if(Tactic == EInterrogationTactic::Corruption) SuccessChance = 0.6f;

    if(FMath::FRand() < SuccessChance) {
        CurrentResistance -= 20.f;
    } else {
        CurrentResistance += 10.f;
    }

    if(CurrentResistance <= 0) {
        OnInterrogationSuccess.Broadcast(CurrentTarget);
    }
}
