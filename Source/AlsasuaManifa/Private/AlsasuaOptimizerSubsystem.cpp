#include "AlsasuaOptimizerSubsystem.h"
#include "AlsasuaCharacter.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

void UAlsasuaOptimizerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UAlsasuaOptimizerSubsystem::Tick(float DeltaTime)
{
	OptimizationTickTimer += DeltaTime;
	if (OptimizationTickTimer >= 0.5f) // Optimizar 2 veces por segundo (ahorro de CPU)
	{
		OptimizationTickTimer = 0.0f;
		AAlsasuaCharacter* Player = Cast<AAlsasuaCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
		if (Player)
		{
			OptimizeCrowd(Player);
		}
	}
}

void UAlsasuaOptimizerSubsystem::OptimizeCrowd(AAlsasuaCharacter* Player)
{
	TArray<AActor*> AllNPCs;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), AllNPCs);

	FVector PlayerLoc = Player->GetActorLocation();

	for (AActor* Actor : AllNPCs)
	{
		if (Actor == Player) continue;

		float Dist = FVector::Dist(PlayerLoc, Actor->GetActorLocation());
		ACharacter* NPC = Cast<ACharacter>(Actor);

		if (Dist > AICullDistance)
		{
			// Si está muy lejos, desactivamos cerebro y colisiones complejas
			NPC->SetActorTickEnabled(false);
			if (NPC->GetController()) NPC->GetController()->SetActorTickEnabled(false);
		}
		else
		{
			// Reactivar si se acerca
			NPC->SetActorTickEnabled(true);
			if (NPC->GetController()) NPC->GetController()->SetActorTickEnabled(true);
		}
	}
}
