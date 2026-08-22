#include "World/AlsasuaTerrainMaterialBlender.h"
#include "World/AlsasuaVisualEffectsManager.h"
#include "World/Time/TimeOfDayManager.h"
#include "Engine/World.h"

UAlsasuaTerrainMaterialBlender::UAlsasuaTerrainMaterialBlender()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.5f;
}

void UAlsasuaTerrainMaterialBlender::BeginPlay()
{
	Super::BeginPlay();
}

void UAlsasuaTerrainMaterialBlender::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateTerrainBlending(DeltaTime);
}

void UAlsasuaTerrainMaterialBlender::UpdateTerrainBlending(float DeltaTime)
{
	UWorld* W = GetWorld();
	if (!W) return;

	UAlsasuaVisualEffectsManager* VFXMgr = W->GetSubsystem<UAlsasuaVisualEffectsManager>();
	UTimeOfDayManager* TimeMgr = W->GetSubsystem<UTimeOfDayManager>();

	if (!VFXMgr || !TimeMgr) return;

	const float Wetness = VFXMgr->GlobalWetness;
	const float Season = TimeMgr->CurrentTime / 365.f;

	// Determine current snow line based on season
	float CurrentSnowLine = SummerSnowLine;
	if (Season < 0.25f) CurrentSnowLine = SpringSnowLine;
	else if (Season < 0.5f) CurrentSnowLine = SummerSnowLine;
	else if (Season < 0.75f) CurrentSnowLine = AutumnSnowLine;
	else CurrentSnowLine = WinterSnowLine;

	AActor* Owner = GetOwner();
	if (!Owner) return;

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
			if (!MID) continue;

			MID->SetScalarParameterValue(FName("GrassMaxSlope"), GrassMaxSlope);
			MID->SetScalarParameterValue(FName("DirtMaxSlope"), DirtMaxSlope);
			MID->SetScalarParameterValue(FName("RockMinSlope"), RockMinSlope);
			MID->SetScalarParameterValue(FName("SnowMinAltitude"), SnowMinAltitude);
			MID->SetScalarParameterValue(FName("SnowMaxAltitude"), SnowMaxAltitude);
			MID->SetScalarParameterValue(FName("BlendSmoothness"), BlendSmoothness);
			MID->SetScalarParameterValue(FName("AltitudeBlendRange"), AltitudeBlendRange);
			MID->SetScalarParameterValue(FName("SnowLine"), CurrentSnowLine);
			MID->SetScalarParameterValue(FName("Wetness"), Wetness);
			MID->SetScalarParameterValue(FName("WetGrassDarkening"), WetGrassDarkening);
			MID->SetScalarParameterValue(FName("WetDirtDarkening"), WetDirtDarkening);
			MID->SetScalarParameterValue(FName("WetRockShininess"), WetRockShininess);
		}
	}
}
