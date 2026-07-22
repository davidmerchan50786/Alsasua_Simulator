// GuardadoSubsystem.cpp
#include "GuardadoSubsystem.h"
#include "AlsasuaLegacySaveGame.h"
#include "EconomiaSubsystem.h"
#include "ApoyoPopularSubsystem.h"
#include "WantedSubsystem.h"
#include "DiaNocheSubsystem.h"
#include "MisionesSubsystem.h"
#include "RespawnSubsystem.h"
#include "AlsasuaPlayerCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

bool UGuardadoSubsystem::ExisteGuardado(int32 Slot) const
{
	return UGameplayStatics::DoesSaveGameExist(NombreSlot(Slot), 0);
}

bool UGuardadoSubsystem::GuardarEnSlot(int32 Slot)
{
	UGameInstance* GI = GetGameInstance();
	UWorld* W = GI ? GI->GetWorld() : nullptr;
	if (!GI || !W) return false;

	UAlsasuaLegacySaveGame* S = Cast<UAlsasuaLegacySaveGame>(UGameplayStatics::CreateSaveGameObject(UAlsasuaLegacySaveGame::StaticClass()));
	if (!S) return false;
	S->bValido = true;
	S->Fecha = FDateTime::Now();

	if (const UEconomiaSubsystem* Eco = GI->GetSubsystem<UEconomiaSubsystem>())   { S->Dinero = Eco->Dinero; S->Puntuacion = Eco->Puntuacion; }
	if (const UApoyoPopularSubsystem* Ap = GI->GetSubsystem<UApoyoPopularSubsystem>()) S->Apoyo = Ap->Apoyo;
	if (const UWantedSubsystem* Wn = GI->GetSubsystem<UWantedSubsystem>())         S->NivelBusqueda = Wn->NivelBusqueda;
	if (const UDiaNocheSubsystem* Dn = GI->GetSubsystem<UDiaNocheSubsystem>())     S->Hora = Dn->Hora;
	if (const UMisionesSubsystem* Mi = GI->GetSubsystem<UMisionesSubsystem>())     S->MisionActual = Mi->MisionActualId();
	if (const URespawnSubsystem* Rs = GI->GetSubsystem<URespawnSubsystem>())       { S->bRespawn = Rs->bTienePunto; S->RespawnPos = Rs->Punto; }

	if (APawn* Jug = UGameplayStatics::GetPlayerPawn(W, 0))
	{
		S->PlayerPos = Jug->GetActorLocation();
		S->PlayerRot = Jug->GetActorRotation();
		if (const AAlsasuaPlayerCharacter* C = Cast<AAlsasuaPlayerCharacter>(Jug)) S->PlayerVida = C->Vida;
	}

	const bool ok = UGameplayStatics::SaveGameToSlot(S, NombreSlot(Slot), 0);
	UE_LOG(LogTemp, Log, TEXT("[Guardado] slot %d %s"), Slot, ok ? TEXT("guardado") : TEXT("FALLO"));
	return ok;
}

bool UGuardadoSubsystem::CargarDeSlot(int32 Slot)
{
	UGameInstance* GI = GetGameInstance();
	UWorld* W = GI ? GI->GetWorld() : nullptr;
	if (!GI || !W) return false;

	UAlsasuaLegacySaveGame* S = Cast<UAlsasuaLegacySaveGame>(UGameplayStatics::LoadGameFromSlot(NombreSlot(Slot), 0));
	if (!S || !S->bValido) { UE_LOG(LogTemp, Warning, TEXT("[Guardado] slot %d vacío"), Slot); return false; }

	if (UEconomiaSubsystem* Eco = GI->GetSubsystem<UEconomiaSubsystem>())   { Eco->Dinero = S->Dinero; Eco->Puntuacion = S->Puntuacion; }
	if (UApoyoPopularSubsystem* Ap = GI->GetSubsystem<UApoyoPopularSubsystem>()) Ap->Apoyo = S->Apoyo;
	if (UWantedSubsystem* Wn = GI->GetSubsystem<UWantedSubsystem>())         Wn->NivelBusqueda = S->NivelBusqueda;
	if (UDiaNocheSubsystem* Dn = GI->GetSubsystem<UDiaNocheSubsystem>())     Dn->Hora = S->Hora;
	if (URespawnSubsystem* Rs = GI->GetSubsystem<URespawnSubsystem>())       { if (S->bRespawn) Rs->FijarPunto(S->RespawnPos); }

	if (APawn* Jug = UGameplayStatics::GetPlayerPawn(W, 0))
	{
		Jug->SetActorLocationAndRotation(S->PlayerPos, S->PlayerRot);
		if (AAlsasuaPlayerCharacter* C = Cast<AAlsasuaPlayerCharacter>(Jug)) C->Vida = S->PlayerVida;
	}

	// Reanuda la misión guardada.
	if (UMisionesSubsystem* Mi = GI->GetSubsystem<UMisionesSubsystem>())
		if (!S->MisionActual.IsNone()) Mi->IniciarMision(S->MisionActual);

	UE_LOG(LogTemp, Log, TEXT("[Guardado] slot %d cargado"), Slot);
	return true;
}
