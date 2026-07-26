#include "Environment/AlsasuaRoadComponent.h"

UAlsasuaRoadComponent::UAlsasuaRoadComponent() {
    // Configuración por defecto para carreteras AAA
    SetCastShadow(true);
    SetAffectDistanceFieldLighting(true);
}

void UAlsasuaRoadComponent::SetupRVTMasking() {
    // RuntimeVirtualTexture support requires RVT streaming plugin.
    // Intended for future implementation when the world streaming system is stable.
    UE_LOG(LogTemp, Verbose, TEXT("AlsasuaRoadComponent: RVT masking not yet active."));
}
