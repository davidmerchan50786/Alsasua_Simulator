// ClimaSubsystem.h (capa GAMEPLAY)
// Clima dinámico: evoluciona entre estados (despejado/nubes/lluvia/tormenta/
// niebla), controla la niebla de altura, expone un factor de nubosidad que el
// ciclo visual usa para atenuar el sol, y lanza VFX de lluvia (si hay asset).
// Puerto de SistemaClima.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "ClimaSubsystem.generated.h"

class UNiagaraComponent;
class AExponentialHeightFog;
class ADirectionalLight;

UENUM(BlueprintType)
enum class EClima : uint8 { Despejado, Nubes, Lluvia, Tormenta, Niebla };

UCLASS()
class ALSASUAGAMEPLAY_API UClimaSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Clima") float TransicionPorSeg = 0.08f;   // velocidad de cambio
	UPROPERTY(EditAnywhere, Category="Clima") float CambioMinSeg = 60.f;
	UPROPERTY(EditAnywhere, Category="Clima") float CambioMaxSeg = 180.f;

	// Tormenta: relámpagos + truenos.
	UPROPERTY(EditAnywhere, Category="Clima|Tormenta") float RayoMinSeg = 6.f;
	UPROPERTY(EditAnywhere, Category="Clima|Tormenta") float RayoMaxSeg = 22.f;
	UPROPERTY(EditAnywhere, Category="Clima|Tormenta") float IntensidadFlash = 18.f;

	// Estado continuo actual (0..1).
	UFUNCTION(BlueprintCallable, Category="Clima") float Lluvia() const { return Cur.Lluvia; }
	UFUNCTION(BlueprintCallable, Category="Clima") float Niebla() const { return Cur.Niebla; }
	// 1 = cielo despejado, ->0.4 tormenta. Lo lee el ciclo visual para atenuar el sol.
	UFUNCTION(BlueprintCallable, Category="Clima") float FactorNubosidad() const { return 1.f - 0.6f * Cur.Nubosidad; }

	UFUNCTION(BlueprintCallable, Category="Clima") void ForzarClima(EClima C);

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UClimaSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }

private:
	struct FCondicion { float Lluvia = 0.f, Niebla = 0.f, Nubosidad = 0.f; };
	FCondicion Cur;
	FCondicion Objetivo;
	float TiempoCambio = 8.f;

	bool bInit = false;
	UPROPERTY() AExponentialHeightFog* Niebla_ = nullptr;
	UPROPERTY() UNiagaraComponent* LluviaVFX = nullptr;

	// Tormenta (relámpagos/truenos).
	UPROPERTY() ADirectionalLight* Relampago = nullptr;
	float ProxRayo = 8.f;
	float FlashTimer = 0.f;
	float TruenoDelay = -1.f;

	// Mojado del suelo (escalar del MPC_Clima).
	float Wetness = 0.f;
	UPROPERTY() class UMaterialParameterCollection* MPCClima = nullptr;
	void GestionarMojado(float DeltaTime);

	static FCondicion Preset(EClima C);
	void ElegirClimaAleatorio();
	void AplicarNiebla();
	void GestionarLluviaVFX();
	void GestionarTormenta(float DeltaTime);
};
