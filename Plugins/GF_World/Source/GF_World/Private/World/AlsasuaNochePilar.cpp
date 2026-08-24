#include "World/AlsasuaNochePilar.h"
#include "World/AlsasuaNightLightingSystem.h"
#include "GameFramework/WorldSettings.h"

int32 UAlsasuaNochePilar::EjecutarArranque()
{
	if (UAlsasuaNightLightingSystem* Noche =
			NewObject<UAlsasuaNightLightingSystem>(GetWorld()->GetWorldSettings()))
	{
		Noche->RegisterComponent();
		return 1;
	}
	return -1;
}
