// EconomiaCriminalSubsystem.h (capa GAMEPLAY)
// Impuesto revolucionario (extorsión) + tráfico. Puerto de SistemaEconomiaCriminal.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "EconomiaCriminalSubsystem.generated.h"

class ANegocioActor;

UCLASS()
class ALSASUAGAMEPLAY_API UEconomiaCriminalSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	void Registrar(ANegocioActor* N);
	void Quitar(ANegocioActor* N);

	UFUNCTION(BlueprintCallable, Category="Crimen") void Extorsionar(ANegocioActor* N);
	UFUNCTION(BlueprintCallable, Category="Crimen") void Trapichear();

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UEconomiaCriminalSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }

private:
	UPROPERTY() TArray<TWeakObjectPtr<ANegocioActor>> Negocios;
	float Acumulado = 0.f;
	static constexpr float PERIODO = 60.f;
};
