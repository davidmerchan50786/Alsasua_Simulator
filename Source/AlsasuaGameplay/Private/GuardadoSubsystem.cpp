// GuardadoSubsystem.cpp
#include "GuardadoSubsystem.h"
#include "AlsasuaLegacySaveGame.h"
#include "EconomiaSubsystem.h"
#include "ApoyoPopularSubsystem.h"
#include "WantedSubsystem.h"
#include "DiaNocheSubsystem.h"
#include "MisionesSubsystem.h"
#include "RespawnSubsystem.h"
#include "Gameplay/Disguise/DisguiseComponent.h"
#include "AlsasuaCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

bool UAlsasuaLegacySaveGame::MigrateFromLegacyV1()
{
	if (SaveVersion >= ALSASUA_SAVE_VERSION) return false;

	UE_LOG(LogTemp, Log, TEXT("[Guardado] Migrando save de versión %d a %d"), SaveVersion, ALSASUA_SAVE_VERSION);

	if (SaveVersion < 2)
	{
		if (CompletedMissionIDs.Num() == 0 && !MisionActual.IsNone())
		{
			CompletedMissionIDs.Add(MisionActual.ToString());
		}
		if (SaveSlotName.IsEmpty())
		{
			SaveSlotName = TEXT("AlsasuaDefaultSlot");
		}
	}

	SaveVersion = ALSASUA_SAVE_VERSION;
	return true;
}

bool UGuardadoSubsystem::ExisteGuardado(int32 Slot) const
{
	return UGameplayStatics::DoesSaveGameExist(NombreSlot(Slot), 0);
}

bool UGuardadoSubsystem::GuardarEnSlot(int32 Slot)
{
	UGameInstance* GI = GetGameInstance();
	UWorld* W = GI ? GI->GetWorld() : nullptr;
	if (!GI || !W) return false;

	UAlsasuaLegacySaveGame* S = Cast<UAlsasuaLegacySaveGame>(
		UGameplayStatics::CreateSaveGameObject(UAlsasuaLegacySaveGame::StaticClass()));
	if (!S) return false;

	S->bValido = true;
	S->Fecha = FDateTime::Now();
	S->SaveVersion = ALSASUA_SAVE_VERSION;
	S->SaveSlotName = NombreSlot(Slot);
	S->UserIndex = 0;

	if (const UEconomiaSubsystem* Eco = GI->GetSubsystem<UEconomiaSubsystem>())
	{
		S->Dinero = Eco->Dinero;
		S->Puntuacion = Eco->Puntuacion;
	}
	if (const UApoyoPopularSubsystem* Ap = GI->GetSubsystem<UApoyoPopularSubsystem>())
	{
		S->Apoyo = Ap->Apoyo;
	}
	if (const UWantedSubsystem* Wn = GI->GetSubsystem<UWantedSubsystem>())
	{
		S->NivelBusqueda = Wn->NivelBusqueda;
	}
	if (const UDiaNocheSubsystem* Dn = GI->GetSubsystem<UDiaNocheSubsystem>())
	{
		S->Hora = Dn->Hora;
	}
	if (const UMisionesSubsystem* Mi = GI->GetSubsystem<UMisionesSubsystem>())
	{
		S->MisionActual = Mi->MisionActualId();
		// CompletedMissionIDs existía en el save y no lo escribía nadie: al cargar
		// el log decía "0 misiones completadas" con la campaña a medias, porque
		// el subsistema tampoco llevaba la cuenta. Ahora la lleva y se guarda.
		S->CompletedMissionIDs = Mi->IdsCompletadas();
	}

	if (const URespawnSubsystem* Rs = GI->GetSubsystem<URespawnSubsystem>())
	{
		S->bRespawn = Rs->bTienePunto;
		S->RespawnPos = Rs->Punto;
	}

	if (APawn* Jug = UGameplayStatics::GetPlayerPawn(W, 0))
	{
		S->PlayerPos = Jug->GetActorLocation();
		S->PlayerRot = Jug->GetActorRotation();
		if (AAlsasuaCharacter* C = Cast<AAlsasuaCharacter>(Jug))
		{
			S->PlayerVida = C->GetVida();
		}
		if (const UDisguiseComponent* Df = Jug->FindComponentByClass<UDisguiseComponent>())
			S->DisfrazType = Df->IsDisguised() ? 1 : 0;
		// DisfrazDurability se queda con su valor por defecto: no hay sistema de
		// durabilidad que lo alimente. Escribir aquí un 1.0f fijo sólo servía
		// para que pareciera que se guardaba algo. El porqué, en el header.
	}

	const bool bOk = UGameplayStatics::SaveGameToSlot(S, NombreSlot(Slot), 0);
	UE_LOG(LogTemp, Log, TEXT("[Guardado] Slot %d %s (v%d)"), Slot, bOk ? TEXT("guardado") : TEXT("FALLO"), ALSASUA_SAVE_VERSION);
	return bOk;
}

