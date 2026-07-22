#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DeepStateFactionComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UDeepStateFactionComponent : public UActorComponent {
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Faction")
    float InfluenceLevel = 50.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Faction")
    bool bIsUnderground = true;
};