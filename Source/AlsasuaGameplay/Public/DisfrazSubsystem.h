// DisfrazSubsystem.h (capa GAMEPLAY)
// Wrapper simple de backward-compat. El sistema completo vive en
// UDisguiseComponent (AlsasuaKernel, antes módulo AlsasuaManifa).
//
// Los callers existentes (PlayerController, ArmasComponent, PoliciaController)
// siguen funcionando sin cambios.
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
	UPROPERTY(BlueprintReadOnly, Category = "Disfraz")
	bool bEncubierto = false;

	UFUNCTION(BlueprintPure, Category = "Disfraz")
	float FactorReconocimiento() const { return bEncubierto ? 0.4f : 1.f; }

	UFUNCTION(BlueprintCallable, Category = "Disfraz")
	void Alternar();

	UFUNCTION(BlueprintCallable, Category = "Disfraz")
	void Delatar();

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UDisfrazSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }

private:
	float Cooldown = 0.f;
};
