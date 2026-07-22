// ConsecuenciasSubsystem.cpp
#include "ConsecuenciasSubsystem.h"
#include "ApoyoPopularSubsystem.h"
#include "AlsasuaNPC.h"

void UConsecuenciasSubsystem::RegistrarDano(AActor* Victima)
{
	AAlsasuaNPC* NPC = Cast<AAlsasuaNPC>(Victima);
	if (!NPC || !NPC->EstaMuerto() || NPC->bEsPolicia) return;   // solo civiles muertos

	const uint32 Id = NPC->GetUniqueID();
	if (MuertesContadas.Contains(Id)) return;
	MuertesContadas.Add(Id);

	if (UGameInstance* GI = GetGameInstance())
		if (UApoyoPopularSubsystem* Ap = GI->GetSubsystem<UApoyoPopularSubsystem>())
		{
			Ap->RestarApoyo(6.f, TEXT("civil muerto"));
			Ap->SumarParanoia(8.f);
		}
}
