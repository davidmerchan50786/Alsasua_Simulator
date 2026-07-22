#include "Effects/AlsasuaVisualGlitch.h"
#include "Components/SkeletalMeshComponent.h"
void UAlsasuaVisualGlitch::UpdateNPCParanoiaEffect(USkeletalMeshComponent* Mesh, float ParanoiaLevel) {
    if (Mesh) Mesh->SetScalarParameterValueOnMaterials(TEXT("ParanoiaIntensity"), ParanoiaLevel / 100.0f);
}
