#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AlsasuaCharacter.h"
#include "AlsasuaActorPoolSubsystem.generated.h"

UCLASS()
class ALSASUAMANIFA_API UAlsasuaActorPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Reserva inicial de actores para evitar Spawn en tiempo real
	void WarmUpPool(TSubclassOf<AAlsasuaCharacter> Class, int32 Size);

	// Obtiene un actor del pool o crea uno si está vacío (Fallback)
	AAlsasuaCharacter* AcquireActor(FVector Location, FRotator Rotation);

	// Devuelve el actor al pool y lo desactiva (ahorro CPU)
	void ReleaseActor(AAlsasuaCharacter* Actor);

private:
	UPROPERTY()
	TArray<AAlsasuaCharacter*> InactivePool;
};
