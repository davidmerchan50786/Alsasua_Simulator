// WantedSubsystem.h (capa GAMEPLAY)
// Nivel de búsqueda (0-5) estilo GTA: timer + zona de búsqueda + fuga por
// esconderse. Puerto de IWantedSystem + SistemaEscapeWanted.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "WantedSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEstrellasCambia, int32, Nivel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWantedEscaped);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSearchStarted, FVector, Location, float, Radius);

UCLASS()
class ALSASUAGAMEPLAY_API UWantedSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category="Wanted") int32 NivelBusqueda = 0;   // 0-5
	UPROPERTY(EditAnywhere, Category="Wanted")      float TiempoBajarNivel = 35.f;
	UPROPERTY(BlueprintAssignable, Category="Wanted") FOnEstrellasCambia OnEstrellasCambia;

	/** Fires when player escapes (wanted hits 0 after being hidden long enough). */
	UPROPERTY(BlueprintAssignable, Category="Wanted") FOnWantedEscaped OnWantedEscaped;

	/** Fires when a search zone spawns at last-known location. */
	UPROPERTY(BlueprintAssignable, Category="Wanted") FOnSearchStarted OnSearchStarted;

	/** Radius of the search zone (scales with wanted level). */
	UPROPERTY(EditAnywhere, Category="Wanted|Search") float SearchRadiusBase = 4000.f;
	UPROPERTY(EditAnywhere, Category="Wanted|Search") float SearchRadiusPerStar = 1500.f;

	/** Time player must remain hidden (out of search zone, not seen) before wanted drops. */
	UPROPERTY(EditAnywhere, Category="Wanted|Search") float TimeToReduceStar = 20.f;

	// cantidad negativa baja el nivel (clamp 0-5), como en Unity.
	UFUNCTION(BlueprintCallable, Category="Wanted") void AumentarBusqueda(int32 Cantidad = 1);

	/** Report a sighting of the player — updates search zone to current position. */
	UFUNCTION(BlueprintCallable, Category="Wanted") void ReportSighting(FVector Location);

	/** Reset and clear wanted without escape event (e.g., interrogation surrender). */
	UFUNCTION(BlueprintCallable, Category="Wanted") void ClearWanted();

	UFUNCTION(BlueprintPure, Category="Wanted") FVector GetSearchCenter() const { return SearchCenter; }
	UFUNCTION(BlueprintPure, Category="Wanted") float  GetSearchRadius() const { return CurrentSearchRadius; }
	UFUNCTION(BlueprintPure, Category="Wanted") bool   IsInSearchZone(FVector Location) const;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UWantedSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }

private:
	float TimerBajar = 0.f;
	float HiddenTimer = 0.f;

	/** Zone where cops are actively searching. */
	bool bSearchActive = false;
	FVector SearchCenter = FVector::ZeroVector;
	float CurrentSearchRadius = 4000.f;
};