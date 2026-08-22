#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PropagandaActor.generated.h"

UCLASS()
class GF_SYSTEMS_API APropagandaActor : public AActor {
    GENERATED_BODY()
public:
    APropagandaActor();
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIsPlaced = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float InfluenceGain = 2.0f;
    UFUNCTION(BlueprintCallable) void PlacePropaganda();
    UFUNCTION(BlueprintCallable) void RemoveByAuthority();
protected:
    virtual void BeginPlay() override;
private:
    UPROPERTY(VisibleAnywhere) class UStaticMeshComponent* VisualMesh;
};