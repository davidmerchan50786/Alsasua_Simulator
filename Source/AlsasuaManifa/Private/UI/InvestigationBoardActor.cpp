#include "UI/InvestigationBoardActor.h"
#include "Mision/SabotageMissionActor.h"

AInvestigationBoardActor::AInvestigationBoardActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AInvestigationBoardActor::BeginPlay()
{
    Super::BeginPlay();
    // Inicializar nodos de prueba si la lista está vacía
    if(TodosLosNodos.Num() == 0) {
        TodosLosNodos.Add({FName("WH_01"), "Almacen Calle Mayor", "Centro logistico del Gremio.", false, false});
        TodosLosNodos.Add({FName("RT_01"), "Ruta del Puerto", "Transporte de mercancia especial.", false, false});
    }
}

void AInvestigationBoardActor::DescubrirNodo(FName NodeId)
{
    for(auto& Nodo : TodosLosNodos) {
        if(Nodo.NodeId == NodeId) {
            Nodo.bDescubierto = true;
            break;
        }
    }
}

void AInvestigationBoardActor::MarcarNodoComoSaboteado(FName NodeId)
{
    for(auto& Nodo : TodosLosNodos) {
        if(Nodo.NodeId == NodeId) {
            Nodo.bSaboteado = true;
            break;
        }
    }
}

void AInvestigationBoardActor::LanzarMisionDeSabotaje(FName NodeId)
{
    // Lógica para spawnear el ASabotageMissionActor configurado para este NodeId
    UE_LOG(LogTemp, Warning, TEXT("Iniciando Operacion de Sabotaje contra: %s"), *NodeId.ToString());
}
