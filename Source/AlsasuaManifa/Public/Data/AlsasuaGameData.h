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

    // Niagara, no Cascade: era el último UParticleSystem* del proyecto. No lo
    // lee nadie todavía, y precisamente por eso conviene que el tipo sea el
    // bueno — un campo suelto de Cascade es la semilla del mismo fallo que dejó
    // sin efecto a los vehículos, a la bengala antidisturbios y al molotov.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
    TObjectPtr<class UNiagaraSystem> PoliceFlashFX = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SFX")
    USoundBase* MegaphoneUseSFX;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SFX")
    USoundBase* PoliceSirenSFX;
};