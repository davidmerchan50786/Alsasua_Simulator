#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/AlsasuaStealthProfileInterface.h"
#include "DisguiseComponent.generated.h"

/**
 * Tipos de disfraz disponibles.
 *   Cada tipo tiene multipliers de detección y costes de durabilidad distintos.
 */
UENUM(BlueprintType)
enum class EDisguiseType : uint8
{
	None,
	Momotxorro,       // Disfraz tradicional (Sakoa, Adarrak, Mozorroa) — máxima disimulación
	Casual_Infiltrator, // Ropa normal — disimulación moderada
	Press_Press       // Acreditación de prensa — buena disimulación urbana
};

/**
 * Configuración por tipo de disfraz.
 * Editable desde Blueprint o datos.
 */
USTRUCT(BlueprintType)
struct GF_SOCIAL_API FDisguiseTypeInfo
{
	GENERATED_BODY()

	/** Multiplicador de detección visual (0 = invisible, 1 = sin efecto). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disguise")
	float DetectionMultiplier = 0.7f;

	/** Durabilidad máxima de este tipo de disfraz. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disguise", meta = (ClampMin = "10"))
	float MaxDurability = 100.f;

	/** Coste de durabilidad por segundo al caminar (passive drain). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disguise|Cost", meta = (ClampMin = "0"))
	float WalkDrainPerSecond = 0.3f;

	/** Coste de durabilidad por segundo al sprintear. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disguise|Cost", meta = (ClampMin = "0"))
	float SprintDrainPerSecond = 2.5f;

	/** Coste por salto. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disguise|Cost", meta = (ClampMin = "0"))
	float JumpCost = 1.5f;

	/** Coste por ataque (disparo/golpe). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disguise|Cost", meta = (ClampMin = "0"))
	float AttackCost = 15.f;

	/** Coste por estar a menos de X cm de un guardia (por segundo). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disguise|Cost", meta = (ClampMin = "0"))
	float NearGuardDrainPerSecond = 4.f;

	/** Radio para considerarse "cerca de un guardia" (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disguise|Cost", meta = (ClampMin = "50"))
	float NearGuardRadius = 400.f;

	/** Reduce el ruido del jugador (0 = silencio total, 1 = ruido normal). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disguise|Cost", meta = (ClampMin = "0", ClampMax = "1"))
	float NoiseReduction = 0.5f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDisguiseChanged, EDisguiseType, NewType, EDisguiseType, OldType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDisguiseBroken, EDisguiseType, Type);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDisguiseDurabilityChanged, float, NewDurability);

/**
 * UDisguiseComponent — sistema de disfraz unificado.
 *
 * Componente per-actor que gestiona equipamiento, degradación contextual
 * y efectos de detección para IA. Reemplaza el antiguo UDisfrazSubsystem.
 *
 * Uso:
 *   1. Colocar en el Character del jugador.
 *   2. Llamar EquipDisguise() al entrar a una safehouse.
 *   3. Llamar OnSprint()/OnAttack()/OnJump() desde el Character al realizar acciones.
 *   4. Consultar GetEffectiveDetectionMultiplier() desde GuardDetectionComponent.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GF_SOCIAL_API UDisguiseComponent : public UActorComponent, public IAlsasuaStealthProfile
{
	GENERATED_BODY()

public:
	UDisguiseComponent();

	// ── API pública ────────────────────────────────────────────────────────

	/**
	 * Equipar un disfraz.
	 * @param Type           Tipo de disfraz a equipar.
	 * @param bConsumable    Si true, se destruye al agotar durabilidad.
	 * @param InitialDurability Durabilidad inicial (override, -1 = usar MaxDurability del tipo).
	 */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Disguise")
	void EquipDisguise(EDisguiseType Type, bool bConsumable = false, float InitialDurability = -1.f);

	/**
	 * Desequipar el disfraz actual.
	 */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Disguise")
	void UnequipDisguise();

	/**
	 * Consumir durabilidad directamente (ej: disparar, ser visto haciendo algo sospechoso).
	 */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Disguise")
	void UseDisguise(float Amount);

	/**
	 * Notificar que el jugador está sprinteando.
	 * Llamar cada frame que el jugador sprintee.
	 */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Disguise")
	void NotifySprint(bool bIsSprinting);

