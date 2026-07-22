#include "Components/Camera/AdvancedCameraComponent.h"
#include "Kismet/KismetSystemLibrary.h"

UAdvancedCameraComponent::UAdvancedCameraComponent() { PrimaryComponentTick.bCanEverTick = true; }

FEvidencePhoto UAdvancedCameraComponent::CapturePhoto() {
    FEvidencePhoto NewPhoto;
    NewPhoto.SubjectName = FText::FromString("Disturbio en Altsasu");
    NewPhoto.Timestamp = FDateTime::Now();
    NewPhoto.ViralPotential = FMath::RandRange(10.f, 100.f);
    return NewPhoto;
}
