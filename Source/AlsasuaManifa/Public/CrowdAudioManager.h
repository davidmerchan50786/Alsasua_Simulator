#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CrowdAudioManager.generated.h"

UCLASS()
class ALSASUAMANIFA_API UCrowdAudioManager : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	// Calcula el volumen dinámico según NPCs activos en la protesta
	UFUNCTION(BlueprintPure, Category = "Audio")
	float GetCrowdVolumeMultiplier(int32 ActiveProtesters) const;
};
