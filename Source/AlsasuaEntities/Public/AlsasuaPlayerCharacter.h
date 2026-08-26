// AAlsasuaCharacter — jugador en tercera persona (puerto de ControladorJugador.cs).
// Paso AAA-1: cuerpo Mannequin (UE5) + cámara/movimiento AAA + Enhanced Input.
// El input clásico (AxisMappings de DefaultInput.ini) queda como fallback siempre activo.
// Listo para Motion Matching: asigna un AnimBP de GASP cuando añadas el Game Animation Sample (Fab).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "AlsasuaTypes.h"
#include "AlsasuaPlayerCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerDied);

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;

UCLASS()
class ALSASUAENTITIES_API AAlsasuaPlayerCharacter : public ACharacter, public IDamageable
{
	GENERATED_BODY()

public:
	AAlsasuaPlayerCharacter();

	// ── Vida (IDamageable) ──────────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category="Vida") int32 Vida = 100;
	UPROPERTY(EditAnywhere,      Category="Vida") int32 VidaMaxima = 100;

	virtual int32 GetVida() const override    { return Vida; }
	virtual int32 GetVidaMax() const override { return VidaMaxima; }
	virtual bool  EstaMuerto() const override { return Vida <= 0; }
	virtual void  Curar(int32 Cantidad) override { Vida = FMath::Min(VidaMaxima, Vida + Cantidad); }
	// La reducción por "chute" (drogas) se aplica en la capa Gameplay del atacante (respeta capas).
	virtual void  RecibirDano(int32 Cantidad, FVector Origen, ETipoDano Tipo) override;

	// ── Enhanced Input (asigna estos assets en el Blueprint derivado del editor) ──
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input") UInputMappingContext* ContextoMapeo = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input") UInputAction* IA_Mover = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input") UInputAction* IA_Mirar = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input") UInputAction* IA_Saltar = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input") UInputAction* IA_Correr = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input") UInputAction* IA_Agacharse = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input") UInputAction* IA_Apuntar = nullptr;

	// ── Velocidades (cm/s) ──
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movimiento") float VelCaminar = 400.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movimiento") float VelCorrer  = 650.f;

	// ── Apuntado (ADS) ──
	UPROPERTY(BlueprintReadOnly, Category="Apuntado") bool bApuntando = false;
	UPROPERTY(EditAnywhere, Category="Apuntado") float FOVCadera = 90.f;
	UPROPERTY(EditAnywhere, Category="Apuntado") float FOVApuntar = 55.f;
	UPROPERTY(EditAnywhere, Category="Apuntado") float BrazoCadera = 350.f;
	UPROPERTY(EditAnywhere, Category="Apuntado") float BrazoApuntar = 160.f;
	UPROPERTY(EditAnywhere, Category="Apuntado") float FOVCorrer = 100.f;
	UPROPERTY(EditAnywhere, Category="Apuntado") float BrazoCorrer = 400.f;
	bool bCorriendo = false;

	bool EstaApuntando() const { return bApuntando; }

	/** Static: fires when player dies. Gameplay layer subscribes for respawn. */
	static FOnPlayerDied OnPlayerDied;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	void ApuntarInicio();
	void ApuntarFin();

	// Enhanced Input
	void EntradaMover(const FInputActionValue& V);
	void EntradaMirar(const FInputActionValue& V);
	void CorrerInicio();
	void CorrerFin();
	void AgacharseToggle();

	// Parkour: saltar o trepar un repecho si lo hay delante.
	void SaltarOTrepar();
	bool IntentarTrepar();
	bool bTrepando = false;
	FVector TrepaInicio = FVector::ZeroVector, TrepaFin = FVector::ZeroVector;
	float TrepaT = 0.f, TrepaDur = 0.4f;
	UPROPERTY(EditAnywhere, Category="Parkour") float AlturaTrepaMax = 220.f;   // cm
	UPROPERTY(EditAnywhere, Category="Parkour") float AlcanceTrepa = 120.f;

	// Construye en runtime el contexto + acciones (teclado y mando) si no hay
	// assets asignados en el editor. Da soporte de gamepad sin depender de uasset.
	void AsegurarInputRuntime();
	bool bInputRuntimeHecho = false;

	// Fallback clásico
	void MoveForward(float Value);
	void MoveRight(float Value);
	void TurnAt(float Value);
	void LookUpAt(float Value);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cámara")
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cámara")
	UCameraComponent* Camera;
};
