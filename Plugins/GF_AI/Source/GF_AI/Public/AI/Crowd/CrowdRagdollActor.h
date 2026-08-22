#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CrowdRagdollActor.generated.h"

/**
 * Niveles de calidad del ragdoll según distancia al jugador.
 *   Full   → física completa con skeletal mesh
 *   Frozen → impulso + freeze en pose (sin ongoing physics)
 *   None   → sin ragdoll (ISMC se oculta directamente)
 */
UENUM(BlueprintType)
enum class ERagdollQuality : uint8
{
    Full,
    Frozen,
    None
};

/**
 * Actor de ragdoll de multitud, poolable.
 * Soporta 3 niveles de calidad según distancia al jugador.
 *
 * Full: física completa, dura RagdollDuration segundos
 * Frozen: impulso breve, freeze rápido, sin ongoing physics
 * Devuelto al pool automáticamente con fade-out.
 */
UCLASS()
class GF_AI_API ACrowdRagdollActor : public ACharacter
{
	GENERATED_BODY()

public:
	ACrowdRagdollActor();
	virtual void Tick(float DeltaTime) override;

	/**
	 * Activa el ragdoll con el nivel de calidad indicado.
	 * @param ImpulseDir       Dirección del impulso (se normaliza).
	 * @param ImpulseStrength  Magnitud del impulso.
	 * @param Quality          Nivel de calidad según distancia al jugador.
	 */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Crowd")
	void ActivateRagdoll(FVector ImpulseDir, float ImpulseStrength, ERagdollQuality Quality = ERagdollQuality::Full);

	/** Resetea el actor para ser devuelto al pool. */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Crowd")
	void DeactivateForPool();

	/** Nivel de calidad actual (para debugging). */
	UPROPERTY(BlueprintReadOnly, Category = "Alsasua|Crowd")
	ERagdollQuality CurrentQuality = ERagdollQuality::None;

	// ── Configuración por tier ──────────────────────────────────────────────

	/** Duración de física activa en modo Full (segundos). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll|Full")
	float FullRagdollDuration = 3.5f;

	/** Duración del impulso antes de freeze en modo Frozen (segundos). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll|Frozen")
	float FrozenImpulseTime = 0.4f;

	/** Tiempo de fade-out antes de devolver al pool (segundos). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	float FadeOutDuration = 1.5f;

	/** Multiplicador de impulso al morir. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	float ImpulseMultiplier = 800.0f;

protected:
	virtual void BeginPlay() override;

private:
	// ── Timers ──────────────────────────────────────────────────────────────
	FTimerHandle PhysicsTimerHandle;
	// FadeTimerHandle se queda: los dos ClearTimer() que lo usan son inocuos y
	// quitarlo tocaría dos ficheros más por nada. Pero conviene saber que
	// nadie le hace SetTimer: no arma ningún temporizador, el fade va por Tick.
	FTimerHandle FadeTimerHandle;
	FTimerHandle ReturnTimerHandle;

	// ── Fade state ──────────────────────────────────────────────────────────
	float FadeAlpha = 1.f;
	float FadeTimer = 0.f;
	bool bIsFading = false;

	// ── Callbacks ───────────────────────────────────────────────────────────
	void OnPhysicsExpired();
	// OnFadeTick() estaba declarada y no la definía nadie: el fade se hace en
	// Tick, no por temporizador ("más barato que timer 30Hz", dice
	// StartFadeOut). Los otros dos callbacks de temporizador sí existen.
	void OnReturnToPool();

	// ── Internals ───────────────────────────────────────────────────────────
	void StartFadeOut();
	void FreezePhysics();
};
