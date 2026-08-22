#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Delegates/Delegate.h"
#include "SocialMediaSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnViralPost, FText, PostContent);

USTRUCT(BlueprintType)
struct FEvidencePost {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ImpactValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RiskValue = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 GlobalFollowers = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ViralPotential = 1.0f;
};

UCLASS()
class GF_SOCIAL_API USocialMediaSubsystem : public UWorldSubsystem {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="AAA|Media")
    void PostToFeed(FEvidencePost Photo);

    UFUNCTION(BlueprintCallable, Category="AAA|Media")
    void AddFollowers(int32 Amount);

    UFUNCTION(BlueprintCallable, Category="AAA|Media")
    void UploadEvidence(const FEvidencePost& Post);

    UPROPERTY(BlueprintReadOnly, Category="AAA|Media")
    int32 TotalFollowers = 1500;

    UPROPERTY(BlueprintReadOnly, Category="AAA|Media")
    int32 GlobalFollowers = 0;

    UPROPERTY(BlueprintReadOnly, Category="AAA|Media")
    float ViralMultiplier = 1.0f;

    UPROPERTY(BlueprintAssignable)
    FOnViralPost OnViralPost;
};