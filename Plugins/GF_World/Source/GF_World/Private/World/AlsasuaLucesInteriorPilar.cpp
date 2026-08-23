#include "World/AlsasuaLucesInteriorPilar.h"
#include "EdificioGenerado.h"
#include "World/AlsasuaInteriorLightComponent.h"
#include "EngineUtils.h"


int32 UAlsasuaLucesInteriorPilar::EjecutarArranque()
{
	int32 Count = 0;
	UWorld* W = GetWorld();
	for (TActorIterator<AEdificioGenerado> It(W); It; ++It)
	{
		AEdificioGenerado* Edificio = *It;
		if (!Edificio) continue;

		if (UAlsasuaInteriorLightComponent* Interior =
				NewObject<UAlsasuaInteriorLightComponent>(Edificio))
		{
			// Plantas viene del LiDAR (CargadorEdificios) y el ancho, de los
			// bounds de la malla ya construida. Sin estos datos el componente
			// creaba cuatro plantas de tres luces en TODOS los edificios.
			const FVector Extension = Edificio->GetComponentsBoundingBox(true).GetSize();
			Interior->Configurar(Edificio->Plantas,
				FMath::Min(Extension.X, Extension.Y),
				Edificio->Id);
			Interior->RegisterComponent();
			++Count;
		}
	}
	return Count;
}
