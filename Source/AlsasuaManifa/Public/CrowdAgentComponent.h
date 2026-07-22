#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CrowdAgentComponent.generated.h"

// Estados de comportamiento del civil
UENUM(BlueprintType)
enum class ECrowdState : uint8
{
	Idle,
	FollowingProtest,
	Fleeing,
	Arrested
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UCrowdAgentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCrowdAgentComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Distancia a la que el civil empezarà a seguir al jugador si hay apoyo
	UPROPERTY(EditAnywhere, Category = "Crowd Configuration")
	float InfluenceRadius = 800.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Crowd State")
	ECrowdState CurrentState = ECrowdState::Idle;

	// Lógica principal: reacciona a los cambios en el mundo
	void UpdateAIBehavior();

private:
	// Referencia al jugador para seguirlo
	class AAlsasuaCharacter* CachedPlayer;
};
