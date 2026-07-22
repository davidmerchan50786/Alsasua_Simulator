// AAlsasuaGameMode — GameMode mínimo (puerto inicial de GameManagerAltsasua.cs).
// Fija el pawn por defecto al jugador de Alsasua. Más adelante registrará los Subsystems
// IWantedSystem / IEconomyService / ISpawnService.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AlsasuaLegacyGameMode.generated.h"

UCLASS()
class ALSASUAGAMEPLAY_API AAlsasuaLegacyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AAlsasuaLegacyGameMode();
};
