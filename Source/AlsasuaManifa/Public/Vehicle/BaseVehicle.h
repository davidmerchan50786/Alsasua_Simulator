#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AlsasuaTypes.h"
#include "BaseVehicle.generated.h"

class UParticleSystem;
class USoundBase;

UCLASS()
class ALSASUAMANIFA_API ABaseVehicle : public APawn, public IDamageable
{
    GENERATED_BODY()

public:
    ABaseVehicle();

    // ── IDamageable ─────────────────────────────────────────────────────
    virtual int32 GetVida() const override    { return Vida; }
    virtual int32 GetVidaMax() const override { return VidaMaxima; }
    virtual bool  EstaMuerto() const override { return bDestruido; }
    virtual void  Curar(int32 Cantidad) override { Vida = FMath::Min(VidaMaxima, Vida + Cantidad); }
    virtual void  RecibirDano(int32 Cantidad, FVector Origen, ETipoDano Tipo) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    class UBoxComponent* CollisionBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    class UStaticMeshComponent* VehicleMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    class UFloatingPawnMovement* MovementComponent;

    // Estadisticas de conduccion
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Physics")
    float MaxSpeed = 2500.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Physics")
    float AccelerationExp = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Physics")
    float TurnSpeed = 45.f;

    UPROPERTY(BlueprintReadOnly, Category="AAA|Status")
    float CurrentSpeedKmh = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Vida")
    int32 Vida = 500;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Vida")
    int32 VidaMaxima = 500;

    UFUNCTION(BlueprintCallable, Category="AAA|Drive")
    virtual void Drive(float ForwardValue, float RightValue);

    UFUNCTION(BlueprintCallable, Category="AAA|Drive")
    virtual void ToggleEngine(bool bOn);

    UFUNCTION(BlueprintCallable, Category="AAA|Drive")
    void DetonateCarBomb();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    bool bEngineActive = true;
    bool bDestruido = false;

private:
    UPROPERTY()
    TObjectPtr<UParticleSystem> CachedExplosionVFX = nullptr;

    UPROPERTY()
    TObjectPtr<USoundBase> CachedExplosionSFX = nullptr;
};
