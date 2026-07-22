#include "Systems/Dialog/DialogSubsystem.h"
#include "Systems/Dialog/DialogInstance.h"
#include "Systems/Dialog/DialogAsset.h"

UDialogInstance* UDialogSubsystem::StartDialog(AActor* Instigator, UDialogAsset* DialogContent) {
    if(!DialogContent) return nullptr;
    UDialogInstance* NewInstance = NewObject<UDialogInstance>(this);
    NewInstance->Init(DialogContent, Instigator);
    return NewInstance;
}

bool UDialogSubsystem::LoadDialogFromJson(const FString& JsonPath, UDialogAsset* TargetAsset) {
    // Aquí iría el parseo de TJsonReader para llenar el DataAsset
    // Implementado como stub para el flujo
    return true;
}
