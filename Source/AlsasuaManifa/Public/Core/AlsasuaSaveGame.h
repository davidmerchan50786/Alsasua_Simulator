#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "AlsasuaSaveGame.generated.h"

/**
 * SaveGame wrapper unificado. Esta clase es un adaptador para migración.
 * La fuente de verdad real es UAlsasuaLegacySaveGame en AlsasuaGameplay.
 * Esta clase existe para compatibilidad con código existente que use UAlsasuaSaveGame.
 */
UCLASS(Deprecated, meta=(DeprecationMessage="Usa UAlsasuaLegacySaveGame directamente"))
class ALSASUAMANIFA_API UAlsasuaSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UAlsasuaSaveGame();

	UPROPERTY(VisibleAnywhere, Category = "Player")
	FVector PlayerLocation;

	UPROPERTY(VisibleAnywhere, Category = "Player")
	FRotator PlayerRotation;

	UPROPERTY(VisibleAnywhere, Category = "World State")
	float SavedPopularSupport;

	UPROPERTY(VisibleAnywhere, Category = "World State")
	float SavedWantedLevel;

	UPROPERTY(VisibleAnywhere, Category = "Missions")
	TArray<FString> CompletedMissionIDs;

	UPROPERTY(VisibleAnywhere, Category = "Climate")
	float SavedCurrentTime;

	UPROPERTY(VisibleAnywhere, Category = "Save Info")
	FString SaveSlotName;

	UPROPERTY(VisibleAnywhere, Category = "Save Info")
	int32 UserIndex;
};
