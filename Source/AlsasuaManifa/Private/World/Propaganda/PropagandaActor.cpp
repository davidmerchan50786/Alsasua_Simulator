#include "World/Propaganda/PropagandaActor.h"
#include "Components/StaticMeshComponent.h"
#include "Politics/FactionSubsystem.h"

APropagandaActor::APropagandaActor() {
    PrimaryActorTick.bCanEverTick = false;
    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    RootComponent = VisualMesh;
}
void APropagandaActor::BeginPlay() {
    Super::BeginPlay();
    VisualMesh->SetHiddenInGame(!bIsPlaced);
}
void APropagandaActor::PlacePropaganda() {
    if (bIsPlaced) return;
    bIsPlaced = true;
    VisualMesh->SetHiddenInGame(false);
    if (UWorld* W = GetWorld()) {
        if (UFactionSubsystem* FS = W->GetSubsystem<UFactionSubsystem>()) {
            FS->RecordPoliticalEvent(FName("LaAsamblea"), FName("ElCentro"), InfluenceGain);
        }
    }
}
void APropagandaActor::RemoveByAuthority() {
    bIsPlaced = false;
    VisualMesh->SetHiddenInGame(true);
}
