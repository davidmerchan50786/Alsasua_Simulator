#include "Visuals/AlsasuaAnimationBlendSystem.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

UAlsasuaAnimationBlendSystem::UAlsasuaAnimationBlendSystem()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UAlsasuaAnimationBlendSystem::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

FFootIKResult UAlsasuaAnimationBlendSystem::CalculateFootIK(float DeltaTime)
{
    FFootIKResult Result;
    AActor* Owner = GetOwner();
    if (!Owner) return Result;

    UWorld* World = Owner->GetWorld();
    if (!World) return Result;

    FVector ActorLoc = Owner->GetActorLocation();
    FVector Forward = Owner->GetActorForwardVector();
    FVector Right = Owner->GetActorRightVector();

    // Trace for left foot.
    {
        FVector FootOffset = FVector(0.f, -30.f, 0.f);
        FVector Start = ActorLoc + FootOffset + FVector(0, 0, 50.f);
        FVector End = Start - FVector(0, 0, FootTraceDistance * 2.f);

        FHitResult Hit;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(Owner);

        if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
        {
            float TargetOffset = (Hit.ImpactPoint.Z - ActorLoc.Z);
            Result.LeftFootOffset = FMath::FInterpTo(SmoothedLeftOffset, TargetOffset, DeltaTime, FootIKInterpSpeed);
            SmoothedLeftOffset = Result.LeftFootOffset;

            FRotator TargetRot = FRotationMatrix::MakeFromZ(Hit.ImpactNormal).Rotator();
            Result.LeftFootRotation = FMath::RInterpTo(SmoothedLeftRotation, TargetRot, DeltaTime, FootIKInterpSpeed);
            SmoothedLeftRotation = Result.LeftFootRotation;
        }
    }

    // Trace for right foot.
    {
        FVector FootOffset = FVector(0, 30.f, 0);
        FVector Start = ActorLoc + FootOffset + FVector(0, 0, 50.f);
        FVector End = Start - FVector(0, 0, FootTraceDistance * 2.f);

        FHitResult Hit;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(Owner);

        if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
        {
            float TargetOffset = (Hit.ImpactPoint.Z - ActorLoc.Z);
            Result.RightFootOffset = FMath::FInterpTo(SmoothedRightOffset, TargetOffset, DeltaTime, FootIKInterpSpeed);
            SmoothedRightOffset = Result.RightFootOffset;

            FRotator TargetRot = FRotationMatrix::MakeFromZ(Hit.ImpactNormal).Rotator();
            Result.RightFootRotation = FMath::RInterpTo(SmoothedRightRotation, TargetRot, DeltaTime, FootIKInterpSpeed);
            SmoothedRightRotation = Result.RightFootRotation;
        }
    }

    return Result;
}

FRotator UAlsasuaAnimationBlendSystem::CalculateLookAtRotation(FVector LookAtTarget)
{
    AActor* Owner = GetOwner();
    if (!Owner) return FRotator::ZeroRotator;

    FVector Dir = (LookAtTarget - Owner->GetActorLocation()).GetSafeNormal2D();
    FRotator LookAtRot = Dir.Rotation();

    // Clamp.
    LookAtRot.Yaw = FMath::Clamp(LookAtRot.Yaw, -LookAtClampYaw, LookAtClampYaw);
    LookAtRot.Pitch = FMath::Clamp(LookAtRot.Pitch, -LookAtClampPitch, LookAtClampPitch);

    return LookAtRot;
}

float UAlsasuaAnimationBlendSystem::GetAimOffsetAlpha(float AimSpeed) const
{
    // Smooth blend: faster aim → higher alpha.
    return FMath::Clamp(AimSpeed / 500.f, 0.f, 1.f);
}

float UAlsasuaAnimationBlendSystem::GetLeanAmount(float Speed, float TurnRate) const
{
    // Lean based on turning speed.
    float LeanTarget = FMath::Clamp(TurnRate / 200.f, -1.f, 1.f);
    // Reduce lean at very low speeds.
    if (Speed < 50.f) LeanTarget *= 0.2f;
    return LeanTarget;
}
