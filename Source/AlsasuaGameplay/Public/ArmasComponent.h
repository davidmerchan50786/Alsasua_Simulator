// ArmasComponent.h (capa GAMEPLAY)
// Armas del jugador: disparo (line trace) y melee, con daño vía IDamageable.
// Puerto de SistemaArmasExtendido (la parte de combate). Pon este componente
// en el personaje jugador.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaTypes.h"
#include "ArmasComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAmmoChanged, int32, CurrentAmmo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHitMark, bool, bIsHeadshot);

UCLASS(ClassGroup=(Alsasua), meta=(BlueprintSpawnableComponent))
class ALSASUAGAMEPLAY_API UArmasComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UArmasComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Armas") ETipoArma ArmaActual = ETipoArma::Punos;

	// Munición actual por arma (índice = ETipoArma).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Armas") TArray<int32> Municion;

	// Munición máxima por arma.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Armas") TArray<int32> MunicionMax;

	// Reserva de munición por arma (para recargar).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Armas") TArray<int32> Reserva;

	// Tiempo de recarga por arma (segundos).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Armas") TArray<float> TiempoRecarga;

	// ¿Está recargando actualmente?
	UPROPERTY(BlueprintReadOnly, Category="Armas") bool bReloading = false;

	// Delegados.
	UPROPERTY(BlueprintAssignable, Category="Armas") FOnAmmoChanged OnAmmoChanged;
	UPROPERTY(BlueprintAssignable, Category="Armas") FOnReloadStarted OnReloadStarted;
	UPROPERTY(BlueprintAssignable, Category="Armas") FOnReloadFinished OnReloadFinished;
	UPROPERTY(BlueprintAssignable, Category="Armas") FOnHitMark OnHitMark;

	/** Headshot damage multiplier (bone = head/trajedia). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Armas") float HeadshotMultiplier = 2.5f;

	/** Distance (cm) beyond which damage starts falling off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Armas") float DamageFalloffStart = 3000.f;
	/** Damage scales linearly to 0 over this range past DamageFalloffStart. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Armas") float DamageFalloffRange = 20000.f;

	/** Recoil pattern: extra kick per consecutive shot (climbs while firing). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Armas") float RecoilPerShot = 0.08f;
	/** Recoil climbs toward this cap while firing continuously. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Armas") float RecoilMax = 0.8f;
	/** Recoil decays this fast per second when not firing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Armas") float RecoilDecay = 1.5f;

	/** How many non-damageable obstacles a bullet passes through (penetration). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Armas") int32 MaxPenetration = 1;
	/** Stop the bullet on the first damageable target (no over-penetration). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Armas") bool bStopOnFirstHit = true;

	/** Aim assist (controller): soft lock-on when aiming a firearm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Armas|AimAssist") bool bAimAssistEnabled = true;
	/** Max deviation from the view ray a target can be and still snap (degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Armas|AimAssist") float AimAssistCone = 8.f;
	/** Max range of aim assist target acquisition. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Armas|AimAssist") float AimAssistRadius = 25000.f;

	UFUNCTION(BlueprintCallable, Category="Armas") void CambiarArma(ETipoArma Arma);
	UFUNCTION(BlueprintCallable, Category="Armas") void RecogerArma(ETipoArma Arma, int32 Cantidad);
	UFUNCTION(BlueprintCallable, Category="Armas") void RecogerMunicion(ETipoArma Arma, int32 Cantidad);

	UFUNCTION(BlueprintCallable, Category="Armas") int32 MunicionActual() const;
	UFUNCTION(BlueprintCallable, Category="Armas") int32 MunicionMaxActual() const;
	UFUNCTION(BlueprintCallable, Category="Armas") int32 ReservaActual() const;
	UFUNCTION(BlueprintCallable, Category="Armas") FString NombreArma() const;
	UFUNCTION(BlueprintCallable, Category="Armas") bool EsCuerpoACuerpo() const { return ArmaActual == ETipoArma::Punos; }
	UFUNCTION(BlueprintCallable, Category="Armas") bool CanReload() const;

	// Acción de "usar arma" (clic izquierdo). Decide melee o disparo según el arma.
	UFUNCTION(BlueprintCallable, Category="Armas") void UsarArma();

	// Recarga manual.
	UFUNCTION(BlueprintCallable, Category="Armas") void Reload();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	float Cooldown = 0.f;
	float ReloadTimer = 0.f;
	float RecoilStack = 0.f;
	bool bBombaLapaActive = false;
	FVector BombaLapaLocation;

	// ── Preloaded VFX/Audio assets (avoid LoadObject in hot paths) ──
	UPROPERTY() class USoundBase* SDisparo = nullptr;
	UPROPERTY() class USoundBase* SImpacto = nullptr;
	UPROPERTY() class USoundBase* SExplosion = nullptr;
	UPROPERTY() class USoundBase* SExplosionGrande = nullptr;
	UPROPERTY() class USoundBase* SMolotov = nullptr;
	UPROPERTY() class USoundBase* SBombaColocar = nullptr;
	UPROPERTY() class UNiagaraSystem* NSFogonazo = nullptr;
	UPROPERTY() class UNiagaraSystem* NSSangre = nullptr;
	UPROPERTY() class UNiagaraSystem* NSImpacto = nullptr;
	UPROPERTY() class UNiagaraSystem* NSSpray = nullptr;
	UPROPERTY() class UNiagaraSystem* NSMolotov = nullptr;
	UPROPERTY() class UNiagaraSystem* NSExplosion = nullptr;
	UPROPERTY() class UNiagaraSystem* NSExplosionCoche = nullptr;

	void PreloadAssets();

	void GolpearMelee();
	void DispararFuego(int32 Dano, int32 Perdigones, float DispersionGrados, float Cadencia);
	void LanzarSpray();
	void LanzarTirachinas();
	void LanzarMolotov();
	void GolpearIkurrina();
	void ColocarBombaLapa();
	void DetonarBombaLapa();
	void CocheBombaDetonar();
	void FinishReload();

	bool ObtenerMira(FVector& OutOrigen, FVector& OutDir) const;
	bool ApplyAimAssist(FVector& OutDir, const FVector& Origen) const;
	float MultDispersionActual() const;
	void SubirBusqueda(int32 Cantidad) const;
	void Consecuencias(AActor* Victima) const;

	void InitAmmoArrays();
	void NotifyNearbyGuards(FVector Location, float Loudness);
};
