#include "World/AlsasuaNoiseComponent.h"
#include "AI/AlsasuaAIController.h"
#include "Kismet/GameplayStatics.h"

UAlsasuaNoiseComponent::UAlsasuaNoiseComponent() { PrimaryComponentTick.bCanEverTick = false; }

void UAlsasuaNoiseComponent::EmitNoise(float Intensity, float Radius)
{
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner) return;

	TArray<AActor*> Controllers;
	UGameplayStatics::GetAllActorsOfClass(World, AAlsasuaAIController::StaticClass(), Controllers);

	const FVector NoiseLocation = Owner->GetActorLocation();
	for (AActor* ControllerActor : Controllers)
	{
		AAlsasuaAIController* AIController = Cast<AAlsasuaAIController>(ControllerActor);
		if (AIController && AIController->GetPawn())
		{
			float Distance = FVector::Dist(NoiseLocation, AIController->GetPawn()->GetActorLocation());
			if (Distance <= Radius)
			{
				AIController->HandleNoiseEvent(NoiseLocation, Intensity);
			}
		}
	}
}
