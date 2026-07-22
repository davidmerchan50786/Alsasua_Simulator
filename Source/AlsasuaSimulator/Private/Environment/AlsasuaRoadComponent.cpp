#include "Environment/AlsasuaRoadComponent.h"

UAlsasuaRoadComponent::UAlsasuaRoadComponent() {
    // Configuración por defecto para carreteras AAA
    SetCastShadow(true);
    SetAffectDistanceFieldLighting(true);
}

void UAlsasuaRoadComponent::SetupRVTMasking() {
    // RuntimeVirtualTexture no disponible en 5.4; stub para compatibilidad futura
}
