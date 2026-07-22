#include "SpatialGrid.h"
#include "EngineUtils.h"

TArray<AActor*> USpatialGrid::GetActorsInRadius(FVector Center, float Radius) const
{
    TArray<AActor*> Result;
    if (UWorld* World = GetWorld())
    {
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            AActor* Actor = *It;
            if (Actor && FVector::DistSquared(Center, Actor->GetActorLocation()) <= Radius * Radius)
            {
                Result.Add(Actor);
            }
        }
    }
    return Result;
}
