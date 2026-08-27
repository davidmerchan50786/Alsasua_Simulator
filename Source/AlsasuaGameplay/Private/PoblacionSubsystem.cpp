// PoblacionSubsystem.cpp
#include "PoblacionSubsystem.h"
#include "PeatonActor.h"
#include "WantedSubsystem.h"
#include "DiaNocheSubsystem.h"
#include "ApoyoPopularSubsystem.h"
#include "ArranqueMundo.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "AIController.h"

FOnNoiseAtLocation UPoblacionSubsystem::OnLoudNoise;

void UPoblacionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (UGameInstance* GI = GetGameInstance())
		if (UWantedSubsystem* Wn = GI->GetSubsystem<UWantedSubsystem>())
			Wn->OnEstrellasCambia.AddDynamic(this, &UPoblacionSubsystem::OnWantedChange);

	OnLoudNoise.AddDynamic(this, &UPoblacionSubsystem::HandleLoudNoise);
}

void UPoblacionSubsystem::HandleLoudNoise(FVector Location)
{
	if (!GetWorld()) return;
	APawn* Jug = GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr;
	const float Dist = Jug ? FVector::Dist(Jug->GetActorLocation(), Location) : 0.f;
	if (Dist < 2000.f)
		HuirDe(Location);

	// Witness: nearby civilian calls police. Chance scales with apoyo.
	// Low apoyo (0) = 60% chance. High apoyo (100) = 5% chance.
	if (Peatones.Num() > 0)
	{
		float BaseWitnessChance = 0.3f;
		if (UGameInstance* GI = GetGameInstance())
			if (UDiaNocheSubsystem* DN = GI->GetSubsystem<UDiaNocheSubsystem>())
				if (DN->EsNoche()) BaseWitnessChance = 0.15f;

		// Scale with apoyo: at 0 apoyo, double the chance; at 100, halve it.
		float ApoyoMultiplier = 1.f;
		if (UGameInstance* GI = GetGameInstance())
			if (UApoyoPopularSubsystem* Apoyo = GI->GetSubsystem<UApoyoPopularSubsystem>())
				ApoyoMultiplier = FMath::Lerp(2.0f, 0.25f, Apoyo->Apoyo / 100.f);

		const float FinalChance = BaseWitnessChance * ApoyoMultiplier;
		if (FMath::FRand() < FinalChance)
			if (UGameInstance* GI = GetGameInstance())
				if (UWantedSubsystem* W = GI->GetSubsystem<UWantedSubsystem>())
					W->AumentarBusqueda(1);
	}
}

void UPoblacionSubsystem::OnWantedChange(int32 Nivel)
{
	bPanicMode = Nivel >= 2;
	if (bPanicMode) HuirDe(GetWorld() ? GetWorld()->GetFirstPlayerController()->GetPawn()->GetActorLocation() : FVector::ZeroVector);

	// High apoyo: civilians DON'T flee at wanted 2 (they stand with you).
	if (Nivel >= 2 && !bPanicMode)
	{
		if (UGameInstance* GI = GetGameInstance())
			if (UApoyoPopularSubsystem* Apoyo = GI->GetSubsystem<UApoyoPopularSubsystem>())
				if (Apoyo->Apoyo >= 70.f)
					bPanicMode = false; // Override flee at high support.
	}
}

void UPoblacionSubsystem::HuirDe(FVector Location)
{
	for (APeatonActor* Pe : Peatones)
	{
		if (!IsValid(Pe)) continue;

		// High apoyo: some civilians stay to help (don't flee).
		if (UGameInstance* GI = GetGameInstance())
			if (UApoyoPopularSubsystem* Apoyo = GI->GetSubsystem<UApoyoPopularSubsystem>())
				if (Apoyo->Apoyo >= 70.f && FMath::FRand() < 0.4f)
					continue; // 40% of civilians stay at high support.

		FVector Away = Pe->GetActorLocation() + (Pe->GetActorLocation() - Location).GetSafeNormal2D() * 3000.f;
		if (AAIController* AIC = Cast<AAIController>(Pe->GetController()))
			AIC->MoveToLocation(Away);
	}
}

void UPoblacionSubsystem::Tick(float DeltaTime)
{
	if (!ArranqueMundo::BaselineListo) return;   // espera al mundo mínimo
	Acum += DeltaTime;
	if (Acum < PeriodoMantenimiento) return;
	Acum = 0.f;
	Mantener();
}

bool UPoblacionSubsystem::PuntoEnAnillo(const FVector& Centro, FVector& Out) const
{
	UWorld* W = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	UNavigationSystemV1* Nav = W ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(W) : nullptr;
	if (!Nav) return false;

	const float Ang = FMath::FRandRange(0.f, 2.f * PI);
	const float Dist = FMath::FRandRange(RadioMin, RadioMax);
	const FVector Cand = Centro + FVector(FMath::Cos(Ang) * Dist, FMath::Sin(Ang) * Dist, 0.f);

	FNavLocation Loc;
	if (Nav->ProjectPointToNavigation(Cand, Loc, FVector(200.f, 200.f, 1000.f)))
	{ Out = Loc.Location; return true; }
	return false;
}

void UPoblacionSubsystem::Mantener()
{
	UWorld* W = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!W) return;
	APawn* Jug = UGameplayStatics::GetPlayerPawn(W, 0);
	if (!Jug) return;
	const FVector P = Jug->GetActorLocation();

	// Limpia nulos/muertos y recicla los lejanos.
	for (int32 i = Peatones.Num() - 1; i >= 0; --i)
	{
		APeatonActor* Pe = Peatones[i];
		if (!IsValid(Pe) || FVector::Dist(P, Pe->GetActorLocation()) > RadioCull)
		{
			if (IsValid(Pe)) Pe->Destroy();
			Peatones.RemoveAtSwap(i);
		}
	}

	// Panic mode: stop spawning, existing peds flee.
	if (bPanicMode) return;

	// Night: fewer civilians on the streets (60% of max).
	int32 EffectiveMax = MaxPeatones;
	if (UGameInstance* GI = GetGameInstance())
		if (UDiaNocheSubsystem* DN = GI->GetSubsystem<UDiaNocheSubsystem>())
			if (DN->EsNoche()) EffectiveMax = MaxPeatones * 60 / 100;

	// Rellena hasta el máximo, con presupuesto por tick.
	int32 spawns = 0;
	while (Peatones.Num() < EffectiveMax && spawns < SpawnsPorTick)
	{
		FVector Punto;
		if (!PuntoEnAnillo(P, Punto)) break;

		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		APeatonActor* Pe = W->SpawnActor<APeatonActor>(
			APeatonActor::StaticClass(), Punto + FVector(0, 0, 90.f), FRotator(0, FMath::FRandRange(0.f, 360.f), 0), SP);
		if (Pe) Peatones.Add(Pe);
		++spawns;
	}
}
