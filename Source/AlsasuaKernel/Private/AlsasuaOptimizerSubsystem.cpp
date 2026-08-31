#include "AlsasuaOptimizerSubsystem.h"
#include "AlsasuaCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"

void UAlsasuaOptimizerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UAlsasuaOptimizerSubsystem::Tick(float DeltaTime)
{
	OptimizationTickTimer += DeltaTime;
	NPCRefreshTimer += DeltaTime;

	if (OptimizationTickTimer < 0.5f) return;
	OptimizationTickTimer = 0.0f;

	AAlsasuaCharacter* Player = Cast<AAlsasuaCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (!Player) return;

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

/**
 * Enciende o apaga TODO lo que tica de un NPC, no sólo el actor.
 *
 * SetActorTickEnabled(false) apaga la función Tick del actor y nada más: los
 * componentes tienen su propia función de tick y siguen corriendo. El caro es
 * UCharacterMovementComponent, que simula gravedad, rozamiento y navegación en
 * cada frame por cada peatón esté donde esté.
 *
 * Medido antes de tocar esto: con 600 peatones, Exclusive/GameThread/
 * CharacterMovement salía a 4,96 ms de mediana — el mayor coste real del hilo
 * de juego, más que TickActors (2,63) y Animation (0,69) juntos. El culling de
 * §8.3 llevaba corriendo todo ese tiempo y no tocaba lo único que importaba.
 */
void UAlsasuaOptimizerSubsystem::FijarTick(ACharacter* NPC, bool bActivo)
{
	NPC->SetActorTickEnabled(bActivo);

	if (AController* C = NPC->GetController())
	{
		C->SetActorTickEnabled(bActivo);
	}

	if (UCharacterMovementComponent* Mov = NPC->GetCharacterMovement())
	{
		Mov->SetComponentTickEnabled(bActivo);
	}

	// La malla esquelética se deja en paz a propósito: la animación sale a 0,69 ms
	// en la misma medida, y de ella ya se ocupa ApplyLOD con
	// VisibilityBasedAnimTickOption, que es la vía del motor para esto. Apagarle
	// el tick aquí pisaría esa decisión y congelaría también los montajes.
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

		const float Dist = FVector::Dist(PlayerLoc, NPC->GetActorLocation());

		// Tier 0: beyond cull distance → hide + disable tick
		if (Dist > AICullDistance)
		{
			FijarTick(NPC, false);
			ApplyLOD(NPC, 3);
			continue;
		}

		// Enable tick for all NPCs within cull distance
		FijarTick(NPC, true);

		// Determine LOD tier
		if (Dist > LOD3Distance)
			ApplyLOD(NPC, 3);
		else if (Dist > LOD2Distance)
			ApplyLOD(NPC, 2);
		else if (Dist > LOD1Distance)
			ApplyLOD(NPC, 1);
		else
			ApplyLOD(NPC, 0);
	}
}

void UAlsasuaOptimizerSubsystem::ApplyLOD(ACharacter* NPC, int32 LODLevel)
{
	USkeletalMeshComponent* Mesh = NPC ? NPC->GetMesh() : nullptr;
	if (!Mesh) return;

	// Avoid redundant state changes
	const int32 CurrentForced = Mesh->GetForcedLOD();
	const bool bCurrentShadow = Mesh->CastShadow;

	switch (LODLevel)
	{
	case 0: // Full quality: LOD0, shadows on
		if (CurrentForced != 0) Mesh->SetForcedLOD(0);
		if (!bCurrentShadow) Mesh->SetCastShadow(true);
		Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		break;

	case 1: // Medium: LOD1, shadows on, skip bones when not visible
		if (CurrentForced != 2) Mesh->SetForcedLOD(2);
		if (!bCurrentShadow) Mesh->SetCastShadow(true);
		Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		break;

	case 2: // Low: coarsest LOD, no shadows, minimal anim
		if (CurrentForced != Mesh->GetNumLODs()) Mesh->SetForcedLOD(Mesh->GetNumLODs());
		if (bCurrentShadow) Mesh->SetCastShadow(false);
		Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		break;

	case 3: // Hidden LOD: same as 2 but render disabled
		if (CurrentForced != Mesh->GetNumLODs()) Mesh->SetForcedLOD(Mesh->GetNumLODs());
		if (bCurrentShadow) Mesh->SetCastShadow(false);
		Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickMontagesWhenNotRendered;
		break;
	}
}
