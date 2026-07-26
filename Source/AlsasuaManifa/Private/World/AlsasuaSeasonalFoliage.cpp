#include "World/AlsasuaSeasonalFoliage.h"
#include "World/Time/TimeOfDayManager.h"
#include "Engine/World.h"

UAlsasuaSeasonalFoliage::UAlsasuaSeasonalFoliage()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 1.0f;
}

void UAlsasuaSeasonalFoliage::BeginPlay()
{
	Super::BeginPlay();
}

void UAlsasuaSeasonalFoliage::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateSeasonalColors(DeltaTime);
}

void UAlsasuaSeasonalFoliage::UpdateSeasonalColors(float DeltaTime)
{
	UWorld* W = GetWorld();
	if (!W) return;

	UTimeOfDayManager* TimeMgr = W->GetSubsystem<UTimeOfDayManager>();
	if (!TimeMgr) return;

	CurrentTimeYear = TimeMgr->CurrentTime / 365.f;

	const float Season = CurrentTimeYear;
	FLinearColor LeafColor;
	float Saturation = 1.f;

	if (Season < 0.25f)
	{
		// Spring (0-0.25)
		const float T = Season / 0.25f;
		LeafColor = FMath::Lerp(WinterLeafColor, SpringLeafColor, T);
		Saturation = FMath::Lerp(WinterSaturation, SpringSaturation, T);
	}
	else if (Season < 0.5f)
	{
		// Summer (0.25-0.5)
		const float T = (Season - 0.25f) / 0.25f;
		LeafColor = FMath::Lerp(SpringLeafColor, SummerLeafColor, T);
		Saturation = FMath::Lerp(SpringSaturation, SummerSaturation, T);
	}
	else if (Season < 0.75f)
	{
		// Autumn (0.5-0.75)
		const float T = (Season - 0.5f) / 0.25f;
		const float AutumnBlend = FMath::Sin(T * 3.14159f);
		LeafColor = FMath::Lerp(SummerLeafColor,
			FMath::Lerp(AutumnLeafColor1, AutumnLeafColor2, AutumnBlend), T);
		Saturation = FMath::Lerp(SummerSaturation, AutumnSaturation, T);
	}
	else
	{
		// Winter (0.75-1.0)
		const float T = (Season - 0.75f) / 0.25f;
		LeafColor = FMath::Lerp(AutumnLeafColor3, WinterLeafColor, T);
		Saturation = FMath::Lerp(AutumnSaturation, WinterSaturation, T);
	}

	AActor* Owner = GetOwner();
	if (!Owner) return;

	TArray<UMaterialInstanceDynamic*> DynamicMats;
	TArray<UPrimitiveComponent*> Primitives;
	Owner->GetComponents<UPrimitiveComponent>(Primitives);

	for (UPrimitiveComponent* Prim : Primitives)
	{
		if (!Prim) continue;
		UMeshComponent* Mesh = Cast<UMeshComponent>(Prim);
		if (!Mesh) continue;

		const int32 NumMats = Mesh->GetNumMaterials();
		for (int32 i = 0; i < NumMats; ++i)
		{
			UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(i));
			if (MID)
			{
				MID->SetVectorParameterValue(FName("LeafColor"), LeafColor);
				MID->SetScalarParameterValue(FName("SeasonSaturation"), Saturation);
			}
		}
	}
}
