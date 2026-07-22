#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "EvidenceCaptureComponent.generated.h"

USTRUCT(BlueprintType)
struct FCameraResultV2 {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float FinalImpact = 0.f;

    UPROPERTY(BlueprintReadOnly)
    float StabilityScore = 0.f;

    UPROPERTY(BlueprintReadOnly)
    FString Description;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UEvidenceCaptureComponent : public USceneComponent {
    GENERATED_BODY()
public:
    UEvidenceCaptureComponent();

    UFUNCTION(BlueprintCallable, Category="AAA|Camera")
    FCameraResultV2 CaptureEvidenceV2(float CurrentStamina, float MaxStamina, float CurrentZoom);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Camera")
    float BaseShakeIntensity = 10.f;

private:
    float CalculateStability(float StaminaRatio);
};
