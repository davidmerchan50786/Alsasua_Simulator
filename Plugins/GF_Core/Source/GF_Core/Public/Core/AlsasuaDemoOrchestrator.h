#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "AlsasuaDemoOrchestrator.generated.h"

UCLASS()
class GF_CORE_API AAlsasuaDemoOrchestrator : public AInfo
{
	GENERATED_BODY()

public:
	AAlsasuaDemoOrchestrator();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// Configura la línea de tiempo narrativa de la demo
	void SetupNarrativeTimeline();

	// Spawnea la multitud inicial y las fuerzas de seguridad
	void InitializeCrowdAndPolice();

	// Callback cuando ocurre el evento policial
	UFUNCTION()
	void HandlePoliceCharge(FName EventID);
};
