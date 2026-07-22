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
};