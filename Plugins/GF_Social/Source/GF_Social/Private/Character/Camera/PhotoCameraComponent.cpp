#include "Character/Camera/PhotoCameraComponent.h"
#include "Social/EvidenceSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

UPhotoCameraComponent::UPhotoCameraComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UPhotoCameraComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UPhotoCameraComponent::AdjustZoom(float AxisValue)
{
    CurrentZoom = FMath::Clamp(CurrentZoom + AxisValue * 0.1f, 1.0f, MaxZoom);
}

void UPhotoCameraComponent::TakePhoto()
{
    UWorld* W = GetWorld();
    if (!W) return;

    APlayerController* PC = UGameplayStatics::GetPlayerController(W, 0);
    if (!PC) return;

    FVector CameraLocation;
    FRotator CameraRotation;
    PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

    FHitResult Hit;
    FVector End = CameraLocation + (CameraRotation.Vector() * 5000.f);
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(GetOwner());

    if (W->LineTraceSingleByChannel(Hit, CameraLocation, End, ECC_Visibility, Params))
    {
        AActor* HitActor = Hit.GetActor();
        FEvidenceItem Evidence;
        if (IsTargetValidEvidence(HitActor, Evidence))
        {
            if (UEvidenceSubsystem* ES = W->GetSubsystem<UEvidenceSubsystem>())
            {
                ES->CollectEvidence(Evidence);
                OnPhotoTaken.Broadcast(Evidence);
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Foto realizada. Zoom: %f"), CurrentZoom);
}

bool UPhotoCameraComponent::IsTargetValidEvidence(AActor* TargetActor, FEvidenceItem& OutEvidence)
{
    if (!TargetActor) return false;

    // Buscamos si el actor tiene Tags de evidencia (ej: "Evidence_Drugs", "Evidence_GCOp")
    for (FName Tag : TargetActor->Tags)
    {
        if (Tag.ToString().StartsWith("Evidence_"))
        {
            OutEvidence.EvidenceId = Tag;
            OutEvidence.Title = "Fotografia de " + Tag.ToString().RightChop(9);
            OutEvidence.ImpactPower = 15.0f;
            OutEvidence.Reliability = 1.0f;
            return true;
        }
    }
    return false;
}
