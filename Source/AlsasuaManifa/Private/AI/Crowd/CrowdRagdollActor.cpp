#include "AI/Crowd/CrowdRagdollActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Optimization/AlsasuaActorPoolSubsystem.h"
#include "TimerManager.h"

ACrowdRagdollActor::ACrowdRagdollActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.033f;

	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		SkelMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
		SkelMesh->SetSimulatePhysics(true);
		SkelMesh->SetCollisionResponseToAllChannels(ECR_Block);
		SkelMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		SkelMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	}

	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->DisableMovement();
	}

	AutoPossessAI = EAutoPossessAI::Disabled;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
}

void ACrowdRagdollActor::BeginPlay()
{
	Super::BeginPlay();
}

// ═══════════════════════════════════════════════════════════════════════════
//  ActivateRagdoll: activa con el nivel de calidad indicado.
// ═══════════════════════════════════════════════════════════════════════════
void ACrowdRagdollActor::ActivateRagdoll(FVector ImpulseDir, float ImpulseStrength, ERagdollQuality Quality)
{
	CurrentQuality = Quality;

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(true);

	USkeletalMeshComponent* SkelMesh = GetMesh();
	if (!SkelMesh)
	{
		return;
	}

	SkelMesh->SetSimulatePhysics(true);
	SkelMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	SkelMesh->WakeAllRigidBodies();
	SkelMesh->SetAllBodiesSimulatePhysics(true);
	SkelMesh->SetAllPhysicsLinearVelocity(FVector::ZeroVector);

	const FVector FinalImpulse = ImpulseDir.GetSafeNormal() * ImpulseStrength * ImpulseMultiplier;
	SkelMesh->AddImpulse(FinalImpulse, NAME_None, true);

	const FVector RandomKick = FVector(
		FMath::RandRange(-200.f, 200.f),
		FMath::RandRange(-200.f, 200.f),
		FMath::RandRange(100.f, 300.f)
	);
	SkelMesh->AddImpulse(RandomKick, NAME_None, true);

	bIsFading = false;
	FadeAlpha = 1.f;
	FadeTimer = 0.f;

	if (UWorld* World = GetWorld())
	{
		FTimerManager& TM = World->GetTimerManager();

		TM.ClearTimer(PhysicsTimerHandle);
		TM.ClearTimer(FadeTimerHandle);
		TM.ClearTimer(ReturnTimerHandle);

		switch (Quality)
		{
		case ERagdollQuality::Full:
			TM.SetTimer(PhysicsTimerHandle, this,
				&ACrowdRagdollActor::OnPhysicsExpired, FullRagdollDuration, false);
			break;

		case ERagdollQuality::Frozen:
			PrimaryActorTick.bCanEverTick = true;
			TM.SetTimer(PhysicsTimerHandle, this,
				&ACrowdRagdollActor::FreezePhysics, FrozenImpulseTime, false);
			break;

		default:
			DeactivateForPool();
			break;
		}
	}
}

// ═══════════════════════════════════════════════════════════════════════════
//  DeactivateForPool: resetea el actor para devolverlo al pool.
// ═══════════════════════════════════════════════════════════════════════════
void ACrowdRagdollActor::DeactivateForPool()
{
	if (UWorld* World = GetWorld())
	{
		FTimerManager& TM = World->GetTimerManager();
		TM.ClearTimer(PhysicsTimerHandle);
		TM.ClearTimer(FadeTimerHandle);
		TM.ClearTimer(ReturnTimerHandle);
	}

	USkeletalMeshComponent* SkelMesh = GetMesh();
	if (SkelMesh)
	{
		SkelMesh->SetSimulatePhysics(false);
		SkelMesh->SetAllBodiesSimulatePhysics(false);
		SkelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SkelMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
		SkelMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

		TArray<UMaterialInterface*> Materials;
		for (int32 i = 0; i < SkelMesh->GetNumMaterials(); ++i)
		{
			Materials.Add(SkelMesh->GetMaterial(i));
		}
		for (UMaterialInterface* Mat : Materials)
		{
			if (UMaterialInstanceDynamic* DynMat = Cast<UMaterialInstanceDynamic>(Mat))
			{
				DynMat->SetScalarParameterValue(FName("Opacity"), 1.f);
			}
		}
	}

	CurrentQuality = ERagdollQuality::None;
	bIsFading = false;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
	SetActorLocation(FVector(0, 0, -10000));
}

// ═══════════════════════════════════════════════════════════════════════════
//  OnPhysicsExpired: cuando termina la física Full, inicia fade-out.
// ═══════════════════════════════════════════════════════════════════════════
void ACrowdRagdollActor::OnPhysicsExpired()
{
	FreezePhysics();
}

// ═══════════════════════════════════════════════════════════════════════════
//  FreezePhysics: congela la pose actual y arranca fade-out.
// ═══════════════════════════════════════════════════════════════════════════
void ACrowdRagdollActor::FreezePhysics()
{
	USkeletalMeshComponent* SkelMesh = GetMesh();
	if (SkelMesh)
	{
		SkelMesh->SetSimulatePhysics(false);
		SkelMesh->SetAllBodiesSimulatePhysics(false);
		SkelMesh->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
		SkelMesh->SetAllPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}

	StartFadeOut();
}

// ═══════════════════════════════════════════════════════════════════════════
//  StartFadeOut: inicia el fade-out visual antes de devolver al pool.
// ═══════════════════════════════════════════════════════════════════════════
void ACrowdRagdollActor::StartFadeOut()
{
	bIsFading = true;
	FadeAlpha = 1.f;
	FadeTimer = 0.f;

	// Habilitar tick solo para el fade-out (más barato que timer 30Hz).
	SetActorTickEnabled(true);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ReturnTimerHandle, this,
			&ACrowdRagdollActor::OnReturnToPool, FadeOutDuration + 0.1f, false);
	}
}

// ═══════════════════════════════════════════════════════════════════════════
//  Tick: solo ejecuta fade-out visual.
// ═══════════════════════════════════════════════════════════════════════════
void ACrowdRagdollActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsFading)
	{
		return;
	}

	FadeTimer += DeltaTime;
	FadeAlpha = 1.f - FMath::Clamp(FadeTimer / FadeOutDuration, 0.f, 1.f);

	USkeletalMeshComponent* SkelMesh = GetMesh();
	if (SkelMesh)
	{
		for (int32 i = 0; i < SkelMesh->GetNumMaterials(); ++i)
		{
			if (UMaterialInstanceDynamic* DynMat = Cast<UMaterialInstanceDynamic>(SkelMesh->GetMaterial(i)))
			{
				DynMat->SetScalarParameterValue(FName("Opacity"), FadeAlpha);
			}
		}

		if (FadeAlpha <= 0.01f)
		{
			bIsFading = false;
			OnReturnToPool();
		}
	}
}

// ═══════════════════════════════════════════════════════════════════════════
//  OnReturnToPool: devuelve el actor al pool.
// ═══════════════════════════════════════════════════════════════════════════
void ACrowdRagdollActor::OnReturnToPool()
{
	bIsFading = false;

	UWorld* W = GetWorld();
	if (!W)
	{
		DeactivateForPool();
		return;
	}
	UAlsasuaActorPoolSubsystem* Pool = W->GetSubsystem<UAlsasuaActorPoolSubsystem>();
	if (Pool)
	{
		Pool->ReleaseActor(this);
	}
	else
	{
		DeactivateForPool();
	}
}
