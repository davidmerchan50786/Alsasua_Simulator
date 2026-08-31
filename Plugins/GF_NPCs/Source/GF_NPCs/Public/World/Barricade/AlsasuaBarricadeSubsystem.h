#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "World/Barricade/AlsasuaBarricadeActor.h"
#include "AlsasuaBarricadeSubsystem.generated.h"

/**
 * Manages barricade placement, destruction, and navigation blocking.
 * During Blockade tactic, crowd places barricades on road segments.
 * Barricades block NPC/vehicle navigation and can be destroyed by police.
 */
UCLASS()
class GF_NPCS_API UAlsasuaBarricadeSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    /** Place a barricade at location on a road segment */
    UFUNCTION(BlueprintCallable, Category = "Barricade")
    AAlsasuaBarricadeActor* ColocarBarricada(const FVector& Location, EBarricadeType Tipo = EBarricadeType::Contenedor, int32 TramoIdx = -1);

    /** Remove all barricades */
    UFUNCTION(BlueprintCallable, Category = "Barricade")
    void LimpiarBarricadas();

    /** Place barricades along a road segment to block it */
    UFUNCTION(BlueprintCallable, Category = "Barricade")
    int32 BloquearTramo(int32 TramoIdx, int32 NumBarricadas = 3);

    /** Check if a location is blocked by a barricade */
    UFUNCTION(BlueprintPure, Category = "Barricade")
    bool EstaBloqueado(const FVector& Location, float Radius = 200.f) const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barricade")
    int32 MaxBarricadas = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barricade")
    float VidaEntreBarricadas = 15.0f;

    const TArray<TWeakObjectPtr<AAlsasuaBarricadeActor>>& GetBarricadas() const { return Barricadas; }

    // FTickableGameObject
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UAlsasuaBarricadeSubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return !IsTemplate(); }

private:
    TArray<TWeakObjectPtr<AAlsasuaBarricadeActor>> Barricadas;
    float SpawnCooldown = 0.f;

    void LimpiarMuertas();
};
