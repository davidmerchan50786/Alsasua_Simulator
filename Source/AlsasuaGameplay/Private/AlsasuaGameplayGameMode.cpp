#include "AlsasuaGameplayGameMode.h"
#include "AlsasuaGameplayHUD.h"

AAlsasuaGameplayGameMode::AAlsasuaGameplayGameMode()
{
    HUDClass = AAlsasuaGameplayHUD::StaticClass();
}

void AAlsasuaGameplayGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
    UE_LOG(LogTemp, Log, TEXT("AlsasuaGameplayGameMode: Inicializado en mapa %s"), *MapName);
}
