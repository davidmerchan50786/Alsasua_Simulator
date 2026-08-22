#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Volume.h"
#include "UrbanSectorVolume.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSectorActorChanged, AUrbanSectorVolume*, Sector, AActor*, Actor);

UCLASS()
class GF_SYSTEMS_API AUrbanSectorVolume : public AVolume {
    GENERATED_BODY()
public:
    AUrbanSectorVolume();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Urban")
    FName SectorName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Urban")
    float PolicePresence = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Urban")
    float PopularSupport = 20.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Urban")
    bool bIsRoadBlocked = false;

    UPROPERTY(BlueprintAssignable, Category="AAA|Urban")
    FOnSectorActorChanged OnActorEnteredSector;

    UPROPERTY(BlueprintAssignable, Category="AAA|Urban")
    FOnSectorActorChanged OnActorLeftSector;

    UFUNCTION(BlueprintPure, Category="AAA|Urban")
    int32 GetActorCount() const { return ActorsInSector.Num(); }

    virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
    virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

private:
    UPROPERTY()
    TArray<TWeakObjectPtr<AActor>> ActorsInSector;
};