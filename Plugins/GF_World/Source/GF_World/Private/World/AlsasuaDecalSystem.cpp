#include "World/AlsasuaDecalSystem.h"
#include "World/AlsasuaVisualEffectsManager.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"

UAlsasuaDecalSystem::UAlsasuaDecalSystem()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAlsasuaDecalSystem::BeginPlay()
{
	Super::BeginPlay();

	SpawnRoadMarkings();
	SpawnPuddles();
	SpawnCracks();
	SpawnWear();
}

void UAlsasuaDecalSystem::SpawnRoadMarkings()
{
	if (!RoadMarkingMaterial) return;

	AActor* Owner = GetOwner();
	if (!Owner) return;

	const FVector Origin = Owner->GetActorLocation();
	const FRotator Rotation = Owner->GetActorRotation();

	const int32 NumMarkings = 3;
	const float Spacing = RoadMarkingLength / NumMarkings;

	for (int32 i = 0; i < NumMarkings; ++i)
	{
		UDecalComponent* Decal = NewObject<UDecalComponent>(Owner);
		if (!Decal) continue;

		Decal->SetupAttachment(Owner->GetRootComponent());
		Decal->SetRelativeLocation(FVector(i * Spacing, 0, 5.f));
		Decal->SetRelativeRotation(FRotator(0, 0, 90.f));

		Decal->SetDecalMaterial(RoadMarkingMaterial);
		Decal->DecalSize = FVector(RoadMarkingLength / NumMarkings, RoadMarkingWidth, 2.f);
		Decal->SetFadeScreenSize(0.001f);
		Decal->RegisterComponent();

		SpawnedDecals.Add(Decal);
	}
}

void UAlsasuaDecalSystem::SpawnPuddles()
{
	if (!PuddleDecalMaterial) return;

	AActor* Owner = GetOwner();
	if (!Owner) return;

	UWorld* W = GetWorld();
	if (!W) return;

	UAlsasuaVisualEffectsManager* VFXMgr = W->GetSubsystem<UAlsasuaVisualEffectsManager>();
	const float Wetness = VFXMgr ? VFXMgr->GlobalWetness : 0.f;

	const int32 NumPuddles = FMath::RandRange(1, FMath::Max(1, (int32)(MaxPuddlesPerRoad * Wetness)));

	for (int32 i = 0; i < NumPuddles; ++i)
	{
		UDecalComponent* Decal = NewObject<UDecalComponent>(Owner);
		if (!Decal) continue;

		Decal->SetupAttachment(Owner->GetRootComponent());

		const float PuddleRadius = FMath::RandRange(PuddleMinRadius, PuddleMaxRadius);
		const float OffsetX = FMath::RandRange(-500.f, 500.f);
		const float OffsetY = FMath::RandRange(-80.f, 80.f);

		Decal->SetRelativeLocation(FVector(OffsetX, OffsetY, 10.f));
		Decal->SetRelativeRotation(FRotator(-90.f, FMath::RandRange(0.f, 360.f), 0.f));

		Decal->SetDecalMaterial(PuddleDecalMaterial);
		Decal->DecalSize = FVector(PuddleRadius, PuddleRadius, 5.f);
		Decal->SetFadeScreenSize(0.005f);
		Decal->RegisterComponent();

		if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Decal->GetDecalMaterial()))
		{
			MID->SetScalarParameterValue(FName("PuddleOpacity"), Wetness * 0.6f);
			MID->SetScalarParameterValue(FName("PuddleRadius"), PuddleRadius);
		}

		SpawnedDecals.Add(Decal);
	}
}

void UAlsasuaDecalSystem::SpawnCracks()
{
	if (!CrackDecalMaterial) return;

	AActor* Owner = GetOwner();
	if (!Owner) return;

	const int32 NumCracks = FMath::RandRange(0, MaxCracksPerRoad);

	for (int32 i = 0; i < NumCracks; ++i)
	{
		UDecalComponent* Decal = NewObject<UDecalComponent>(Owner);
		if (!Decal) continue;

		Decal->SetupAttachment(Owner->GetRootComponent());

		const float OffsetX = FMath::RandRange(-600.f, 600.f);
		const float OffsetY = FMath::RandRange(-60.f, 60.f);

		Decal->SetRelativeLocation(FVector(OffsetX, OffsetY, 8.f));
		Decal->SetRelativeRotation(FRotator(-90.f, FMath::RandRange(0.f, 360.f), 0.f));

		Decal->SetDecalMaterial(CrackDecalMaterial);
		const float Scale = FMath::RandRange(0.8f, CrackScale);
		Decal->DecalSize = FVector(Scale * 100.f, Scale * 100.f, 3.f);
		Decal->SetFadeScreenSize(0.01f);
		Decal->RegisterComponent();

		SpawnedDecals.Add(Decal);
	}
}

void UAlsasuaDecalSystem::SpawnWear()
{
	if (!WearDecalMaterial) return;

	AActor* Owner = GetOwner();
	if (!Owner) return;

	if (FMath::FRand() > WearProbability) return;

	UDecalComponent* Decal = NewObject<UDecalComponent>(Owner);
	if (!Decal) return;

	Decal->SetupAttachment(Owner->GetRootComponent());
	Decal->SetRelativeLocation(FVector(0, 0, 6.f));
	Decal->SetRelativeRotation(FRotator(-90.f, 0, 0));

	Decal->SetDecalMaterial(WearDecalMaterial);
	Decal->DecalSize = FVector(300.f, 100.f, 4.f);
	Decal->SetFadeScreenSize(0.005f);
	Decal->RegisterComponent();

	SpawnedDecals.Add(Decal);
}
