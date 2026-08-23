#include "World/AlsasuaCtrlFarolasPilar.h"
#include "World/AlsasuaStreetLightController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"


int32 UAlsasuaCtrlFarolasPilar::EjecutarArranque()
{
	int32 FarolaCount = 0;
	TArray<AActor*> FarolaActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName(TEXT("Farola")), FarolaActors);
	for (AActor* Actor : FarolaActors)
	{
		if (!Actor) continue;
		if (UAlsasuaStreetLightController* Light = NewObject<UAlsasuaStreetLightController>(Actor))
		{
			Light->RegisterComponent();
			++FarolaCount;
		}
	}
	return FarolaCount;
}
