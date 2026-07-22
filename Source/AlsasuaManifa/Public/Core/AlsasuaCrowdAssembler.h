#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "AlsasuaCrowdAssembler.generated.h"

UCLASS()
class ALSASUAMANIFA_API AAlsasuaCrowdAssembler : public AInfo
{
	GENERATED_BODY()

public:
	AAlsasuaCrowdAssembler();

	// Ejecuta el despliegue masivo optimizado
	UFUNCTION(BlueprintCallable, Category = "AAA|Deployment")
	void DeployMassiveCrowd(FVector Center, float Radius, int32 Count);

private:
	void InternalSpawnProxy(FVector Location);
};
