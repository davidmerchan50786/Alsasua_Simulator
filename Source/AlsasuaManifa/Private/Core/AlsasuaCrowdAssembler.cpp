#include "Core/AlsasuaCrowdAssembler.h"
#include "Mass/AlsasuaMassParallelManager.h"

AAlsasuaCrowdAssembler::AAlsasuaCrowdAssembler() { PrimaryActorTick.bCanEverTick = false; }

void AAlsasuaCrowdAssembler::DeployMassiveCrowd(FVector Center, float Radius, int32 Count)
{
	UAlsasuaMassParallelManager* Mass = GetWorld()->GetSubsystem<UAlsasuaMassParallelManager>();
	if (!Mass) return;

	for(int32 i = 0; i < Count; i++)
	{
		// Generación en anillo para evitar el centro colapsado
		FVector RandomPos = Center + FVector(FMath::RandRange(-Radius, Radius), FMath::RandRange(-Radius, Radius), 0);
		InternalSpawnProxy(RandomPos);
	}
	UE_LOG(LogTemp, Warning, TEXT("AAA+++: Desplegados %d ciudadanos de Alsasua sistémicos."), Count);
}

void AAlsasuaCrowdAssembler::InternalSpawnProxy(FVector Location)
{
	UAlsasuaMassParallelManager* Mass = GetWorld()->GetSubsystem<UAlsasuaMassParallelManager>();
	if (Mass)
	{
		FMassProtesterProxy NewProxy;
		NewProxy.Position = Location;
		NewProxy.Rotation = FRotator(0, FMath::RandRange(0.f, 360.f), 0);
		Mass->Proxies.Add(NewProxy);
	}
}
