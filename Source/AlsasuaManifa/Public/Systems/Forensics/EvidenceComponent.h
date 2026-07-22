#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EvidenceComponent.generated.h"

UENUM(BlueprintType)
enum class EForensicState : uint8 {
    Pristine,
    Contaminated,
    Collected
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEvidenceCollected, AActor*, SourceActor, FName, EvidenceTag);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UEvidenceComponent : public UActorComponent {
    GENERATED_BODY()
public:
    UEvidenceComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Forensics")
    FName EvidenceTag = "GenericEvidence";

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Forensics")
    float TimeToDecay = 600.f; // segundos antes de que la evidencia empiece a degradarse

    UPROPERTY(BlueprintAssignable)
    FOnEvidenceCollected OnEvidenceCollected;

    UFUNCTION(BlueprintCallable, Category="Forensics")
    void CollectEvidence(AActor* Collector);

    UFUNCTION(BlueprintCallable, Category="Forensics")
    void ContaminateEvidence(float Amount);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Forensics")
    EForensicState GetForensicState() const { return ForensicState; }

protected:
    virtual void BeginPlay() override;

private:
    EForensicState ForensicState = EForensicState::Pristine;
    float SpawnTime = 0.f;

    void UpdateDecay();
    FTimerHandle TimerHandle_Decay;
};