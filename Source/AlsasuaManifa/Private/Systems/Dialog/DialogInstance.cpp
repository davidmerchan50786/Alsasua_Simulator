#include "Systems/Dialog/DialogInstance.h"
#include "Systems/Dialog/DialogAsset.h"

void UDialogInstance::Init(UDialogAsset* Asset, AActor* InParticipant) {
    SourceAsset = Asset;
    Participant = InParticipant;
    CurrentNodeID = 0; // Asumimos que el nodo 0 es el inicio
}

FDialogNode UDialogInstance::GetCurrentNode() const {
    return FDialogNode();
}

void UDialogInstance::SelectOption(int32 OptionIndex) {
    FDialogNode Node = GetCurrentNode();
    if(Node.Options.IsValidIndex(OptionIndex)) {
        CurrentNodeID = Node.Options[OptionIndex].TargetNodeID;
        OnNodeReached.Broadcast(GetCurrentNode());
    }
}
