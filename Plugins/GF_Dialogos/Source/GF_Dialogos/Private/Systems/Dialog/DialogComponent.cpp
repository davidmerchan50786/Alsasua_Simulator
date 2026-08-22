#include "Systems/Dialog/DialogComponent.h"
#include "Systems/Dialog/DialogSubsystem.h"
#include "Systems/Dialog/DialogAsset.h"

void UDialogComponent::Interact(AActor* Interactor) {
    if(UWorld* World = GetWorld()) {
        if(UDialogSubsystem* DSS = World->GetSubsystem<UDialogSubsystem>()) {
            DSS->StartDialog(Interactor, MainDialog);
        }
    }
}
