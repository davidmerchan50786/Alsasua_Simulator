#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SocialStealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStealthStateChanged, bool, bIsHiddenInCrowd);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALSASUAMANIFA_API USocialStealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USocialStealthComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Stealth")
    float CrowdDetectionRadius = 300.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Stealth")
    int32 MinCrowdSizeToHide = 3;

    UPROPERTY(BlueprintReadOnly, Category="AAA|Stealth")
    bool bIsHiddenInCrowd = false;

    UPROPERTY(BlueprintAssignable, Category="AAA|Stealth")
    FOnStealthStateChanged OnStealthStateChanged;

    UFUNCTION(BlueprintCallable, Category="AAA|Stealth")
    void UpdateStealthStatus();

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
