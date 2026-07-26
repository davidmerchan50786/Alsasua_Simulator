#include "AlsasuaEditorTool.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GameFramework/Actor.h"

void AAlsasuaEditorTool::DebugBuildings()
{
	UWorld* World = GetWorld();
	if (!World) return;

	int32 TotalBuildings = 0;
	int32 TotalInstances = 0;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor) continue;

		TArray<UInstancedStaticMeshComponent*> ISMs;
		Actor->GetComponents<UInstancedStaticMeshComponent>(ISMs);

		if (ISMs.Num() == 0) continue;

		for (UInstancedStaticMeshComponent* ISM : ISMs)
		{
			if (!ISM) continue;
			const int32 Count = ISM->GetInstanceCount();
			if (Count == 0) continue;

			TotalInstances += Count;
			TotalBuildings++;

			UE_LOG(LogTemp, Log, TEXT("[EditorTool] %s — %d instancias en %s"),
				*Actor->GetActorLabel(), Count, *ISM->GetStaticMesh()->GetName());
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[EditorTool] DebugBuildings: %d actores con ISM, %d instancias totales"),
		TotalBuildings, TotalInstances);
}
