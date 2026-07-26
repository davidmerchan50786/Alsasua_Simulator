#include "AlsasuaOptimizerSubsystem.h"
#include "AlsasuaCharacter.h"
#include "Kismet/GameplayStatics.h"

void UAlsasuaOptimizerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UAlsasuaOptimizerSubsystem::Tick(float DeltaTime)
{
	OptimizationTickTimer += DeltaTime;
	NPCRefreshTimer += DeltaTime;

	if (OptimizationTickTimer >= 0.5f)
	{
		OptimizationTickTimer = 0.0f;

		AAlsasuaCharacter* Player = Cast<AAlsasuaCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
		if (Player)
		{
			// Refrescar cache de NPCs cada NPCRefreshInterval segundos.
			if (NPCRefreshTimer >= NPCRefreshInterval || CachedNPCs.Num() == 0)
			{
				NPCRefreshTimer = 0.0f;
				CachedNPCs.Empty();
				TArray<AActor*> TempActors;
				UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), TempActors);
				for (AActor* Actor : TempActors)
				{
					if (ACharacter* Char = Cast<ACharacter>(Actor))
					{
						CachedNPCs.Add(Char);
					}
				}
			}

			OptimizeCrowd(Player);
		}
	}
}

void UAlsasuaOptimizerSubsystem::OptimizeCrowd(AAlsasuaCharacter* Player)
{
	FVector PlayerLoc = Player->GetActorLocation();

	for (int32 i = CachedNPCs.Num() - 1; i >= 0; --i)
	{
		ACharacter* NPC = CachedNPCs[i];
		if (!IsValid(NPC) || NPC == Player)
		{
			CachedNPCs.RemoveAtSwap(i);
			continue;
		}

		const float Dist = FVector::DistSquared(PlayerLoc, NPC->GetActorLocation());
		const float CullDistSq = AICullDistance * AICullDistance;

		if (Dist > CullDistSq)
		{
			NPC->SetActorTickEnabled(false);
			if (NPC->GetController())
			{
				NPC->GetController()->SetActorTickEnabled(false);
			}
		}
		else
		{
			NPC->SetActorTickEnabled(true);
			if (NPC->GetController())
			{
				NPC->GetController()->SetActorTickEnabled(true);
			}
		}
	}
}
