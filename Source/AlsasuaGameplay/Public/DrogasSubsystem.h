// DrogasSubsystem.h (capa GAMEPLAY)
// Colocón con efectos sobre sistemas (mecánica ficticia). Puerto de SistemaDrogas.
// Los efectos VISUALES/AUDIO van en el módulo de UI/Audio leyendo este estado.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "AlsasuaTypes.h"
#include "DrogasSubsystem.generated.h"

UCLASS()
class ALSASUAGAMEPLAY_API UDrogasSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category="Drogas") ESustancia Activa = ESustancia::Ninguna;
	UPROPERTY(BlueprintReadOnly, Category="Drogas") float TiempoRestante = 0.f;
	UPROPERTY(BlueprintReadWrite, Category="Drogas") float Borrachera = 0.f;   // 0-100 (de las acciones)

	// Modificadores que leen el disparo / el daño / el movimiento.
	UFUNCTION(BlueprintPure, Category="Drogas") float MultDispersion() const { return MultDisp * (1.f + Borrachera / 80.f); }
	UFUNCTION(BlueprintPure, Category="Drogas") float ReduccionDanoRecibido() const { return ReduDano; }
	UFUNCTION(BlueprintPure, Category="Drogas") float MultVelocidad() const { return MultVel; }

	UFUNCTION(BlueprintCallable, Category="Drogas") void Tomar(ESustancia S);

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UDrogasSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }

private:
	float MultDisp = 1.f, ReduDano = 1.f, MultVel = 1.f;
	void Bajada();
};
