#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ActorPoolComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UActorPoolComponent : public UActorComponent {
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Pooling") TSubclassOf<AActor> ActorClass;
    UPROPERTY(EditAnywhere, Category="Pooling") int32 PoolSize = 20;

    UFUNCTION(BlueprintCallable) AActor* GetFromPool(FVector Location, FRotator Rotation);
    UFUNCTION(BlueprintCallable) void ReturnToPool(AActor* Actor);

protected:
    virtual void BeginPlay() override;
private:
    UPROPERTY() TArray<AActor*> InactivePool;
};