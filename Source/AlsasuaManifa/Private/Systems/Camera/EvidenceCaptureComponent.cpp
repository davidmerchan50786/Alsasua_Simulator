#include "Systems/Camera/EvidenceCaptureComponent.h"
#include "Systems/Social/SocialMediaSubsystem.h"
#include "Kismet/KismetMathLibrary.h"

UEvidenceCaptureComponent::UEvidenceCaptureComponent() {}

float UEvidenceCaptureComponent::CalculateStability(float StaminaRatio) {
    return FMath::Clamp(StaminaRatio * 100.f, 10.f, 100.f);
}

FCameraResultV2 UEvidenceCaptureComponent::CaptureEvidenceV2(float CurrentStamina, float MaxStamina, float CurrentZoom) {
    FCameraResultV2 Result;

    UWorld* W = GetWorld();
    if (!W) return Result;

    float StaminaRatio = (MaxStamina > 0.f) ? CurrentStamina / MaxStamina : 1.f;
    Result.StabilityScore = CalculateStability(StaminaRatio);

    float ZoomBonus = FMath::Clamp(CurrentZoom / 2.f, 1.0f, 3.0f);

    float RawImpact = 20.f * ZoomBonus * (Result.StabilityScore / 100.f);

    if (USocialMediaSubsystem* SocialSS = W->GetSubsystem<USocialMediaSubsystem>()) {
        FEvidencePost Post;
        Post.Description = "Acci�n grabada en directo en Altsasu";
        Post.ImpactValue = RawImpact;
        Post.RiskValue = ZoomBonus * 5.f;

        SocialSS->UploadEvidence(Post);
        Result.FinalImpact = RawImpact;
    }

    Result.Description = (Result.StabilityScore < 40.f) ? "Imagen movida (Baja Stamina)" : "Toma estable y clara";

    return Result;
}
