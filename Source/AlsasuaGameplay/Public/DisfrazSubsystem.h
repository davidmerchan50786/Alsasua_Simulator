// DisfrazSubsystem.h (capa GAMEPLAY)
// Encubierto: la policía te reconoce menos. Disparar te delata. Puerto de SistemaDisfraz.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "DisfrazSubsystem.generated.h"

UCLASS()
class ALSASUAGAMEPLAY_API UDisfrazSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category="Disfraz") bool bEncubierto = false;

	UFUNCTION(BlueprintPure, Category="Disfraz") float FactorReconocimiento() const { return bEncubierto ? 0.4f : 1.f; }

	UFUNCTION(BlueprintCallable, Category="Disfraz") void Alternar();   // tecla H
	UFUNCTION(BlueprintCallable, Category="Disfraz") void Delatar();    // al disparar

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UDisfrazSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }

private:
	float Cooldown = 0.f;
};
