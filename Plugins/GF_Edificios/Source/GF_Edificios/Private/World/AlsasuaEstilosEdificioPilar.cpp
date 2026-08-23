#include "World/AlsasuaEstilosEdificioPilar.h"
#include "EdificioGenerado.h"
#include "World/AlsasuaBarrioStyleSystem.h"
#include "World/AlsasuaBuildingEmissiveComponent.h"
#include "EngineUtils.h"


int32 UAlsasuaEstilosEdificioPilar::EjecutarArranque()
{
	int32 StyleCount = 0, EmissiveCount = 0;
	UWorld* W = GetWorld();
	for (TActorIterator<AEdificioGenerado> It(W); It; ++It)
	{
		AEdificioGenerado* Edificio = *It;
		if (!Edificio) continue;

		if (UAlsasuaBarrioStyleSystem* Style = NewObject<UAlsasuaBarrioStyleSystem>(Edificio))
		{
			// El barrio se lo pasa el tronco aqui: el componente no puede ver
			// a AEdificioGenerado y sacaba el estilo de Owner->GetName(), que
			// deja los ocho barrios iguales.
			Style->Barrio = Edificio->Barrio;
			Style->RegisterComponent();
			++StyleCount;
		}
		if (UAlsasuaBuildingEmissiveComponent* Emissive =
				NewObject<UAlsasuaBuildingEmissiveComponent>(Edificio))
		{
			Emissive->RegisterComponent();
			++EmissiveCount;
		}
	}
	return StyleCount + EmissiveCount;
}
