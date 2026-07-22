#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MegaphoneTool.generated.h"

UCLASS()
class ALSASUAMANIFA_API AMegaphoneTool : public AActor {
    GENERATED_BODY()

public:
    AMegaphoneTool();

    // Activa el megáfono consumiendo Apoyo Popular o Energía
    UFUNCTION(BlueprintCallable, Category="AAA|Tools")
    bool UseMegaphone(float Intensity);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Tools")
    float InfluenceRadius = 2000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Tools")
    float CooldownTime = 5.f;

    UPROPERTY(BlueprintReadOnly, Category="AAA|Tools")
    bool bIsOnCooldown = false;

private:
    void ResetCooldown();
    FTimerHandle CooldownTimerHandle;

    // Afecta a la moral de la policía y NPCs
    void ApplyAudioInfluence(float Intensity);
};