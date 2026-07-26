#pragma once
#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "AdvancedCameraComponent.generated.h"

USTRUCT(BlueprintType)
struct FEvidencePhoto {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FText SubjectName;

    UPROPERTY(BlueprintReadOnly)
    float ViralPotential = 0.f;

    UPROPERTY(BlueprintReadOnly)
    FDateTime Timestamp;

    UPROPERTY(BlueprintReadOnly)
    FVector CaptureLocation;

    UPROPERTY(BlueprintReadOnly)
    FRotator CaptureDirection;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UAdvancedCameraComponent : public UCameraComponent {
    GENERATED_BODY()
public:
    UAdvancedCameraComponent();

    UFUNCTION(BlueprintCallable, Category="AAA|Media")
    FEvidencePhoto CapturePhoto();

    UPROPERTY(EditAnywhere, Category="AAA|Media")
    float ZoomSpeed = 10.f;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhotoCaptured, const FEvidencePhoto&, Photo);

    UPROPERTY(BlueprintAssignable, Category="AAA|Media")
    FOnPhotoCaptured OnPhotoCaptured;
};