#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProtestManager.generated.h"

UCLASS()
class GF_SYSTEMS_API AProtestManager : public AActor
{
    GENERATED_BODY()

public:
    AProtestManager();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Protest")
    int32 BaseProtesterCount = 20;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Protest")
    int32 MaxProtesterCount = 200;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Protest")
    float ProtesterLifetimeSeconds = 120.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Protest")
    TSubclassOf<AActor> ProtesterClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Protest")
    TArray<FVector> SpawnPoints;

    UFUNCTION(BlueprintCallable, Category="AAA|Protest")
    void TriggerProtest(float Intensity);

    UFUNCTION(BlueprintCallable, Category="AAA|Protest")
    void StopProtest();

    UFUNCTION(BlueprintCallable, Category="AAA|Protest")
    void TriggerProtestByEvidence(FName EvidenceId);

    void SpawnProtesters(int32 Amount);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    UPROPERTY()
    TArray<AActor*> ActiveProtesters;
};
