#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaAnimationBlendSystem.generated.h"

USTRUCT(BlueprintType)
struct FFootIKResult {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) float LeftFootOffset = 0.f;
    UPROPERTY(BlueprintReadOnly) float RightFootOffset = 0.f;
    UPROPERTY(BlueprintReadOnly) FRotator LeftFootRotation = FRotator::ZeroRotator;
    UPROPERTY(BlueprintReadOnly) FRotator RightFootRotation = FRotator::ZeroRotator;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GF_SYSTEMS_API UAlsasuaAnimationBlendSystem : public UActorComponent
{
    GENERATED_BODY()
public:
    UAlsasuaAnimationBlendSystem();

    // ── Foot IK ────────────────────────────────────────────────────────
    UFUNCTION(BlueprintCallable, Category="AAA|Animation|FootIK")
    FFootIKResult CalculateFootIK(float DeltaTime);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Animation|FootIK")
    float FootTraceDistance = 50.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Animation|FootIK")
    float FootIKInterpSpeed = 15.f;

    // ── Look At ────────────────────────────────────────────────────────
    UFUNCTION(BlueprintCallable, Category="AAA|Animation|LookAt")
    FRotator CalculateLookAtRotation(FVector LookAtTarget);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Animation|LookAt")
    float LookAtClampYaw = 90.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Animation|LookAt")
    float LookAtClampPitch = 60.f;

    // ── Blend Helpers ──────────────────────────────────────────────────
    UFUNCTION(BlueprintPure, Category="AAA|Animation|Blend")
    float GetAimOffsetAlpha(float AimSpeed) const;

    UFUNCTION(BlueprintPure, Category="AAA|Animation|Blend")
    float GetLeanAmount(float Speed, float TurnRate) const;

    /** Last computed FootIK result — read from Animation Blueprint. */
    UPROPERTY(BlueprintReadOnly, Category="AAA|Animation|FootIK")
    FFootIKResult LastFootIK;

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    float SmoothedLeftOffset = 0.f;
    float SmoothedRightOffset = 0.f;
    FRotator SmoothedLeftRotation = FRotator::ZeroRotator;
    FRotator SmoothedRightRotation = FRotator::ZeroRotator;
};
