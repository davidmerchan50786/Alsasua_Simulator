// ConsecuenciasSubsystem.cpp
#include "ConsecuenciasSubsystem.h"
#include "ApoyoPopularSubsystem.h"
#include "ParanoiaVisualSubsystem.h"
#include "EconomiaSubsystem.h"
#include "AlsasuaNPC.h"
#include "AlsasuaCharacter.h"
#include "AlsasuaAttributeSet.h"
#include "Gameplay/Detention/DetentionMinigameComponent.h"
#include "Inventory/AlsasuaInventoryComponent.h"
#include "Kismet/GameplayStatics.h"

void UConsecuenciasSubsystem::RegistrarDano(AActor* Victima)
{
	AAlsasuaNPC* NPC = Cast<AAlsasuaNPC>(Victima);
	if (!NPC || !NPC->EstaMuerto()) return;

	const uint32 Id = NPC->GetUniqueID();
	if (MuertesContadas.Contains(Id)) return;
	MuertesContadas.Add(Id);

	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	if (NPC->bEsPolicia)
	{
		if (UEconomiaSubsystem* Ec = GI->GetSubsystem<UEconomiaSubsystem>())
			Ec->GanarDinero(RecompensaGuardia);
	}
	else
	{
		if (UApoyoPopularSubsystem* Ap = GI->GetSubsystem<UApoyoPopularSubsystem>())
		{
			Ap->RestarApoyo(6.f, TEXT("civil muerto"));
			Ap->SumarParanoia(8.f);
		}
		if (UParanoiaVisualSubsystem* PV = GI->GetSubsystem<UParanoiaVisualSubsystem>())
			PV->RegistrarVictimaCivil(NPC->GetActorLocation(), NPC->bEsPolicia);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  Detention consequences
// ─────────────────────────────────────────────────────────────────────────────
void UConsecuenciasSubsystem::Tick(float DeltaTime)
{
	if (bBoundToPlayer) return;
	UWorld* W = GetWorld();
	if (!W) return;
	APawn* Player = UGameplayStatics::GetPlayerPawn(W, 0);
	if (Player) BindToPlayerDetention(Player);
}

void UConsecuenciasSubsystem::BindToPlayerDetention(AActor* Player)
{
	if (!Player) return;
	if (UDetentionMinigameComponent* Det = Player->FindComponentByClass<UDetentionMinigameComponent>())
	{
		Det->OnDetentionResult.AddDynamic(this, &UConsecuenciasSubsystem::HandleDetentionResult);
		bBoundToPlayer = true;
	}
}

void UConsecuenciasSubsystem::HandleDetentionResult(bool bEscaped)
{
	UWorld* W = GetWorld();
	if (!W) return;
	APawn* Player = UGameplayStatics::GetPlayerPawn(W, 0);
	if (Player) AplicarConsecuenciasDetencion(Player, bEscaped);
}

void UConsecuenciasSubsystem::AplicarConsecuenciasDetencion(AActor* Jugador, bool bEscaped)
{
	if (!Jugador) return;
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	if (bEscaped)
	{
		// Escape: they know you resisted — small apoyo hit.
		if (UApoyoPopularSubsystem* Ap = GI->GetSubsystem<UApoyoPopularSubsystem>())
			Ap->RestarApoyo(EscapeApoyoLoss, TEXT("escape de detención"));
	}
	else
	{
		// Surrender: lose everything.
		if (UEconomiaSubsystem* Ec = GI->GetSubsystem<UEconomiaSubsystem>())
		{
			const int32 Loss = FMath::RoundToInt(Ec->Dinero * SurrenderCashLossPercent);
			if (Loss > 0) Ec->GastarDinero(Loss);
		}

		if (UAlsasuaInventoryComponent* Inv = Jugador->FindComponentByClass<UAlsasuaInventoryComponent>())
		{
			for (int32 i = 0; i < SurrenderItemsLost && Inv->Items.Num() > 0; ++i)
			{
				const int32 Idx = FMath::RandRange(0, Inv->Items.Num() - 1);
				Inv->RemoveItem(Inv->Items[Idx].ItemID, 1);
			}
		}

		if (UApoyoPopularSubsystem* Ap = GI->GetSubsystem<UApoyoPopularSubsystem>())
		{
			Ap->RestarApoyo(SurrenderApoyoLoss, TEXT("entregado a guardia"));
			Ap->SumarParanoia(SurrenderParanoiaGain);
		}

		if (AAlsasuaCharacter* Ch = Cast<AAlsasuaCharacter>(Jugador))
		{
			if (UAlsasuaAttributeSet* Attr = Ch->GetAttributeSet())
				Attr->SetWantedLevel(SurrenderWantedReset);
		}
	}
}
