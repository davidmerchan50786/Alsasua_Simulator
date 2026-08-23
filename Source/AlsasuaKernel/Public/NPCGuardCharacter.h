#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "NPCGuardCharacter.generated.h"

UCLASS()
class ALSASUAKERNEL_API ANPCGuardCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	ANPCGuardCharacter();
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** Nivel de sospecha/agresión (0-100). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float SuspicionLevel = 0.0f;

	/** Reducir agresión (ej: megáfono, disfraz). */
	UFUNCTION(BlueprintCallable, Category = "AI|Guard")
	void ReduceAggression(float Amount);

	/** Aumentar agresión (ej: ser atacado). */
	UFUNCTION(BlueprintCallable, Category = "AI|Guard")
	void IncreaseAggression(float Amount);

	/** ¿Está en modo combate? */
	UFUNCTION(BlueprintPure, Category = "AI|Guard")
	bool IsAggro() const { return SuspicionLevel > 70.f; }

	/** Intentar deescalar a un estado más bajo. */
	UFUNCTION(BlueprintCallable, Category = "AI|Guard")
	void TryDeescalate();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	class UAlsasuaAbilitySystemComponent* AbilitySystemComponent;

	/** Agresión máxima. */
	UPROPERTY(EditAnywhere, Category = "AI")
	float MaxAggression = 100.f;

	/** Tasa de deescalamiento pasivo por segundo. */
	UPROPERTY(EditAnywhere, Category = "AI")
	float PassiveDeescalateRate = 2.f;

	virtual void Tick(float DeltaTime) override;
};
