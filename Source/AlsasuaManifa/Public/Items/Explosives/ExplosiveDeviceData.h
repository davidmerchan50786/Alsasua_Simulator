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
    //
    // Niagara, no Cascade. Eran UParticleSystem*, y el proyecto no genera un solo
    // asset de Cascade: los NS_* los fabrica Tools/create_niagara_vfx.py y son
    // los que usa todo lo demás. Como además no hay ningún DataAsset de
    // explosivo en Content/, estos dos campos salían siempre nulos y el molotov
    // no enseñaba ni fuego ni explosión. Si se dejan vacíos, AIncendiaryCharge
    // cae a NS_Molotov y NS_Explosion, que sí existen.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Explosive|Effects")
    TObjectPtr<class UNiagaraSystem> ExplosionFX = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Explosive|Effects")
    TObjectPtr<class UNiagaraSystem> FirePrefab = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Explosive|Effects")
    USoundBase* ExplosionSound;

    // Cinematic
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Explosive|Cinematic")
    bool bEnableCinematicSlowMo = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Explosive|Safety")
    float MaxImpulseClamp = 500000.f; // safeguard to avoid physics explosion abuse
};