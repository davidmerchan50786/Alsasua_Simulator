#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AlsasuaCore.h"
#include "InterrogatorActor.generated.h"

UCLASS()
class ALSASUAMANIFA_API AInterrogatorActor : public AActor
{
    GENERATED_BODY()

public:
    AInterrogatorActor();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Interrogation")
    float DifficultyMultiplier = 1.f;

    UFUNCTION(BlueprintCallable, Category="AAA|Interrogation")
    void StartInterrogation(AActor* Target, float Duration = 30.f);

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    AActor* TargetActor;
};
