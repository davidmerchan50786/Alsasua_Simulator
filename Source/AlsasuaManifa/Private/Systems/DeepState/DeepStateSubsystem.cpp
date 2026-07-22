#include "Systems/DeepState/DeepStateSubsystem.h"
#include "Systems/Social/SocialMediaSubsystem.h"

void UDeepStateSubsystem::LaunchCovertOp(FName OpName, EDeepStateOp Type) {
    FDeepStateProject& NewOp = ActiveProjects.FindOrAdd(OpName);
    NewOp.ProjectName = OpName;
    NewOp.OperationType = Type;
    NewOp.bIsActive = true;

    FText WarningMsg;
    switch(Type) {
        case EDeepStateOp::Disinformation: WarningMsg = FText::FromString("CAMPAÑA DE DIFAMACIÓN: Bots sospechosos atacan tu perfil."); break;
        case EDeepStateOp::EvidencePlanting: WarningMsg = FText::FromString("FALSA BANDERA: Se están fabricando pruebas en tu contra."); break;
        case EDeepStateOp::AssetFreezing: WarningMsg = FText::FromString("BLOQUEO BANCARIO: Las cloacas rastrean tus fondos."); break;
        default: WarningMsg = FText::FromString("OPERACIÓN DESCONOCIDA DETECTADA."); break;
    }

    OnDeepStateAlert.Broadcast(WarningMsg, Type);
}

void UDeepStateSubsystem::CounterOperation(FName OpName, float SabotageAmount) {
    if (FDeepStateProject* Op = ActiveProjects.Find(OpName)) {
        Op->Progress -= SabotageAmount;
        if (Op->Progress <= 0.f) {
            Op->bIsActive = false;
            OnDeepStateAlert.Broadcast(FText::FromString("OPERACIÓN ABORTADA: Has filtrado los documentos de las cloacas."), Op->OperationType);
        }
    }
}
