#include "Systems/Criminal/HideoutActor.h"
#include "Components/StaticMeshComponent.h"
#include "Systems/Forensics/EvidenceComponent.h"

AHideoutActor::AHideoutActor() {
    RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(RootComp);
    EntranceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Entrance"));
    EntranceMesh->SetupAttachment(RootComp);

    // Añadir componente de evidencia automática al zulo
    EvidenceComp = CreateDefaultSubobject<UEvidenceComponent>(TEXT("ForensicEvidence"));
    EvidenceComp->EvidenceTag = "CriminalHideout";
}

void AHideoutActor::OpenZulo() {
    if(bIsLocked) {
        bIsLocked = false;
        OnZuloOpened();

        // Al abrirse, la evidencia se vuelve "fresca" o puede contaminarse si no se hace con cuidado
        if(EvidenceComp) {
            EvidenceComp->ContaminateEvidence(0.5f);
        }
    }
}
