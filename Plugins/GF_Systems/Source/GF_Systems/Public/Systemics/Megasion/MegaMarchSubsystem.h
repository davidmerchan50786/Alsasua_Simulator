#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "MegaMarchSubsystem.generated.h"

class AStaticMeshActor;
class UMaterialInterface;
class UStaticMesh;

/**
 * Megasion: protesta multitudinaria. Cría hasta MaxProtesters cubos en tonos
 * cálidos alrededor de un centro y los mueve en formación circular lenta con
 * bamboleo. La multitud crece de 0 a MaxProtesters durante los 10 primeros
 * segundos. El tick del subsistema alimenta TickMarch, que también es callable
 * desde BP si alguien prefiere dirigirla a mano.
 */
UCLASS()
class GF_SYSTEMS_API UMegaMarchSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
public:
	/** Cría la manifestación alrededor de Center (reinicia si ya había una). */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Megasion")
	void StartMegaMarch(FVector Center);

	/** Destruye todos los manifestantes. */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Megasion")
	void StopMegaMarch();

	/** Avanza la marcha; llamado por el tick del subsistema a cada frame activo. */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Megasion")
	void TickMarch(float DeltaTime);

	/** 0 si no hay marcha; si la hay, manifestantes / MaxProtesters. */
	UFUNCTION(BlueprintPure, Category = "Alsasua|Megasion")
	float GetProtestIntensity() const;

	// --- FTickableGameObject ---
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMegaMarchSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate() && bActive && GetWorld() != nullptr && GetWorld()->HasBegunPlay(); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Megasion")
	int32 MaxProtesters = 200;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Megasion")
	float ProtestRadius = 5000.f; // cm

	/** Velocidad tangencial de la formación en cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Megasion")
	float MarchSpeed = 100.f;

	UPROPERTY(BlueprintReadOnly, Category = "Alsasua|Megasion")
	bool bActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Alsasua|Megasion")
	FVector ProtestCenter = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Alsasua|Megasion")
	TArray<TObjectPtr<AActor>> ProtesterActors;

private:
	/** Slot de formación de un manifestante: ángulo y radio base alrededor del
	 *  centro más fase propia para el bob y el bamboleo. */
	struct FMarcher
	{
		float Angulo = 0.f;
		float Radio = 0.f;
		float FaseBob = 0.f;
	};

	bool SpawnMarchador(int32 Indice);

	UPROPERTY()
	TObjectPtr<UStaticMesh> MeshCubo = nullptr;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> MaterialBase = nullptr;

	TArray<FMarcher> Marchers;
	float AnguloFormacion = 0.f;
	float TiempoTranscurrido = 0.f;

	static constexpr float SegundosCrecimiento = 10.f;
};
