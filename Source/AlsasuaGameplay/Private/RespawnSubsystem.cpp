// RespawnSubsystem.cpp
#include "RespawnSubsystem.h"
#include "AlsasuaTypes.h"   // IDamageable
#include "GameFramework/Pawn.h"

bool URespawnSubsystem::Reaparecer(APawn* Jugador) const
{
	if (!bTienePunto || !Jugador) return false;
	Jugador->SetActorLocation(Punto + FVector(0, 0, 120.f));
	if (IDamageable* D = Cast<IDamageable>(Jugador))
		D->Curar(D->GetVidaMax());   // reaparece con vida completa
	return true;
}
