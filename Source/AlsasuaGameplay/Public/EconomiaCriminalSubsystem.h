// EconomiaCriminalSubsystem.h (capa GAMEPLAY)
// Impuesto revolucionario (extorsión) + tráfico. Puerto de SistemaEconomiaCriminal.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "EconomiaCriminalSubsystem.generated.h"

class ANegocioActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCriminalActivity, FName, ActivityType, int32, Severity);

UCLASS()
class ALSASUAGAMEPLAY_API UEconomiaCriminalSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	void Registrar(ANegocioActor* N);
	void Quitar(ANegocioActor* N);

	UFUNCTION(BlueprintCallable, Category="Crimen") void Extorsionar(ANegocioActor* N);
	UFUNCTION(BlueprintCallable, Category="Crimen") void Trapichear();

	/** Static: fires on any criminal act. Other modules (GF_Social) subscribe. */
	static FOnCriminalActivity OnCriminalActivity;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UEconomiaCriminalSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }

private:
	UPROPERTY() TArray<TWeakObjectPtr<ANegocioActor>> Negocios;
	float Acumulado = 0.f;
	static constexpr float PERIODO = 60.f;

	bool bCooldownActive = false;
	float CooldownTimer = 0.f;
	static constexpr float REDADA_COOLDOWN = 120.f;
};
