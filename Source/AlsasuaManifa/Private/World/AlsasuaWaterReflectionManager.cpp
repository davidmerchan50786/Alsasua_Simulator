#include "World/AlsasuaWaterReflectionManager.h"
#include "World/AlsasuaVisualEffectsManager.h"
#include "Components/PlanarReflectionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

UAlsasuaWaterReflectionManager::UAlsasuaWaterReflectionManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
}

void UAlsasuaWaterReflectionManager::BeginPlay()
{
	Super::BeginPlay();
	SetupReflections();
}

void UAlsasuaWaterReflectionManager::SetupReflections()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	UPlanarReflectionComponent* Reflection = NewObject<UPlanarReflectionComponent>(Owner);
	if (Reflection)
	{
		Reflection->SetupAttachment(Owner->GetRootComponent());
		Reflection->SetRelativeLocation(FVector(0, 0, 5.f));
		Reflection->SetRelativeRotation(FRotator(-90.f, 0, 0));

		Reflection->PlanarReflectionNormal = FVector(0, 0, 1);
		Reflection->CaptureScene = true;
		Reflection->bUseClippingPlane = true;
		Reflection->ClippingPlaneOffset = 10.f;
		Reflection->RenderStage = EPlanarReflectionRenderStage::MainPass;

		Reflection->RegisterComponent();
	}
}

void UAlsasuaWaterReflectionManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateWaterParameters(DeltaTime);
}

void UAlsasuaWaterReflectionManager::UpdateWaterParameters(float DeltaTime)
{
	TimeAccum += DeltaTime;

	UWorld* W = GetWorld();
	if (!W) return;

	UAlsasuaVisualEffectsManager* VFXMgr = W->GetSubsystem<UAlsasuaVisualEffectsManager>();
	const float Wetness = VFXMgr ? VFXMgr->GlobalWetness : 0.f;
	const float Wind = VFXMgr ? VFXMgr->WindIntensity : 0.3f;

	AActor* Owner = GetOwner();
	if (!Owner) return;

	TArray<UStaticMeshComponent*> MeshComps;
	Owner->GetComponents<UStaticMeshComponent>(MeshComps);

	for (UStaticMeshComponent* SMC : MeshComps)
	{
		if (!SMC) continue;

		const int32 NumMats = SMC->GetNumMaterials();
		for (int32 i = 0; i < NumMats; ++i)
		{
			UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(SMC->GetMaterial(i));
			if (!MID) continue;

			MID->SetScalarParameterValue(FName("Time"), TimeAccum);
			MID->SetScalarParameterValue(FName("WaterSpeed"), WaterSpeed);
			MID->SetScalarParameterValue(FName("WaveAmplitude"), WaveAmplitude);
			MID->SetScalarParameterValue(FName("WaveFrequency"), WaveFrequency);
			MID->SetVectorParameterValue(FName("ShallowColor"), ShallowColor);
			MID->SetVectorParameterValue(FName("DeepColor"), DeepColor);
			MID->SetScalarParameterValue(FName("FoamIntensity"), FoamIntensity);
			MID->SetScalarParameterValue(FName("RainRippleIntensity"), Wetness * RainRippleIntensity);
			MID->SetScalarParameterValue(FName("RainNormalStrength"), Wetness * RainNormalStrength);
			MID->SetScalarParameterValue(FName("WindStrength"), Wind);
		}
	}
}