bool UGuardadoSubsystem::CargarDeSlot(int32 Slot)
{
	UGameInstance* GI = GetGameInstance();
	UWorld* W = GI ? GI->GetWorld() : nullptr;
	if (!GI || !W) return false;

	UAlsasuaLegacySaveGame* S = Cast<UAlsasuaLegacySaveGame>(
		UGameplayStatics::LoadGameFromSlot(NombreSlot(Slot), 0));
	if (!S || !S->bValido)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Guardado] Slot %d vacío o inválido"), Slot);
		return false;
	}

	if (S->MigrateFromLegacyV1())
	{
		UE_LOG(LogTemp, Log, TEXT("[Guardado] Save migrado a versión %d"), ALSASUA_SAVE_VERSION);
	}

	if (UEconomiaSubsystem* Eco = GI->GetSubsystem<UEconomiaSubsystem>())
	{
		Eco->Dinero = S->Dinero;
		Eco->Puntuacion = S->Puntuacion;
	}
	if (UApoyoPopularSubsystem* Ap = GI->GetSubsystem<UApoyoPopularSubsystem>())
	{
		Ap->Apoyo = S->Apoyo;
	}
	if (UWantedSubsystem* Wn = GI->GetSubsystem<UWantedSubsystem>())
	{
		Wn->NivelBusqueda = S->NivelBusqueda;
	}
	if (UDiaNocheSubsystem* Dn = GI->GetSubsystem<UDiaNocheSubsystem>())
	{
		Dn->Hora = S->Hora;
	}
	if (URespawnSubsystem* Rs = GI->GetSubsystem<URespawnSubsystem>())
	{
		if (S->bRespawn) Rs->FijarPunto(S->RespawnPos);
	}

	if (APawn* Jug = UGameplayStatics::GetPlayerPawn(W, 0))
	{
		Jug->SetActorLocationAndRotation(S->PlayerPos, S->PlayerRot);
		if (AAlsasuaCharacter* C = Cast<AAlsasuaCharacter>(Jug))
		{
			C->Curar(S->PlayerVida - C->GetVida());
		}
	}

	// El disfraz se guardaba y no se restauraba: quien salvara estando
	// encubierto reaparecía a cara descubierta, con la policía reconociéndole
	// otra vez. DisfrazType es 1 encubierto y 0 a cara descubierta, tal y como
	// lo escribe GuardarEnSlot.
	if (APawn* Jug = UGameplayStatics::GetPlayerPawn(W, 0))
		if (UDisguiseComponent* Df = Jug->FindComponentByClass<UDisguiseComponent>())
		{
			if (S->DisfrazType == 1)
				Df->EquipDisguise(EDisguiseType::Momotxorro);
			else
				Df->UnequipDisguise();
		}

	if (UMisionesSubsystem* Mi = GI->GetSubsystem<UMisionesSubsystem>())
	{
		// Las completadas primero: si no, la misión que se reanuda podría
		// encadenar con una que ya estaba hecha.
		Mi->RestaurarCompletadas(S->CompletedMissionIDs);
		if (!S->MisionActual.IsNone())
		{
			Mi->IniciarMision(S->MisionActual);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Guardado] Slot %d cargado (v%d, %d misiones completadas)"),
		Slot, S->SaveVersion, S->CompletedMissionIDs.Num());
	return true;
}
