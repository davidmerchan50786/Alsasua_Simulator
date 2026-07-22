#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NarcoSyndicateComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNodeSabotaged, FName, NodeId);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALSASUAMANIFA_API UNarcoSyndicateComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UNarcoSyndicateComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Narco")
    FName NodeId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Narco")
    float Value = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Narco")
    float Alertness = 0.f;

    UFUNCTION(BlueprintCallable, Category="Narco")
    void SabotageNode(float Power);

    UFUNCTION(BlueprintCallable, Category="Narco")
    void InterceptShipment(float Effectiveness);

    UPROPERTY(BlueprintAssignable)
    FOnNodeSabotaged OnNodeSabotaged;
};
