// ParanoiaVisualSubsystem.h (capa GAMEPLAY)
// Cuando la paranoia global sube, el mundo visual cambia:
//   - NPCs parecen guardias civiles (material swap)
//   - Coches parecen patrol (material swap + sirena)
//   - Post-process: desaturación, aberración cromática, viñeta
//   - Alucinaciones: guardias fantasma que se acercan al jugador
// No puedes distinguir amenaza real de paranoia.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "ParanoiaVisualSubsystem.generated.h"

class UMaterialInterface;
class UPointLightComponent;
class UAudioComponent;
class USoundBase;
class UGameplayPostProcessComponent;

// Datos de una víctima civil muerta (para alucinaciones).
USTRUCT(BlueprintType)
struct FVictimaRecord
{
	GENERATED_BODY()

	UPROPERTY() FVector Ubicacion = FVector::ZeroVector;
	UPROPERTY() float TiempoMuerte = 0.f;
	UPROPERTY() bool bEsPolicia = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnParanoiaVisualCambia, float, NivelParanoia);

UCLASS()
class ALSASUAGAMEPLAY_API UParanoiaVisualSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// ── Umbrales ────────────────────────────────────────────────────────────
	/** Paranoia mínima para que NPCs empiecen a parecer guardias (0-100). */
	UPROPERTY(EditAnywhere, Category="Paranoia|Umbrales", meta=(ClampMin="0",ClampMax="100"))
	float GuardiaThreshold = 30.f;

	/** Paranoia para que TODOS los NPCs parezcan guardias (0-100). */
	UPROPERTY(EditAnywhere, Category="Paranoia|Umbrales", meta=(ClampMin="0",ClampMax="100"))
	float TodosGuardiaThreshold = 60.f;

	/** Paranoia para que coches parezcan patrol (0-100). */
	UPROPERTY(EditAnywhere, Category="Paranoia|Umbrales", meta=(ClampMin="0",ClampMax="100"))
	float PatrolThreshold = 40.f;

	/** Paranoia para spawnear alucinaciones (0-100). */
	UPROPERTY(EditAnywhere, Category="Paranoia|Umbrales", meta=(ClampMin="0",ClampMax="100"))
	float AlucinacionThreshold = 70.f;

	/** Paranoia máxima para efectos extremos (pantalla falla, sonido distorsionado). */
	UPROPERTY(EditAnywhere, Category="Paranoia|Umbrales", meta=(ClampMin="0",ClampMax="100"))
	float ExtremoThreshold = 85.f;

	// ── Materiales override ─────────────────────────────────────────────────
	/** Materiales GuardiaCivil para aplicar a NPCs (orden: slot 0, 1, ...). */
	UPROPERTY(EditAnywhere, Category="Paranoia|Materiales")
	TArray<TObjectPtr<UMaterialInterface>> MaterialesGuardia;

	/** Material patrol para coches. */
	UPROPERTY(EditAnywhere, Category="Paranoia|Materiales")
	TObjectPtr<UMaterialInterface> MaterialPatrol;

	// ── Audio ───────────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category="Paranoia|Audio")
	TObjectPtr<USoundBase> SirenaSound;

	UPROPERTY(EditAnywhere, Category="Paranoia|Audio")
	TObjectPtr<USoundBase> ParanoiaHeartbeat;

	// ── API ─────────────────────────────────────────────────────────────────
	UFUNCTION(BlueprintCallable, Category="Paranoia")
	void RegistrarVictimaCivil(FVector Ubicacion, bool bEsPolicia);

	UFUNCTION(BlueprintPure, Category="Paranoia")
	float GetNivelParanoia() const { return NivelParanoiaActual; }

	// ── Delegate ────────────────────────────────────────────────────────────
	UPROPERTY(BlueprintAssignable, Category="Paranoia")
	FOnParanoiaVisualCambia OnParanoiaVisualCambia;

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;

	// UGameInstanceSubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UParanoiaVisualSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }

private:
	float NivelParanoiaActual = 0.f;

	// ── Material swap state ─────────────────────────────────────────────────
	struct FOriginalMaterialState
	{
		TArray<TObjectPtr<UMaterialInterface>> Originales;
		bool bSwapped = false;
	};
	TMap<TObjectPtr<USceneComponent>, FOriginalMaterialState> MaterialStates;

	void SwapNPCMaterials(AActor* NPC, bool bGuardiaLook);
	void SwapVehicleMaterials(AActor* Vehicle, bool bPatrolLook);
	void RevertAllMaterials();
	void UpdateVisualOverrides();

	// ── Alucinaciones ───────────────────────────────────────────────────────
	UPROPERTY() TArray<FVictimaRecord> VictimRegistradas;
	UPROPERTY() TArray<TObjectPtr<AActor>> AlucinacionesActivas;
	float TimerSpawnAlucinacion = 0.f;

	void SpawnAlucinacion(FVector Ubicacion);
	void TickAlucinaciones(float DeltaTime);

	// ── Post-process ────────────────────────────────────────────────────────
	UGameplayPostProcessComponent* FindPlayerPostProcess() const;
	void UpdateParanoiaPostProcess(float DeltaTime);

	float HeartbeatTimer = 0.f;
};
