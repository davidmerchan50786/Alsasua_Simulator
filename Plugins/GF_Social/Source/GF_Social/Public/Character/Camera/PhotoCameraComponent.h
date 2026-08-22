#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Social/EvidenceData.h"
#include "PhotoCameraComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhotoTaken, FEvidenceItem, Evidence);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GF_SOCIAL_API UPhotoCameraComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPhotoCameraComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Camera")
    float MaxZoom = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Camera")
    float CurrentZoom = 1.0f;

    UPROPERTY(BlueprintAssignable, Category="AAA|Camera")
    FOnPhotoTaken OnPhotoTaken;

    UFUNCTION(BlueprintCallable, Category="AAA|Camera")
    void TakePhoto();

    UFUNCTION(BlueprintCallable, Category="AAA|Camera")
    void AdjustZoom(float AxisValue);

    UFUNCTION(BlueprintCallable, Category="AAA|Camera")
    bool IsTargetValidEvidence(AActor* TargetActor, FEvidenceItem& OutEvidence);

protected:
    virtual void BeginPlay() override;
};
