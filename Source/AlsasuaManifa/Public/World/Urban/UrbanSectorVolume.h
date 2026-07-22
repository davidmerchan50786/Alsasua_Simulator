#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Volume.h"
#include "UrbanSectorVolume.generated.h"

UCLASS()
class ALSASUAMANIFA_API AUrbanSectorVolume : public AVolume {
    GENERATED_BODY()
public:
    AUrbanSectorVolume();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Urban")
    FName SectorName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Urban")
    float PolicePresence = 0.f;  // 0..100

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Urban")
    float PopularSupport = 20.f; // 0..100

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Urban")
    bool bIsRoadBlocked = false;
};