#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "ManifaManager.generated.h"

UCLASS()
class GF_CORE_API UManifaManager : public UWorldSubsystem, public FTickableGameObject {
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
    virtual bool IsAllowedToTick() const override { return true; }
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(ManifaManager, STATGROUP_Game); }
};