#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Social/EvidenceData.h"
#include "EvidenceSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEvidencePublished, FName, EvidenceId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEvidenceThresholdReached, int32, ThresholdLevel);

UCLASS()
class GF_SOCIAL_API UEvidenceSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="AAA|Social")
    void CollectEvidence(FEvidenceItem NewEvidence);

    UFUNCTION(BlueprintCallable, Category="AAA|Social")
    void PublishToPress(FName EvidenceId);

    UPROPERTY(BlueprintAssignable, Category="AAA|Social")
    FOnEvidencePublished OnEvidencePublished;

    /** Fires when evidence count crosses a threshold (3, 6, 10, 15). */
    UPROPERTY(BlueprintAssignable, Category="AAA|Social")
    FOnEvidenceThresholdReached OnEvidenceThresholdReached;

    UPROPERTY(BlueprintReadOnly, Category="AAA|Social")
    TArray<FEvidenceItem> CollectedEvidence;

    /** Total evidence ever collected (for thresholds). */
    UPROPERTY(BlueprintReadOnly, Category="AAA|Social")
    int32 TotalEvidenceCollected = 0;

    /** Evidence thresholds that trigger automatic events. */
    UPROPERTY(EditAnywhere, Category="AAA|Social|Thresholds")
    TArray<int32> EvidenceThresholds = {3, 6, 10, 15};

protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
    UFUNCTION()
    void OnCriminalActivity(FName ActivityType, int32 Severity);

    void CheckEvidenceThresholds();
};
