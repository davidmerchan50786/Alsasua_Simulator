#include "UI/InvestigationBoardWidget.h"

void UInvestigationBoardWidget::RequestStartMission(FName NodeId)
{
    if (NodeId.IsNone())
    {
        return;
    }

    OnMissionRequested.Broadcast(NodeId);
}
