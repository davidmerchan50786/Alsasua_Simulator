#pragma once
#include "CoreMinimal.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "AlsasuaCrowdHISMManager.generated.h"

UCLASS()
class ALSASUAMANIFA_API AAlsasuaCrowdHISMManager : public AActor
{
	GENERATED_BODY()
public:
	AAlsasuaCrowdHISMManager();

	UPROPERTY(VisibleAnywhere)
	UHierarchicalInstancedStaticMeshComponent* HISMComponent;

	void UpdateInstance(int32 Index, FVector NewLocation);
};
