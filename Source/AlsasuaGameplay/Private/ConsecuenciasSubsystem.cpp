// ConsecuenciasSubsystem.cpp
#include "ConsecuenciasSubsystem.h"
#include "ApoyoPopularSubsystem.h"
#include "ParanoiaVisualSubsystem.h"
#include "EconomiaSubsystem.h"
#include "AlsasuaNPC.h"

void UConsecuenciasSubsystem::RegistrarDano(AActor* Victima)
{
	AAlsasuaNPC* NPC = Cast<AAlsasuaNPC>(Victima);
	if (!NPC || !NPC->EstaMuerto()) return;   // sólo muertes, no cada impacto

	const uint32 Id = NPC->GetUniqueID();
	if (MuertesContadas.Contains(Id)) return;   // una consecuencia por víctima
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
		// Register victim location for paranoia hallucination system.
		if (UParanoiaVisualSubsystem* PV = GI->GetSubsystem<UParanoiaVisualSubsystem>())
		{
			PV->RegistrarVictimaCivil(NPC->GetActorLocation(), NPC->bEsPolicia);
		}
	}
}
