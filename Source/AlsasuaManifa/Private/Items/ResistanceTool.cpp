#include "Items/ResistanceTool.h"
#include "Kismet/GameplayStatics.h"

void AResistanceTool::UseTool(FVector TargetLocation)
{
    switch (ToolType)
    {
    case EToolType::Slingshot:
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), nullptr, TargetLocation);
        break;
    case EToolType::SmokeBomb:
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), nullptr, TargetLocation);
        break;
    case EToolType::Megaphone:
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), nullptr, TargetLocation);
        break;
    }
}
