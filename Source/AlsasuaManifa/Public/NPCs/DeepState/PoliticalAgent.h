#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PoliticalAgent.generated.h"

UCLASS()
class ALSASUAMANIFA_API APoliticalAgent : public AActor {
    GENERATED_BODY()
public:
    APoliticalAgent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|DeepState")
    FText AgentName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|DeepState")
    float InfluencePower = 10.f;

    UFUNCTION(BlueprintCallable, Category="AAA|DeepState")
    void PublishHeadline(const FText& Headline, const FText& Body);

protected:
    virtual void BeginPlay() override;
};