#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InterrogatorActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnInterrogationStarted, AActor*, Target, float, Duration, float, Difficulty);

UCLASS()
class GF_SOCIAL_API AInterrogatorActor : public AActor
{
    GENERATED_BODY()

public:
    AInterrogatorActor();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Interrogation")
    float DifficultyMultiplier = 1.f;

    UFUNCTION(BlueprintCallable, Category="AAA|Interrogation")
    void StartInterrogation(AActor* Target, float Duration = 30.f);

    UPROPERTY(BlueprintAssignable, Category="AAA|Interrogation")
    FOnInterrogationStarted OnInterrogationStarted;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    AActor* TargetActor;
};
