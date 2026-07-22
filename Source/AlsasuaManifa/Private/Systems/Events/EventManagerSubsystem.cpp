#include "Systems/Events/EventManagerSubsystem.h"
#include "Systems/Social/SocialMediaSubsystem.h"
#include "Systems/Urban/UrbanStateSubsystem.h"
#include "Systems/Forensics/EvidenceComponent.h"
#include "Kismet/GameplayStatics.h"

void UEventManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
    Super::Initialize(Collection);
}

void UEventManagerSubsystem::TickDirector(float DeltaTime) {
    CheckTimer += DeltaTime;
    if (CheckTimer >= CheckInterval) {
        CheckTimer = 0.f;
        EvaluateWorldState();
    }
}

void UEventManagerSubsystem::EvaluateWorldState() {
    // ... lógica previa ...
}

// Nueva función de hook para cuando la policía recoge pruebas
void UEventManagerSubsystem::HandleEvidenceCollected(AActor* Owner, FName Tag) {
    if (UWorld* World = GetWorld()) {
        if (UUrbanStateSubsystem* UrbanSS = World->GetSubsystem<UUrbanStateSubsystem>()) {
            UrbanSS->IncreaseTension("Global", 15.0f);
            OnDirectorAction.Broadcast(FText::FromString("¡ALERTA! La Guardia Civil ha recuperado pruebas en la escena. El nivel de búsqueda ha subido."));
        }
    }
}
