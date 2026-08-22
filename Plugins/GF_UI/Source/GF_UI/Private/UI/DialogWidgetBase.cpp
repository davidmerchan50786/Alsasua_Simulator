#include "UI/DialogWidgetBase.h"
#include "Systems/Dialog/DialogInstance.h"

void UDialogWidgetBase::BindToInstance(UDialogInstance* Instance) {
    CurrentInstance = Instance;
    if(CurrentInstance) {
        CurrentInstance->OnNodeReached.AddDynamic(this, &UDialogWidgetBase::Internal_OnNodeReached);
        FDialogNode Node = CurrentInstance->GetCurrentNode();
        Internal_OnNodeReached(Node);
    }
}

void UDialogWidgetBase::Internal_OnNodeReached(FDialogNode Node) {
    OnUpdateNPCText(Node.DialogueText, Node.VoiceOver.LoadSynchronous());
    OnUpdateOptions(Node.Options);
}
