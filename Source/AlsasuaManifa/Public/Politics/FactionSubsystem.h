#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "FactionSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FFactionData {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Influence = 50.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Suspicion = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<FName,float> Relations;
};

UCLASS()
class ALSASUAMANIFA_API UFactionSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category="Alsasua|Factions")
    void RegisterFaction(const FFactionData& Data);

    UFUNCTION(BlueprintCallable, Category="Alsasua|Factions")
    void RecordPoliticalEvent(FName SubjectFaction, FName TargetFaction, float Impact);

    UFUNCTION(BlueprintCallable, Category="Alsasua|Factions")
    void PublishEvidence(FName TargetFaction, float Strength);

    UFUNCTION(BlueprintCallable, Category="Alsasua|Factions")
    FFactionData GetFactionData(FName Id) const;

    // ── Reputation System ──────────────────────────────────────────────
    UFUNCTION(BlueprintCallable, Category="Alsasua|Factions|Reputation")
    float GetReputation(FName FactionId) const;

    UFUNCTION(BlueprintCallable, Category="Alsasua|Factions|Reputation")
    void ModifyReputation(FName FactionId, float Delta);

    UFUNCTION(BlueprintCallable, Category="Alsasua|Factions|Reputation")
    bool AreAllied(FName FactionA, FName FactionB) const;

private:
    bool CargarDesdeJSON();

    UPROPERTY()
    TMap<FName, FFactionData> Factions;

    UPROPERTY()
    TMap<FName, float> FactionReputation;
};
