#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PoliticalAgent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHeadlinePublished, const FText&, Headline, const FText&, Body);

UCLASS()
class GF_POLITICA_API APoliticalAgent : public AActor {
    GENERATED_BODY()
public:
    APoliticalAgent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|DeepState")
    FText AgentName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|DeepState")
    float InfluencePower = 10.f;

    UFUNCTION(BlueprintCallable, Category="AAA|DeepState")
    void PublishHeadline(const FText& Headline, const FText& Body);

    UPROPERTY(BlueprintAssignable, Category="AAA|DeepState")
    FOnHeadlinePublished OnHeadlinePublished;

protected:
    virtual void BeginPlay() override;
};
