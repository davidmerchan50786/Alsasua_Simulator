#include "UI/InvestigationBoardActor.h"
#include "Engine/World.h"

AInvestigationBoardActor::AInvestigationBoardActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AInvestigationBoardActor::BeginPlay()
{
    Super::BeginPlay();
    if (TodosLosNodos.Num() == 0) {
        TodosLosNodos.Add({FName("WH_01"), "Almacen Calle Mayor", "Centro logistico del Gremio.", false, false});
        TodosLosNodos.Add({FName("RT_01"), "Ruta del Puerto", "Transporte de mercancia especial.", false, false});
    }
}

void AInvestigationBoardActor::DescubrirNodo(FName NodeId)
{
    for (auto& Nodo : TodosLosNodos) {
        if (Nodo.NodeId == NodeId) {
            Nodo.bDescubierto = true;
            break;
        }
    }
}

void AInvestigationBoardActor::MarcarNodoComoSaboteado(FName NodeId)
{
    for (auto& Nodo : TodosLosNodos) {
        if (Nodo.NodeId == NodeId) {
            Nodo.bSaboteado = true;
            break;
        }
    }
}

void AInvestigationBoardActor::LanzarMisionDeSabotaje(FName NodeId)
{
    bool bNodeValid = false;
    for (const auto& Nodo : TodosLosNodos) {
        if (Nodo.NodeId == NodeId && Nodo.bDescubierto && !Nodo.bSaboteado) {
            bNodeValid = true;
            break;
        }
    }

    if (!bNodeValid)
    {
        UE_LOG(LogTemp, Warning, TEXT("LanzarMisionDeSabotaje: Nodo %s no válido o ya saboteado."), *NodeId.ToString());
        return;
    }

    OnSabotageRequested.Broadcast(NodeId);
}
