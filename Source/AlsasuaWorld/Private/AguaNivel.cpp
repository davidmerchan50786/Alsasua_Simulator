#include "AguaNivel.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "World/Weather/WeatherSubsystem.h"

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
			TEXT("/Game/Materiales/M_AguaNivel.M_AguaNivel"));
	}
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
			WaterMatInst->SetScalarParameterValue(TEXT("WaveSpeed"), WaveSpeed);
			WaterMatInst->SetScalarParameterValue(TEXT("ShorelineDepth"), ShorelineDepth);
			WaterMatInst->SetScalarParameterValue(TEXT("SpecularPower"), SpecularPower);
			WaterMatInst->SetScalarParameterValue(TEXT("DepthFadeDistance"), DepthFadeDistance);
			WaterMatInst->SetScalarParameterValue(TEXT("RainRippleIntensity"), 0.f);
			WaterMatInst->SetScalarParameterValue(TEXT("RainRippleSpeed"), RainRippleSpeed);
			WaterMatInst->SetScalarParameterValue(TEXT("RainRippleScale"), RainRippleScale);
			WaterMatInst->SetScalarParameterValue(TEXT("FoamIntensity"), 0.f);
		}
	}
}

void AAguaNivel::ActualizarOndas(float Time)
{
	if (WaterMatInst)
	{
		WaterMatInst->SetScalarParameterValue(TEXT("TimeParam"), Time * WaveSpeed);
	}
}

void AAguaNivel::ActualizarClima()
{
	if (!WaterMatInst) return;

	UWorld* W = GetWorld();
	if (!W) return;

	const UWeatherSubsystem* Weather = W->GetSubsystem<UWeatherSubsystem>();
	if (!Weather) return;

	const float Rain = Weather->GetRainIntensity();
	const float Wind = Weather->GetWindSpeed();
	const FVector WindDir = Weather->GetWindDirection();

	// ── Color: lerp clean → rain-darkened ────────────────────────────────
	const FLinearColor CurColor = WaterColor + (RainWaterColor - WaterColor) * Rain;
	WaterMatInst->SetVectorParameterValue(TEXT("WaterColor"), CurColor);

	// ── Opacity: slightly more opaque in rain ───────────────────────────
	WaterMatInst->SetScalarParameterValue(TEXT("OpacityBase"), OpacityBase + Rain * RainOpacityBoost);

	// ── Waves: wind drives amplitude + frequency ─────────────────────────
	const float WindT = FMath::Clamp(Wind / 30.f, 0.f, 1.f); // 0-30 kmh → 0-1
	const float CurAmplitude = WaveAmplitude + WindT * WindAmplitudeScale;
	const float CurFrequency = WaveFrequency + WindT * WindFrequencyScale;
	WaterMatInst->SetScalarParameterValue(TEXT("WaveAmplitude"), CurAmplitude);
	WaterMatInst->SetScalarParameterValue(TEXT("WaveFrequency"), CurFrequency);

	// ── Rain ripples: concentric ring pattern on surface ─────────────────
	WaterMatInst->SetScalarParameterValue(TEXT("RainRippleIntensity"), Rain * RainRippleIntensity);

	// ── Specular: rain makes water shinier (wet surface = lower roughness) ─
	const float CurSpecular = FMath::Lerp(SpecularPower, SpecularPower / RainSpecularBoost, Rain);
	WaterMatInst->SetScalarParameterValue(TEXT("SpecularPower"), CurSpecular);

	// ── Foam: storm boost ───────────────────────────────────────────────
	const float bStorm = (Weather->CurrentWeather == EWeatherSubsystemState::Thunderstorm) ? 1.f : 0.f;
	const float FoamIntensity = Rain * 0.2f + bStorm * StormFoamBoost;
	WaterMatInst->SetScalarParameterValue(TEXT("FoamIntensity"), FoamIntensity);

	// ── Wind direction: feed to wave direction offset ────────────────────
	const float WindAngle = FMath::Atan2(WindDir.Y, WindDir.X);
	WaterMatInst->SetScalarParameterValue(TEXT("WindDirectionAngle"), WindAngle);
}

void AAguaNivel::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UWorld* W = GetWorld();
	if (!W) return;

	const float Time = W->GetTimeSeconds();
	ActualizarOndas(Time);
	ActualizarClima();
}
