#include "AI/AlsasuaTacticManager.h"

void UAlsasuaTacticManager::Tick(float DeltaTime)
{
    // Lógica de decisión simplificada para el tick
}

void UAlsasuaTacticManager::SetGlobalTactic(EAlsasuaTactic NewTactic)
{
    if(CurrentTactic == NewTactic) return;

    CurrentTactic = NewTactic;
    OnTacticChanged.Broadcast(CurrentTactic);

    UE_LOG(LogAlsasuaAI, Warning, TEXT("Táctica Global Cambiada a: %d"), (int32)NewTactic);
}
