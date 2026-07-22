#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "InputActionValue.h"
#include "AlsasuaCore.h"
#include "AlsasuaMovementTypes.h"
#include "AlsasuaTypes.h"
#include "AlsasuaCharacter.generated.h"

class UAlsasuaAbilitySystemComponent;
class UAlsasuaAttributeSet;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UCharacterTrajectoryComponent;

UCLASS(config=Game)
class ALSASUAMANIFA_API AAlsasuaCharacter : public ACharacter, public IAbilitySystemInterface, public IDamageable
{
	GENERATED_BODY()

public:
	AAlsasuaCharacter();

	// ── IAbilitySystemInterface ────────────────────────────────────────────
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	FORCEINLINE class UAlsasuaAbilitySystemComponent* GetAlsasuaAbilitySystem() const { return AbilitySystemComponent; }
	FORCEINLINE class UAlsasuaAttributeSet* GetAttributeSet() const { return AttributeSet; }
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	// ── GAS Health wrappers ────────────────────────────────────────────────
	UFUNCTION(BlueprintCallable, Category = "AAA|Character")
	float GetHealth() const;
	UFUNCTION(BlueprintCallable, Category = "AAA|Character")
	float GetStamina() const;
	UFUNCTION(BlueprintCallable, Category = "AAA|Character")
	float GetPopularSupport() const;

	// ── IDamageable (bridge a GAS) ─────────────────────────────────────────
	virtual int32 GetVida() const override;
	virtual int32 GetVidaMax() const override;
	virtual bool  EstaMuerto() const override;
	virtual void  RecibirDano(int32 Cantidad, FVector Origen, ETipoDano Tipo) override;
	virtual void  Curar(int32 Cantidad) override;

	// ── Locomoción para AnimInstance ───────────────────────────────────────
	UFUNCTION(BlueprintPure, Category = "AAA|Locomotion")
	float GetSpeed2D() const;

	UFUNCTION(BlueprintPure, Category = "AAA|Locomotion")
	float GetMovementDirection() const;

	UFUNCTION(BlueprintPure, Category = "AAA|Locomotion")
	EMovementGait GetMovementGait() const;

	UFUNCTION(BlueprintPure, Category = "AAA|Locomotion")
	bool IsRunning() const;

	UFUNCTION(BlueprintPure, Category = "AAA|Locomotion")
	bool IsCrouchingState() const;

	UFUNCTION(BlueprintPure, Category = "AAA|Locomotion")
	bool CanVault() const;

	UFUNCTION(BlueprintPure, Category = "AAA|Locomotion")
	float GetAimOffsetYaw() const;

	UFUNCTION(BlueprintPure, Category = "AAA|Locomotion")
	float GetAimOffsetPitch() const;

	UFUNCTION(BlueprintPure, Category = "AAA|Locomotion")
	float GetAimYawRate() const;

	UCharacterTrajectoryComponent* GetCharacterTrajectory() const;

	// ── Apuntado (ADS) ────────────────────────────────────────────────────
	UFUNCTION(BlueprintPure, Category = "AAA|Apuntado")
	bool EstaApuntando() const { return bApuntando; }

	UFUNCTION(BlueprintCallable, Category = "AAA|Apuntado")
	void ApuntarInicio();

	UFUNCTION(BlueprintCallable, Category = "AAA|Apuntado")
	void ApuntarFin();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// ── Enhanced Input (runtime fallback) ──────────────────────────────────
	void AsegurarInputRuntime();
	bool bInputRuntimeHecho = false;

	void EntradaMover(const FInputActionValue& V);
	void EntradaMirar(const FInputActionValue& V);
	void CorrerInicio();
	void CorrerFin();
	void AgacharseToggle();
	void SaltarOTrepar();
	bool IntentarTrepar();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputMappingContext* ContextoMapeo = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* IA_Mover = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* IA_Mirar = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* IA_Saltar = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* IA_Correr = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* IA_Agacharse = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* IA_Apuntar = nullptr;

	// ── Fallback clásico ───────────────────────────────────────────────────
	void MoveForward(float Value);
	void MoveRight(float Value);
	void TurnAt(float Value);
	void LookUpAt(float Value);

	// ── Velocidades (cm/s) ─────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movimiento")
	float VelCaminar = 400.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movimiento")
	float VelCorrer  = 650.f;
	bool bCorriendo = false;

private:
	// ── Cámara ─────────────────────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	// ── GAS ────────────────────────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = GAS, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAlsasuaAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAlsasuaAttributeSet> AttributeSet;

	void InitializeGAS();

	// ── ADS ────────────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category="Apuntado")
	float FOVCadera = 90.f;
	UPROPERTY(EditAnywhere, Category="Apuntado")
	float FOVApuntar = 55.f;
	UPROPERTY(EditAnywhere, Category="Apuntado")
	float BrazoCadera = 350.f;
	UPROPERTY(EditAnywhere, Category="Apuntado")
	float BrazoApuntar = 160.f;
	UPROPERTY(EditAnywhere, Category="Apuntado")
	float FOVCorrer = 100.f;
	UPROPERTY(EditAnywhere, Category="Apuntado")
	float BrazoCorrer = 400.f;
	bool bApuntando = false;

	// ── Parkour ────────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category="Parkour")
	float AlturaTrepaMax = 220.f;
	UPROPERTY(EditAnywhere, Category="Parkour")
	float AlcanceTrepa = 120.f;
	bool bTrepando = false;
	FVector TrepaInicio = FVector::ZeroVector;
	FVector TrepaFin = FVector::ZeroVector;
	float TrepaT = 0.f;
	float TrepaDur = 0.4f;
};
