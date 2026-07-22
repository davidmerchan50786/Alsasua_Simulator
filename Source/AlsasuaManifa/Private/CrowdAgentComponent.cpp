#include "CrowdAgentComponent.h"
#include "AlsasuaCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"

UCrowdAgentComponent::UCrowdAgentComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCrowdAgentComponent::BeginPlay()
{
	Super::BeginPlay();
	CachedPlayer = Cast<AAlsasuaCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn());
}

void UCrowdAgentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateAIBehavior();
}

void UCrowdAgentComponent::UpdateAIBehavior()
{
	if (!CachedPlayer || !GetOwner()) return;

	float DistanceToPlayer = FVector::Dist(GetOwner()->GetActorLocation(), CachedPlayer->GetActorLocation());
	float PopularSupport = CachedPlayer->GetPopularSupport();

	AAIController* AIController = Cast<AAIController>(Cast<APawn>(GetOwner())->GetController());
	if (!AIController) return;

	// Mecánica de Realismo: Si el apoyo popular es alto y el jugador está cerca, se unen a la protesta
	if (PopularSupport > 20.f && DistanceToPlayer < InfluenceRadius)
	{
		CurrentState = ECrowdState::FollowingProtest;
		AIController->MoveToActor(CachedPlayer, 150.f); // Mantener distancia de grupo
	}
	else if (CachedPlayer->GetStamina() < 5.f) // Si el jugador está exhausto o hay caos (simplificado aquí)
	{
		CurrentState = ECrowdState::Idle;
		AIController->StopMovement();
	}
}
