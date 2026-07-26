#include "Components/Camera/AdvancedCameraComponent.h"
#include "AI/AlsasuaCrowdSentiment.h"
#include "Kismet/GameplayStatics.h"

UAdvancedCameraComponent::UAdvancedCameraComponent() { PrimaryComponentTick.bCanEverTick = true; }

FEvidencePhoto UAdvancedCameraComponent::CapturePhoto() {
    FEvidencePhoto NewPhoto;
    NewPhoto.Timestamp = FDateTime::Now();
    NewPhoto.CaptureLocation = GetComponentLocation();
    NewPhoto.CaptureDirection = GetComponentRotation();

    UWorld* W = GetWorld();
    if (!W) return NewPhoto;

    // Generate subject name based on what's in front of the camera.
    FVector Forward = GetForwardVector();
    FHitResult Hit;
    FCollisionQueryParams Q;
    Q.AddIgnoredActor(GetOwner());
    if (W->LineTraceSingleByChannel(Hit, NewPhoto.CaptureLocation, NewPhoto.CaptureLocation + Forward * 5000.f, ECC_Visibility, Q))
    {
        NewPhoto.SubjectName = FText::FromString(FString::Printf(TEXT("Foto de %s"), *Hit.GetActor()->GetName()));
    }
    else
    {
        NewPhoto.SubjectName = FText::FromString(TEXT("Paisaje urbano"));
    }

    // ViralPotential scales with crowd tension and proximity to events.
    float BasePotential = 20.f;
    if (UAlsasuaCrowdSentiment* Sentiment = W->GetSubsystem<UAlsasuaCrowdSentiment>())
    {
        BasePotential += Sentiment->GlobalTension * 60.f;
    }
    NewPhoto.ViralPotential = FMath::Clamp(BasePotential, 5.f, 100.f);

    OnPhotoCaptured.Broadcast(NewPhoto);
    return NewPhoto;
}
