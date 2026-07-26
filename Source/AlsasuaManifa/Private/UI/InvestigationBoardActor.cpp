#include "UI/InvestigationBoardActor.h"
#include "Mision/SabotageMissionActor.h"
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
    UWorld* World = GetWorld();
    if (!World) return;

    // Verificar que el nodo existe y está descubierto.
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

    // Spawnear el ASabotageMissionActor cerca del tablero.
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    const FVector SpawnLoc = GetActorLocation() + GetActorForwardVector() * 200.f;

    ASabotageMissionActor* Mission = World->SpawnActor<ASabotageMissionActor>(
        ASabotageMissionActor::StaticClass(), SpawnLoc, FRotator::ZeroRotator, Params);

    if (Mission)
    {
        Mission->TargetNodeId = NodeId;
        Mission->StartMission();

        UE_LOG(LogTemp, Warning, TEXT("Iniciando Operacion de Sabotaje contra: %s"), *NodeId.ToString());
    }
}
