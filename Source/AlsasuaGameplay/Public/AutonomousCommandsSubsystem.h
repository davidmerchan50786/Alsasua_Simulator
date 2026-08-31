#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "AutonomousCommandsSubsystem.generated.h"

/**
 * Orchestrates autonomous NPC actions during civil unrest.
 * Based on global tension + popular support + manifestation state.
 *
 * Actions (all autonomous, player observes):
 *  - Asamblea: NPCs gather to decide tactics
 *  - Secuestro: NPCs capture banker/business NPCs near banks
 *  - Bombas: NPCs place explosives on government/commercial buildings
 *  - Lucha de clases: NPCs attack symbols of power
 *
 * Every action has a cooldown and requires minimum tension threshold.
 */
UCLASS()
class ALSASUAGAMEPLAY_API UAutonomousCommandsSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UAutonomousCommandsSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }

	/** Minimum tension to trigger any autonomous action */
	UPROPERTY(EditAnywhere, Category="Autonomia") float TensionMinima = 0.6f;

	/** Cooldowns between actions (seconds) */
	UPROPERTY(EditAnywhere, Category="Autonomia") float CooldownAsamblea = 45.f;
	UPROPERTY(EditAnywhere, Category="Autonomia") float CooldownSecuestro = 60.f;
	UPROPERTY(EditAnywhere, Category="Autonomia") float CooldownBomba = 90.f;
	UPROPERTY(EditAnywhere, Category="Autonomia") float CooldownLucha = 30.f;

	/** Number of NPCs participating in each action */
	UPROPERTY(EditAnywhere, Category="Autonomia") int32 NPCsAsamblea = 12;
	UPROPERTY(EditAnywhere, Category="Autonomia") int32 NpcSecuestro = 4;
	UPROPERTY(EditAnywhere, Category="Autonomia") int32 NPCsBomba = 2;

	/** Bomb fuse time in seconds */
	UPROPERTY(EditAnywhere, Category="Autonomia") float BombaTiempo = 5.f;

	/** Damage dealt to buildings during class struggle per second */
	UPROPERTY(EditAnywhere, Category="Autonomia") float LuchaDanoPorSeg = 25.f;

	// Current state
	UPROPERTY(BlueprintReadOnly, Category="Autonomia") bool bAsambleaActiva = false;
	UPROPERTY(BlueprintReadOnly, Category="Autonomia") bool bSecuestroActivo = false;
	UPROPERTY(BlueprintReadOnly, Category="Autonomia") bool bLuchaActiva = false;
	UPROPERTY(BlueprintReadOnly, Category="Autonomia") int32 BombasColocadas = 0;

	/** Delegate broadcast when an action starts/stops */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAccionAutonoma, FName, Accion, bool, bActiva);
	UPROPERTY(BlueprintAssignable) FOnAccionAutonoma OnAccionAutonoma;

private:
	float TimerAsamblea = 0.f;
	float TimerSecuestro = 0.f;
	float TimerBomba = 0.f;
	float TimerLucha = 0.f;
	float TimerEval = 0.f;

	void EvaluarAcciones(float DeltaTime);
	void IniciarAsamblea();
	void DetenerAsamblea();
	void IniciarSecuestro();
	void DetenerSecuestro();
	void ColocarBomba();
	void IniciarLuchaClases();
	void DetenerLuchaClases();

	AActor* EncontrarEdificioObjetivo() const;
	AActor* EncontrarNPCBanquero() const;
};
