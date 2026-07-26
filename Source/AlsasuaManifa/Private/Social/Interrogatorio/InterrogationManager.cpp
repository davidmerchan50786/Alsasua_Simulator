#include "Social/Interrogatorio/InterrogationManager.h"
#include "UI/InvestigationBoardActor.h"
#include "Kismet/GameplayStatics.h"

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
        TArray<AActor*> Boards;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AInvestigationBoardActor::StaticClass(), Boards);
        if(Boards.Num() > 0) {
            AInvestigationBoardActor* Board = Cast<AInvestigationBoardActor>(Boards[0]);

            // Discover the first undiscovered, unsabotaged node instead of hardcoding.
            FName NewNode = NAME_None;
            for (const FNodoInvestigacion& Nodo : Board->TodosLosNodos) {
                if (!Nodo.bDescubierto && !Nodo.bSaboteado) {
                    NewNode = Nodo.NodeId;
                    break;
                }
            }

            if (NewNode != NAME_None) {
                Board->DescubrirNodo(NewNode);
                OnInterrogationSuccess.Broadcast(NewNode);
            }
        }
    }
}