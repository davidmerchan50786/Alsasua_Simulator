#include "World/AlsasuaArranqueLOD.h"
#include "World/AlsasuaLODConfigComponent.h"

int32 UAlsasuaArranqueLOD::EjecutarArranque()
{
	UAlsasuaLODConfigComponent::ApplyGlobalNaniteSettings(true, 1);
	UAlsasuaLODConfigComponent::ApplyGlobalHLODSettings(true, 0.05f);
	return 0;
}
