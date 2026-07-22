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
};
