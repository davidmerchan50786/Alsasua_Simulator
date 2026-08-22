#include "Systemics/Urban/UrbanStateSubsystem.h"

void UUrbanStateSubsystem::IncreaseTension(FName SectorName, float Amount) {
    FSectorState& State = Sectors.FindOrAdd(SectorName);
    State.SectorName = SectorName;
    State.TensionLevel = FMath::Clamp(State.TensionLevel + Amount, 0.f, 100.f);

    ESectorTension OldState = State.CurrentState;

    if (State.TensionLevel > 80.f) State.CurrentState = ESectorTension::MartialLaw;
    else if (State.TensionLevel > 60.f) State.CurrentState = ESectorTension::Riot;
    else if (State.TensionLevel > 40.f) State.CurrentState = ESectorTension::Disturbance;
    else if (State.TensionLevel > 20.f) State.CurrentState = ESectorTension::Uneasy;
    else State.CurrentState = ESectorTension::Calm;

    if (OldState != State.CurrentState) {
        OnSectorStateChanged.Broadcast(SectorName, State.CurrentState);
    }
}

FSectorState UUrbanStateSubsystem::GetSectorState(FName SectorName) const {
    if (const FSectorState* State = Sectors.Find(SectorName)) return *State;
    return FSectorState();
}
