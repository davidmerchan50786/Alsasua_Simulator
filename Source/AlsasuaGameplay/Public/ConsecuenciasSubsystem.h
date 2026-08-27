// ConsecuenciasSubsystem.h (capa GAMEPLAY)
// El apoyo popular tiene memoria: matar a un CIVIL baja el apoyo (daño
// colateral); matar a un GUARDIA paga recompensa. Puerto de
// SistemaConsecuencias + el recompensa de GuardiaCivilAI.Morir() del
// prototipo Unity. Lo invoca el arma al causar daño — AAlsasuaNPC vive en
// AlsasuaEntities, por debajo de AlsasuaGameplay en la capa, así que la
// consecuencia se decide aquí (arriba) leyendo su estado público
// (EstaMuerto/bEsPolicia), no dentro de NPC::Morir().
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "ConsecuenciasSubsystem.generated.h"

UCLASS()
class ALSASUAGAMEPLAY_API UConsecuenciasSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// $ al jugador por cada guardia civil abatido. Mismo valor que
	// GuardiaCivilAI.recompensa en el prototipo Unity (200).
	UPROPERTY(EditAnywhere, Category="Consecuencias") int32 RecompensaGuardia = 200;

	// Llamar tras aplicar daño a una víctima. Si acaba de morir: civil ->
	// penaliza apoyo popular; guardia -> paga recompensa. Una vez por víctima.
	void RegistrarDano(AActor* Victima);

	// ── Detention consequences ─────────────────────────────────────────────
	/** % of cash lost on surrender. */
	UPROPERTY(EditAnywhere, Category="Detencion") float SurrenderCashLossPercent = 0.25f;
	/** Items lost on surrender. */
	UPROPERTY(EditAnywhere, Category="Detencion") int32 SurrenderItemsLost = 2;
	/** Apoyo lost on surrender. */
	UPROPERTY(EditAnywhere, Category="Detencion") float SurrenderApoyoLoss = 15.f;
	/** Paranoia gained on surrender. */
	UPROPERTY(EditAnywhere, Category="Detencion") float SurrenderParanoiaGain = 10.f;
	/** Wanted reset on surrender. */
	UPROPERTY(EditAnywhere, Category="Detencion") float SurrenderWantedReset = 0.f;
	/** Apoyo lost on escape. */
	UPROPERTY(EditAnywhere, Category="Detencion") float EscapeApoyoLoss = 5.f;

	/** Apply detention consequences when player escapes or surrenders. */
	void AplicarConsecuenciasDetencion(AActor* Jugador, bool bEscaped);

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UConsecuenciasSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }

private:
	UPROPERTY() TSet<uint32> MuertesContadas;   // una consecuencia por víctima
	UPROPERTY() bool bBoundToPlayer = false;

	void BindToPlayerDetention(AActor* Player);

	UFUNCTION()
	void HandleDetentionResult(bool bEscaped);
};
