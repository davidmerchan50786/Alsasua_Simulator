#include "Gameplay/Disguise/DisguiseComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInterface.h"

UDisguiseComponent::UDisguiseComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f; // 10Hz, no cada frame.

	MomotxorroConfig.DetectionMultiplier = 0.3f;
	MomotxorroConfig.MaxDurability = 100.f;
	MomotxorroConfig.WalkDrainPerSecond = 0.5f;
	MomotxorroConfig.SprintDrainPerSecond = 3.5f;
	MomotxorroConfig.JumpCost = 2.0f;
	MomotxorroConfig.AttackCost = 20.f;
	MomotxorroConfig.NearGuardDrainPerSecond = 5.0f;
	MomotxorroConfig.NearGuardRadius = 350.f;
	MomotxorroConfig.NoiseReduction = 0.3f;

	CasualConfig.DetectionMultiplier = 0.7f;
	CasualConfig.MaxDurability = 120.f;
	CasualConfig.WalkDrainPerSecond = 0.2f;
	CasualConfig.SprintDrainPerSecond = 1.8f;
	CasualConfig.JumpCost = 1.0f;
	CasualConfig.AttackCost = 12.f;
	CasualConfig.NearGuardDrainPerSecond = 3.0f;
	CasualConfig.NearGuardRadius = 350.f;
	CasualConfig.NoiseReduction = 0.5f;

	PressConfig.DetectionMultiplier = 0.5f;
	PressConfig.MaxDurability = 80.f;
	PressConfig.WalkDrainPerSecond = 0.3f;
	PressConfig.SprintDrainPerSecond = 2.0f;
	PressConfig.JumpCost = 1.5f;
	PressConfig.AttackCost = 25.f;
	PressConfig.NearGuardDrainPerSecond = 4.5f;
	PressConfig.NearGuardRadius = 500.f;
	PressConfig.NoiseReduction = 0.6f;
}

void UDisguiseComponent::BeginPlay()
{
	Super::BeginPlay();
}

// ═══════════════════════════════════════════════════════════════════════════
//  Tick: drenaje pasivo de durabilidad y chequeo de rotura.
// ═══════════════════════════════════════════════════════════════════════════
void UDisguiseComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CurrentDisguise == EDisguiseType::None)
	{
		return;
	}

	TickPassiveDrain(DeltaTime);
	TickDurabilityCheck();
}

// ═══════════════════════════════════════════════════════════════════════════
//  EquipDisguise
// ═══════════════════════════════════════════════════════════════════════════
void UDisguiseComponent::EquipDisguise(EDisguiseType Type, bool bConsumableIn, float InitialDurability)
{
	if (Type == EDisguiseType::None)
	{
		UnequipDisguise();
		return;
	}

	const EDisguiseType OldType = CurrentDisguise;
	CurrentDisguise = Type;
	bIsConsumable = bConsumableIn;
	bIsSprinting = false;
	bNearGuard = false;

	const FDisguiseTypeInfo& Cfg = GetConfigForType(Type);
	Durability = (InitialDurability > 0.f) ? FMath::Min(InitialDurability, Cfg.MaxDurability) : Cfg.MaxDurability;

	AplicarTinte(1);
	OnDisguiseChanged.Broadcast(Type, OldType);
	OnDurabilityChanged.Broadcast(Durability);
}

// ═══════════════════════════════════════════════════════════════════════════
//  UnequipDisguise
// ═══════════════════════════════════════════════════════════════════════════
void UDisguiseComponent::UnequipDisguise()
{
	if (CurrentDisguise == EDisguiseType::None)
	{
		return;
	}

	const EDisguiseType OldType = CurrentDisguise;
	CurrentDisguise = EDisguiseType::None;
	Durability = 0.f;
	bIsSprinting = false;
	bNearGuard = false;

	AplicarTinte(0);
	OnDisguiseChanged.Broadcast(EDisguiseType::None, OldType);
}

// ═══════════════════════════════════════════════════════════════════════════
//  UseDisguise: consumo directo de durabilidad.
// ═══════════════════════════════════════════════════════════════════════════
void UDisguiseComponent::UseDisguise(float Amount)
{
	if (CurrentDisguise == EDisguiseType::None)
	{
		return;
	}

	Durability = FMath::Max(0.f, Durability - Amount);
	OnDurabilityChanged.Broadcast(Durability);

	if (Durability <= 0.f)
	{
		BreakDisguise();
	}
}

// ═══════════════════════════════════════════════════════════════════════════
//  Notificaciones de acción
// ═══════════════════════════════════════════════════════════════════════════
void UDisguiseComponent::NotifySprint(bool bIsSprintingIn)
{
	bIsSprinting = bIsSprintingIn;
}

void UDisguiseComponent::NotifyJump()
{
	if (CurrentDisguise == EDisguiseType::None) return;

	const FDisguiseTypeInfo& Cfg = GetConfigForType(CurrentDisguise);
	UseDisguise(Cfg.JumpCost);
}

void UDisguiseComponent::NotifyAttack()
{
	if (CurrentDisguise == EDisguiseType::None) return;

	const FDisguiseTypeInfo& Cfg = GetConfigForType(CurrentDisguise);
	UseDisguise(Cfg.AttackCost);
}

