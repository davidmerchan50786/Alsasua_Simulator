#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AlsasuaGameData.generated.h"

UCLASS(BlueprintType)
class ALSASUAMANIFA_API UAlsasuaGameData : public UPrimaryDataAsset {
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Balance|Locomotion")
    float BaseWalkSpeed = 400.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Balance|Locomotion")
    float SprintSpeedMultiplier = 1.8f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Balance|Social")
    float MegaphoneBaseInfluence = 1500.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Balance|Social")
    float EvidenceQualityMultiplier = 1.25f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
    UParticleSystem* PoliceFlashFX;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SFX")
    USoundBase* MegaphoneUseSFX;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SFX")
    USoundBase* PoliceSirenSFX;
};