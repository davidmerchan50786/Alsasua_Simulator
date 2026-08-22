#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Social/EvidenceData.h"
#include "EvidenceSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEvidencePublished, FName, EvidenceId);

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

    UPROPERTY(BlueprintReadOnly, Category="AAA|Social")
    TArray<FEvidenceItem> CollectedEvidence;
};
