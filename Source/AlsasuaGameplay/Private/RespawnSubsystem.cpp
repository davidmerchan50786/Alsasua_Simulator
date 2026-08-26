// RespawnSubsystem.cpp
#include "RespawnSubsystem.h"
#include "AlsasuaTypes.h"
#include "AlsasuaPlayerCharacter.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

void URespawnSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	AAlsasuaPlayerCharacter::OnPlayerDied.AddDynamic(this, &URespawnSubsystem::HandlePlayerDeath);
}

void URespawnSubsystem::HandlePlayerDeath()
{
	UWorld* W = GetWorld();
	if (!W) return;
	APawn* Jug = W->GetFirstPlayerController() ? W->GetFirstPlayerController()->GetPawn() : nullptr;
	if (!Jug) return;
	Reaparecer(Jug);
}

bool URespawnSubsystem::Reaparecer(APawn* Jugador) const
{
	if (!bTienePunto || !Jugador) return false;
	Jugador->SetActorLocation(Punto + FVector(0, 0, 120.f));
	if (IDamageable* D = Cast<IDamageable>(Jugador))
		D->Curar(D->GetVidaMax());
	return true;
}
