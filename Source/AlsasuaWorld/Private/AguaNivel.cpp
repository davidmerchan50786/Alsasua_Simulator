#include "AguaNivel.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Engine.h"
#include "UObject/ConstructorHelpers.h"

AAguaNivel::AAguaNivel()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.033f; // 30fps for smooth waves

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	WaterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaterMesh"));
	WaterMesh->SetupAttachment(RootComponent);
	WaterMesh->SetMobility(EComponentMobility::Static);
	WaterMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WaterMesh->SetCollisionObjectType(ECC_WorldStatic);
	WaterMesh->SetCollisionResponseToAllChannels(ECR_Overlap);
	WaterMesh->SetGenerateOverlapEvents(true);
	WaterMesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneAsset(
		TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneAsset.Succeeded())
	{
		WaterMesh->SetStaticMesh(PlaneAsset.Object);
	}
}

void AAguaNivel::BeginPlay()
{
	Super::BeginPlay();

	const FVector Pos = GetActorLocation();
	SetActorLocation(FVector(Pos.X, Pos.Y, AlturaNivelCm));
	SetActorScale3D(FVector(AnchoCm / 100.f, LargoCm / 100.f, 1.f));

	CrearMaterialDinamico();
}

void AAguaNivel::CrearMaterialDinamico()
{
	static UMaterialInterface* BaseMat = nullptr;
	if (!BaseMat)
	{
		BaseMat = LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Engine/EngineMaterials/DefaultWaterMaterial.DefaultWaterMaterial"));
	}
	if (!BaseMat)
	{
		BaseMat = LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial"));
	}
	if (BaseMat)
	{
		WaterMatInst = UMaterialInstanceDynamic::Create(BaseMat, this);
		if (WaterMatInst)
		{
			WaterMesh->SetMaterial(0, WaterMatInst);
			WaterMatInst->SetVectorParameterValue(TEXT("WaterColor"), WaterColor);
			WaterMatInst->SetScalarParameterValue(TEXT("OpacityBase"), OpacityBase);
			WaterMatInst->SetScalarParameterValue(TEXT("FresnelPower"), FresnelPower);
			WaterMatInst->SetScalarParameterValue(TEXT("WaveAmplitude"), WaveAmplitude);
			WaterMatInst->SetScalarParameterValue(TEXT("WaveFrequency"), WaveFrequency);
			WaterMatInst->SetScalarParameterValue(TEXT("WaveSteepness"), WaveSteepness);
		}
	}
}

void AAguaNivel::ActualizarOndas(float Time)
{
	// Gerstner wave parameters are passed to material for vertex displacement
	if (WaterMatInst)
	{
		WaterMatInst->SetScalarParameterValue(TEXT("TimeParam"), Time * WaveSpeed);
	}
}

void AAguaNivel::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UWorld* W = GetWorld();
	if (!W) return;

	const float Time = W->GetTimeSeconds();
	ActualizarOndas(Time);
}
