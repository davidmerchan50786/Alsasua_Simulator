#include "Mision/MisionManager.h"

void UMisionManager::StartMission(const FMissionData& MissionData)
{
    if (IsMissionActive(MissionData.MissionID))
    {
        return;
    }

    FMisionActiva NewMission;
    NewMission.MissionID = MissionData.MissionID;
    NewMission.TiempoInicio = FPlatformTime::Seconds();
    NewMission.bCompletada = false;
    ActiveMissions.Add(NewMission);

    OnMissionStateChanged.Broadcast(NewMission);
}

void UMisionManager::CompleteMission(FName MissionID)
{
    for (int32 i = 0; i < ActiveMissions.Num(); ++i)
    {
        if (ActiveMissions[i].MissionID == MissionID && !ActiveMissions[i].bCompletada)
        {
            ActiveMissions[i].bCompletada = true;
            OnMissionStateChanged.Broadcast(ActiveMissions[i]);
            ActiveMissions.RemoveAt(i);
            return;
        }
    }
}

void UMisionManager::FailMission(FName MissionID)
{
    for (int32 i = ActiveMissions.Num() - 1; i >= 0; --i)
    {
        if (ActiveMissions[i].MissionID == MissionID)
        {
            ActiveMissions.RemoveAt(i);
            return;
        }
    }
}

bool UMisionManager::IsMissionActive(FName MissionID) const
{
    for (const FMisionActiva& M : ActiveMissions)
    {
        if (M.MissionID == MissionID) return true;
    }
    return false;
}
