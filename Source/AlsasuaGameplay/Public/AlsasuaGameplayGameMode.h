#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AlsasuaGameplayGameMode.generated.h"

UCLASS()
class ALSASUAGAMEPLAY_API AAlsasuaGameplayGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AAlsasuaGameplayGameMode();
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual void StartPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    bool bPendienteColocarJugador = false;
    double HoraInicioWall = 0.0;
    bool bCsvListo = false;
};
