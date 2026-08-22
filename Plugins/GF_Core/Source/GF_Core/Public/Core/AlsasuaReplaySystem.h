#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "AlsasuaReplaySystem.generated.h"

USTRUCT(BlueprintType)
struct FReplaySnapshot
{
    GENERATED_BODY()

    UPROPERTY()
    float TimeStamp = 0.f;

    UPROPERTY()
    float GlobalTension = 0.f;

    UPROPERTY()
    TArray<FVector> ActorLocations;
};

UCLASS()
class GF_CORE_API UAlsasuaReplaySystem : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override { return true; }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UAlsasuaReplaySystem, STATGROUP_Game); }

    UFUNCTION(BlueprintCallable, Category = "AAA|Replay")
    void StartRecording();

    UFUNCTION(BlueprintCallable, Category = "AAA|Replay")
    void StopRecording();

    UFUNCTION(BlueprintCallable, Category = "AAA|Replay")
    void SaveReplayToFile(FString FileName);

private:
    bool bIsRecording = false;
    TArray<FReplaySnapshot> CurrentSession;

    // Throttle: capturar cada N segundos en vez de cada frame.
    float CaptureInterval = 0.5f;
    float CaptureTimer = 0.0f;

    // Cache de actores clave (evitar GetAllActorsOfClass cada captura).
    UPROPERTY()
    TArray<TObjectPtr<ACharacter>> CachedKeyActors;
    float CacheRefreshInterval = 3.0f;
    float CacheRefreshTimer = 0.0f;

    void CaptureFrame();
    void RefreshActorCache();
};
