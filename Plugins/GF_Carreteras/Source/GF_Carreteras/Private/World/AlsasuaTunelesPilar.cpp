#include "World/AlsasuaTunelesPilar.h"
#include "TunelAlsasua.h"
#include "Engine/World.h"


int32 UAlsasuaTunelesPilar::EjecutarArranque()
{
	ATunelAlsasua* Tuneles = GetWorld()->SpawnActor<ATunelAlsasua>(
		ATunelAlsasua::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	if (!Tuneles)
	{
		return -1;
	}
#if WITH_EDITOR
	Tuneles->SetActorLabel(TEXT("Alsasua_Tuneles"));
#endif
	return Tuneles->Construir();
}
