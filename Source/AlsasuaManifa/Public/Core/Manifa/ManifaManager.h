#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ManifaManager.generated.h"

UCLASS()
class ALSASUAMANIFA_API UManifaManager : public UWorldSubsystem {
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly, Category="AAA|Manifa")
    int32 ActiveProtesters = 0;

    UPROPERTY(BlueprintReadOnly, Category="AAA|Manifa")
    float Momentum = 0.f; // 0..100 (Fuerza de la protesta)

    UFUNCTION(BlueprintCallable, Category="AAA|Manifa")
    void TriggerManifestation(FVector CenterLocation);

    UFUNCTION(BlueprintCallable, Category="AAA|Manifa")
    void UpdateManifestationStrength(float DeltaPopularSupport);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnManifaStateChanged, bool, bIsActive);
    UPROPERTY(BlueprintAssignable)
    FOnManifaStateChanged OnManifaStateChanged;

public:
    bool IsTickable() const { return true; }
    void Tick(float DeltaTime);
    TStatId GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(ManifaManager, STATGROUP_Tickables); }
};