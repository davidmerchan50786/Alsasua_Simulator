#include "World/AlsasuaLODConfigComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

UAlsasuaLODConfigComponent::UAlsasuaLODConfigComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
}

void UAlsasuaLODConfigComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UAlsasuaLODConfigComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bAutoUpdateLOD) return;

	TimeSinceLastUpdate += DeltaTime;
	if (TimeSinceLastUpdate < DistanceUpdateInterval) return;
	TimeSinceLastUpdate = 0.f;

	UpdateLODFromDistance();
}

void UAlsasuaLODConfigComponent::SetGlobalLODThresholds(const FLODThresholds& NewThresholds)
{
	Thresholds = NewThresholds;
}

ELODLevel UAlsasuaLODConfigComponent::GetCurrentLODLevel() const
{
	return CurrentLOD;
}

void UAlsasuaLODConfigComponent::SetScreenSizeOverride(float ScreenSize)
{
	const float AdjustedSize = ScreenSize + LODBias;

	if (AdjustedSize > 0.5f)
	{
		CurrentLOD = ELODLevel::High;
	}
	else if (AdjustedSize > 0.25f)
	{
		CurrentLOD = ELODLevel::Medium;
	}
	else if (AdjustedSize > 0.1f)
	{
		CurrentLOD = ELODLevel::Low;
	}
	else
	{
		CurrentLOD = ELODLevel::UltraLow;
	}
}

void UAlsasuaLODConfigComponent::UpdateLODFromDistance()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	UWorld* W = Owner->GetWorld();
	if (!W) return;

	APlayerController* PC = W->GetFirstPlayerController();
	if (!PC || !PC->GetPawn()) return;

	const float Distance = FVector::Dist(Owner->GetActorLocation(), PC->GetPawn()->GetActorLocation());
	LastDistanceToCamera = Distance;

	const float AdjustedDist = FMath::Max(0.f, Distance + LODBias * 1000.f);

	if (AdjustedDist < Thresholds.HighDistance)
	{
		CurrentLOD = ELODLevel::High;
	}
	else if (AdjustedDist < Thresholds.MediumDistance)
	{
		CurrentLOD = ELODLevel::Medium;
	}
	else if (AdjustedDist < Thresholds.LowDistance)
	{
		CurrentLOD = ELODLevel::Low;
	}
	else
	{
		CurrentLOD = ELODLevel::UltraLow;
	}
}

void UAlsasuaLODConfigComponent::ApplyGlobalNaniteSettings(bool bEnableNanite, int32 MaxPixelsPerEdge)
{
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Nanite.MaxPixelsPerEdge")))
	{
		CVar->Set(bEnableNanite ? FMath::Max(1, MaxPixelsPerEdge) : 16);
	}

	if (IConsoleVariable* CVarProxy = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Nanite.ProxyTriangleThreshold")))
	{
		CVarProxy->Set(bEnableNanite ? 500000 : 0);
	}

	UE_LOG(LogTemp, Log, TEXT("[LOD] Nanite %s (MaxPixels=%d)"), bEnableNanite ? TEXT("ON") : TEXT("OFF"), MaxPixelsPerEdge);
}

void UAlsasuaLODConfigComponent::ApplyGlobalHLODSettings(bool bEnableHLOD, float HLODScreenSize)
{
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.HLOD")))
	{
		CVar->Set(bEnableHLOD ? 1 : 0);
	}

	if (IConsoleVariable* CVarDist = IConsoleManager::Get().FindConsoleVariable(TEXT("r.HLODBoundingBoxScale")))
	{
		CVarDist->Set(HLODScreenSize);
	}

	UE_LOG(LogTemp, Log, TEXT("[LOD] HLOD %s (ScreenSize=%.2f)"), bEnableHLOD ? TEXT("ON") : TEXT("OFF"), HLODScreenSize);
}
