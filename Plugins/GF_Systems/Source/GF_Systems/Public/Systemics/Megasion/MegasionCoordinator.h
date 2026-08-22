#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "MegasionCoordinator.generated.h"

class AAmbientSound;
class UMegaMarchSubsystem;
class USoundBase;

/**
 * MegasionCoordinator: subsistema de mundo que ata la manifestación (marcha),
 * las fuentes de audio (megáfonos y banda) y el temporizador del evento.
 * StartMegasion activa MegaMarchSubsystem con los participantes dados, cría
 * emisores de audio alrededor del centro y arranca la cuenta atrás; al llegar
 * a EventDuration se detiene solo. StopMegasion desmonta todo.
 */
UCLASS()
class GF_SYSTEMS_API UMegasionCoordinator : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
public:
	/** Arranca el evento: marcha + audio + temporizador (reinicia si ya activo). */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Megasion")
	void StartMegasion(FVector Center, int32 Participants);

	/** Para la marcha, destruye las fuentes de audio y apaga el evento. */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Megasion")
	void StopMegasion();

	/** Resumen legible del estado para HUD/logs. */
	UFUNCTION(BlueprintPure, Category = "Alsasua|Megasion")
	FString GetEventStatus() const;

	// --- FTickableGameObject ---
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMegasionCoordinator, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate() && bEventActive && GetWorld() != nullptr && GetWorld()->HasBegunPlay(); }

	/** Duración del evento en segundos; al cumplirse se auto-detiene. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Megasion")
	float EventDuration = 300.f;

	UPROPERTY(BlueprintReadOnly, Category = "Alsasua|Megasion")
	bool bEventActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Alsasua|Megasion")
	float EventTimer = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Alsasua|Megasion")
	int32 TotalParticipants = 0;

	/** Sonido de megáfono; si es nulo no se spawnan megáfonos. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alsasua|Megasion")
	TObjectPtr<USoundBase> MegaphoneSound = nullptr;

	/** Sonido de banda; si es nulo no se spawnan bandas. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alsasua|Megasion")
	TObjectPtr<USoundBase> BandSound = nullptr;

	/** Emisores por tipo repartidos alrededor del centro. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alsasua|Megasion", meta = (ClampMin = "1"))
	int32 AudioSourcesPerType = 2;

private:
	void SpawnAudioSources(FVector Center);
	AAmbientSound* SpawnEmisor(USoundBase* Sonido, const FVector& Posicion);

	UPROPERTY()
	TArray<TObjectPtr<AAmbientSound>> AudioSources;

	FVector EventCenter = FVector::ZeroVector;
};
