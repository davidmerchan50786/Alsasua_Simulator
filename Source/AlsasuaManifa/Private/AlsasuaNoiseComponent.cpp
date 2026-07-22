#include "AlsasuaNoiseComponent.h"
#include "AI/AlsasuaAIController.h"
#include "Kismet/GameplayStatics.h"

UAlsasuaNoiseComponent::UAlsasuaNoiseComponent() { PrimaryComponentTick.bCanEverTick = false; }

void UAlsasuaNoiseComponent::EmitNoise(float Intensity, float Radius)
{
	TArray<AActor*> Controllers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAlsasuaAIController::StaticClass(), Controllers);

	for (AActor* ControllerActor : Controllers)
	{
		AAlsasuaAIController* AIController = Cast<AAlsasuaAIController>(ControllerActor);
		if (AIController && AIController->GetPawn())
		{
			float Distance = FVector::Dist(GetOwner()->GetActorLocation(), AIController->GetPawn()->GetActorLocation());
			if (Distance <= Radius)
			{
				// Notificar a la IA del ruido (Lógica simplificada de percepción auditiva)
				AIController->HandleNoiseEvent(GetOwner()->GetActorLocation(), Intensity);
			}
		}
	}
}