	/**
	 * Notificar que el jugador saltó.
	 */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Disguise")
	void NotifyJump();

	/**
	 * Notificar que el jugador atacó.
	 */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Disguise")
	void NotifyAttack();

	/**
	 * Actualizar posición del jugador y lista de guardias cercanos.
	 * Llamar desde Tick del Character o desde GuardDetectionComponent.
	 * @param PlayerLocation  Posición del jugador.
	 * @param GuardLocations  Posiciones de guardias activos en el área.
	 */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Disguise")
	void UpdateNearbyGuards(const FVector& PlayerLocation, const TArray<FVector>& GuardLocations);

	// ── Queries ────────────────────────────────────────────────────────────

	/** Multiplicador de detección efectivo (factura durabilidad y tipo). */
	UFUNCTION(BlueprintPure, Category = "Alsasua|Disguise")
	float GetEffectiveDetectionMultiplier() const;

	/** Factor de reducción de ruido (0 = silencio, 1 = normal). */
	UFUNCTION(BlueprintPure, Category = "Alsasua|Disguise")
	float GetNoiseReduction() const;

	/** ¿Tiene disfraz equipado? */
	UFUNCTION(BlueprintPure, Category = "Alsasua|Disguise")
	bool IsDisguised() const { return CurrentDisguise != EDisguiseType::None; }

	/** Tipo de disfraz actual. */
	UFUNCTION(BlueprintPure, Category = "Alsasua|Disguise")
	EDisguiseType GetCurrentDisguise() const { return CurrentDisguise; }

	/** Durabilidad actual (0-100). */
	UFUNCTION(BlueprintPure, Category = "Alsasua|Disguise")
	float GetDurability() const { return Durability; }

	// ── IAlsasuaStealthProfile (contrato del Kernel) ──────────────────────

	virtual float GetVisionMultiplier_Implementation() const override
	{
		return GetEffectiveDetectionMultiplier();
	}

	virtual float GetNoiseDampening_Implementation() const override
	{
		return GetNoiseReduction();
	}

	/** Durabilidad máxima del disfraz actual. */
	UFUNCTION(BlueprintPure, Category = "Alsasua|Disguise")
	float GetMaxDurability() const;

	/** Porcentaje de durabilidad (0-1). */
	UFUNCTION(BlueprintPure, Category = "Alsasua|Disguise")
	float GetDurabilityPercent() const;

	// ── Configuración por tipo ─────────────────────────────────────────────

	/** Configuración por tipo de disfraz. Indexado por (int32)EDisguiseType. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disguise|Config")
	FDisguiseTypeInfo MomotxorroConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disguise|Config")
	FDisguiseTypeInfo CasualConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disguise|Config")
	FDisguiseTypeInfo PressConfig;

	// ── Delegados ──────────────────────────────────────────────────────────

	/** Se emite al cambiar de disfraz (incluye quitar). */
	UPROPERTY(BlueprintAssignable, Category = "Alsasua|Disguise")
	FOnDisguiseChanged OnDisguiseChanged;

	/** Se emite cuando el disfraz se rompe (durabilidad = 0). */
	UPROPERTY(BlueprintAssignable, Category = "Alsasua|Disguise")
	FOnDisguiseBroken OnDisguiseBroken;

	/** Se emite al cambiar la durabilidad (para HUD). */
	UPROPERTY(BlueprintAssignable, Category = "Alsasua|Disguise")
	FOnDisguiseDurabilityChanged OnDurabilityChanged;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// ── Estado ─────────────────────────────────────────────────────────────

	EDisguiseType CurrentDisguise = EDisguiseType::None;
	float Durability = 0.f;
	bool bIsConsumable = false;
	bool bIsSprinting = false;
	bool bNearGuard = false;

	// ── Config lookup ──────────────────────────────────────────────────────

	const FDisguiseTypeInfo& GetConfigForType(EDisguiseType Type) const;
	void BreakDisguise();

	// ── Tick helpers ───────────────────────────────────────────────────────

	void TickPassiveDrain(float DeltaTime);
	void TickDurabilityCheck();
};
