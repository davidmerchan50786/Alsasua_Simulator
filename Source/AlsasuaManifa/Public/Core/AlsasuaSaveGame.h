#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Systems/MisionManager.h"
#include "AlsasuaSaveGame.generated.h"

UCLASS()
class ALSASUAMANIFA_API UAlsasuaSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UAlsasuaSaveGame();

	// --- Datos del Jugador ---
	UPROPERTY(VisibleAnywhere, Category = "Player")
	FVector PlayerLocation;

	UPROPERTY(VisibleAnywhere, Category = "Player")
	FRotator PlayerRotation;

	// --- Datos del Mundo y GAS ---
	UPROPERTY(VisibleAnywhere, Category = "World State")
	float SavedPopularSupport;

	UPROPERTY(VisibleAnywhere, Category = "World State")
	float SavedWantedLevel;

	// --- Estado de Misiones ---
	UPROPERTY(VisibleAnywhere, Category = "Missions")
	TArray<FString> CompletedMissionIDs;

	// --- Estado de la Ciudad ---
	UPROPERTY(VisibleAnywhere, Category = "Climate")
	float SavedCurrentTime;

	UPROPERTY(VisibleAnywhere, Category = "Save Info")
	FString SaveSlotName;

	UPROPERTY(VisibleAnywhere, Category = "Save Info")
	int32 UserIndex;
};