// ═══════════════════════════════════════════════════════════════════════════
//  UpdateNearbyGuards: actualiza proximidad a guardias.
// ═══════════════════════════════════════════════════════════════════════════
void UDisguiseComponent::UpdateNearbyGuards(const FVector& PlayerLocation, const TArray<FVector>& GuardLocations)
{
	if (CurrentDisguise == EDisguiseType::None)
	{
		bNearGuard = false;
		return;
	}

	const FDisguiseTypeInfo& Cfg = GetConfigForType(CurrentDisguise);
	const float RadiusSq = Cfg.NearGuardRadius * Cfg.NearGuardRadius;

	bNearGuard = false;
	for (const FVector& GuardLoc : GuardLocations)
	{
		if (FVector::DistSquared(PlayerLocation, GuardLoc) < RadiusSq)
		{
			bNearGuard = true;
			break;
		}
	}
}

// ═══════════════════════════════════════════════════════════════════════════
//  Queries
// ═══════════════════════════════════════════════════════════════════════════
float UDisguiseComponent::GetEffectiveDetectionMultiplier() const
{
	if (CurrentDisguise == EDisguiseType::None)
	{
		return 1.0f;
	}

	const FDisguiseTypeInfo& Cfg = GetConfigForType(CurrentDisguise);
	const float BaseMult = Cfg.DetectionMultiplier;

	// Degradar según durabilidad: a menor durabilidad, peor disimulo.
	const float DurabilityFactor = GetDurabilityPercent();

	// Interpolar entre 1.0 (sin disfraz) y BaseMult (disfraz perfecto).
	return FMath::Lerp(1.0f, BaseMult, DurabilityFactor);
}

float UDisguiseComponent::GetNoiseReduction() const
{
	if (CurrentDisguise == EDisguiseType::None)
	{
		return 1.0f;
	}

	const FDisguiseTypeInfo& Cfg = GetConfigForType(CurrentDisguise);
	return FMath::Lerp(1.0f, Cfg.NoiseReduction, GetDurabilityPercent());
}

float UDisguiseComponent::GetMaxDurability() const
{
	if (CurrentDisguise == EDisguiseType::None) return 0.f;
	return GetConfigForType(CurrentDisguise).MaxDurability;
}

float UDisguiseComponent::GetDurabilityPercent() const
{
	const float Max = GetMaxDurability();
	if (Max <= 0.f) return 0.f;
	return FMath::Clamp(Durability / Max, 0.f, 1.f);
}

// ═══════════════════════════════════════════════════════════════════════════
//  Visual: tiñe el mesh del jugador al encubrirse y lo restaura al quitarse.
// ═══════════════════════════════════════════════════════════════════════════
void UDisguiseComponent::AplicarTinte(uint8 bOn)
{
	const ACharacter* Char = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Mesh = Char ? Char->GetMesh() : nullptr;
	if (!Mesh) return;

	if (bOn)
	{
		if (OriginalMaterials.Num() == 0)
		{
			for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
				OriginalMaterials.Add(Mesh->GetMaterial(i));
		}
		for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
		{
			if (UMaterialInstanceDynamic* MID = Mesh->CreateAndSetMaterialInstanceDynamic(i))
			{
				MID->SetVectorParameterValue(FName("Tint"), TintColor);
				MID->SetVectorParameterValue(FName("BaseColor"), TintColor);
			}
		}
	}
	else
	{
		for (int32 i = 0; i < OriginalMaterials.Num() && i < Mesh->GetNumMaterials(); ++i)
			Mesh->SetMaterial(i, OriginalMaterials[i]);
		OriginalMaterials.Reset();
	}
}

// ═══════════════════════════════════════════════════════════════════════════
//  Config lookup
// ═══════════════════════════════════════════════════════════════════════════
const FDisguiseTypeInfo& UDisguiseComponent::GetConfigForType(EDisguiseType Type) const
{
	switch (Type)
	{
	case EDisguiseType::Momotxorro:       return MomotxorroConfig;
	case EDisguiseType::Casual_Infiltrator: return CasualConfig;
	case EDisguiseType::Press_Press:       return PressConfig;
	default:                               return CasualConfig;
	}
}

// ═══════════════════════════════════════════════════════════════════════════
//  Tick helpers
// ═══════════════════════════════════════════════════════════════════════════
void UDisguiseComponent::TickPassiveDrain(float DeltaTime)
{
	if (CurrentDisguise == EDisguiseType::None) return;

	const FDisguiseTypeInfo& Cfg = GetConfigForType(CurrentDisguise);

	// Drenaje base por caminar.
	float DrainRate = Cfg.WalkDrainPerSecond;

	// Drenaje extra por sprint.
	if (bIsSprinting)
	{
		DrainRate += Cfg.SprintDrainPerSecond;
	}

	// Drenaje extra por proximidad a guardias.
	if (bNearGuard)
	{
		DrainRate += Cfg.NearGuardDrainPerSecond;
	}

	if (DrainRate > 0.f)
	{
		UseDisguise(DrainRate * DeltaTime);
	}
}

void UDisguiseComponent::TickDurabilityCheck()
{
	if (CurrentDisguise == EDisguiseType::None) return;

	// Broadcast warn at low durability for HUD flashing etc.
	if (Durability <= GetMaxDurability() * 0.25f && Durability > 0.f)
	{
		OnDurabilityChanged.Broadcast(Durability);
	}
}

void UDisguiseComponent::BreakDisguise()
{
	const EDisguiseType BrokenType = CurrentDisguise;
	CurrentDisguise = EDisguiseType::None;
	Durability = 0.f;
	bIsSprinting = false;
	bNearGuard = false;

	AplicarTinte(0);
	OnDisguiseBroken.Broadcast(BrokenType);
	OnDisguiseChanged.Broadcast(EDisguiseType::None, BrokenType);
}
