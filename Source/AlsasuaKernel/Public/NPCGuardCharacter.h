#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Character/Stealth/GuardDetectionComponent.h"
#include "NPCGuardCharacter.generated.h"

UCLASS()
class ALSASUAKERNEL_API ANPCGuardCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	ANPCGuardCharacter();
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI")
	float SuspicionLevel = 0.f;

	UFUNCTION(BlueprintCallable, Category="AI|Guard")
	void ReduceAggression(float Amount);
	UFUNCTION(BlueprintCallable, Category="AI|Guard")
	void IncreaseAggression(float Amount);
	UFUNCTION(BlueprintPure, Category="AI|Guard")
	bool IsAggro() const { return SuspicionLevel > 70.f; }
	UFUNCTION(BlueprintCallable, Category="AI|Guard")
	void TryDeescalate();

	UPROPERTY(EditAnywhere, Category="AI|Patrol")
	float PatrolRadius = 2000.f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS")
	TObjectPtr<class UAlsasuaAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(EditAnywhere, Category="AI")
	float MaxAggression = 100.f;
	UPROPERTY(EditAnywhere, Category="AI")
	float PassiveDeescalateRate = 1.2f;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	UFUNCTION()
	void OnDetectionStateChanged(AActor* Guard, EGuardAlertState NewState, EGuardAlertState OldState);
	void Patrol();
	void Investigate(FVector Location);
	void Chase(FVector Location);
	void Attack();

	FVector SpawnLocation;
	bool bHasTarget = false;
	FVector CurrentTarget;
	float AttackCooldown = 0.f;
};
