#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PursuitSubsystem.generated.h"

UCLASS()
class GF_VEHICULOS_API UPursuitSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category="AAA|Pursuit")
    int32 CurrentPatrolsInChase = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Pursuit")
    int32 MaxPatrols = 5;

    UFUNCTION(BlueprintCallable, Category="AAA|Pursuit")
    void RequestBackup(FVector Location, AActor* Target);

    UFUNCTION(BlueprintCallable, Category="AAA|Pursuit")
    void RegisterPatrolAction(bool bJoined);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Pursuit")
    TSubclassOf<AActor> PatrolVehicleClass;
};
