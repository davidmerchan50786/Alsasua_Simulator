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

    if(FMath::FRand() < SuccessChance) {
        CurrentResistance -= 20.f;
    } else {
        CurrentResistance += 10.f; // Se cierra en banda
    }

    if(CurrentResistance <= 0) {
        // Encontrar un InvestigationBoardActor para descubrir un nodo
        TArray<AActor*> Boards;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AInvestigationBoardActor::StaticClass(), Boards);
        if(Boards.Num() > 0) {
            AInvestigationBoardActor* Board = Cast<AInvestigationBoardActor>(Boards[0]);
            // Por ahora desbloqueamos uno de prueba
            FName NewNode = FName("WH_01");
            Board->DescubrirNodo(NewNode);
            OnInterrogationSuccess.Broadcast(NewNode);
        }
    }
}