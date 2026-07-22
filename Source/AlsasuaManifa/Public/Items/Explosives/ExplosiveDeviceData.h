#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ExplosiveDeviceData.generated.h"

UENUM(BlueprintType)
enum class EExplosiveFuseType : uint8 {
    Timed,
    Remote
};

UCLASS(BlueprintType)
class ALSASUAMANIFA_API UExplosiveDeviceData : public UPrimaryDataAsset {
    GENERATED_BODY()

public:
    // Gameplay
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Explosive|Gameplay")
    float ChargePower = 1000.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Explosive|Gameplay")
    float BlastRadius = 600.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Explosive|Gameplay")
    float ImpulseStrength = 200000.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Explosive|Gameplay")
    float IgniteDuration = 30.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Explosive|Gameplay")
    EExplosiveFuseType FuseType = EExplosiveFuseType::Timed;

    // VFX / SFX
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Explosive|Effects")
    UParticleSystem* ExplosionFX;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Explosive|Effects")
    UParticleSystem* FirePrefab;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Explosive|Effects")
    USoundBase* ExplosionSound;

    // Cinematic
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Explosive|Cinematic")
    bool bEnableCinematicSlowMo = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Explosive|Safety")
    float MaxImpulseClamp = 500000.f; // safeguard to avoid physics explosion abuse
};