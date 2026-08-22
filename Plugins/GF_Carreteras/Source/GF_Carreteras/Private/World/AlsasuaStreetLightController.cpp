#include "World/AlsasuaStreetLightController.h"
#include "World/Time/TimeOfDayManager.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/BillboardComponent.h"
#include "Engine/World.h"

UAlsasuaStreetLightController::UAlsasuaStreetLightController()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.15f;
}

void UAlsasuaStreetLightController::BeginPlay()
{
	Super::BeginPlay();
	SetupLightComponents();

	// Una sola tirada por farola. Antes se tiraba cada tick estando encendida:
	// con BurnOutChance=0.001 a 0.15 s/tick el pueblo entero se quedaba a
	// oscuras en unos minutos de juego.
	bBurnedOut = FMath::FRand() < BurnOutChance;

	// El parpadeo también se decide una vez: si cada farola puede parpadear en
	// cualquier momento, todo el alumbrado estrobea a la vez.
	bFlickers = bEnableFlicker && FMath::FRand() < FlickerFixtureChance;
}

void UAlsasuaStreetLightController::SetupLightComponents()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	if (!PointLight)
	{
		PointLight = NewObject<UPointLightComponent>(Owner);
		if (PointLight)
		{
			PointLight->SetupAttachment(Owner->GetRootComponent());
			PointLight->SetRelativeLocation(FVector(0, 0, LightHeight));
			PointLight->SetIntensity(0.f);
			PointLight->SetLightColor(WarmColor);
			PointLight->SetAttenuationRadius(LightRadius);
			PointLight->SetCastShadows(true);
			PointLight->SetIntensityUnits(ELightUnits::Candelas);
			PointLight->RegisterComponent();
		}
	}

	// El "reflejo de charco" era una segunda luz por farola que nadie volvía a
	// tocar: se quedaba a intensidad 0 para siempre y sólo costaba GPU. El
	// reflejo real lo dan las reflexiones de Lumen sobre el asfalto mojado.
}

void UAlsasuaStreetLightController::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateLightState(DeltaTime);

	if (bFlickers && bIsOn)
	{
		ApplyFlicker(DeltaTime);
	}
}

void UAlsasuaStreetLightController::UpdateLightState(float DeltaTime)
{
	UWorld* W = GetWorld();
	if (!W) return;

	UTimeOfDayManager* TimeMgr = W->GetSubsystem<UTimeOfDayManager>();
	if (!TimeMgr) return;

	const float Hour = TimeMgr->CurrentTime;
	bool bShouldBeOn = false;

	if (TurnOnHour > TurnOffHour)
	{
		bShouldBeOn = (Hour >= TurnOnHour || Hour < TurnOffHour);
	}
	else
	{
		bShouldBeOn = (Hour >= TurnOnHour && Hour < TurnOffHour);
	}

	if (bBurnedOut)
	{
		TargetIntensity = 0.f;
	}
	else
	{
		TargetIntensity = bShouldBeOn ? MaxIntensity : 0.f;
	}

	bIsOn = bShouldBeOn && !bBurnedOut;

	const float FadeSpeed = (TargetIntensity > CurrentIntensity) ? (1.f / FadeInTime) : (1.f / FadeOutTime);
	CurrentIntensity = FMath::FInterpTo(CurrentIntensity, TargetIntensity, DeltaTime, FadeSpeed);

	if (PointLight)
	{
		PointLight->SetIntensity(CurrentIntensity);
	}
}

void UAlsasuaStreetLightController::ApplyFlicker(float DeltaTime)
{
	FlickerTimer += DeltaTime;

	// Un parpadeo dura FlickerDuration y después la farola vuelve a su
	// intensidad: antes se dejaba a medio gas hasta el tick siguiente.
	if (FlickerTimer < FlickerDuration)
	{
		if (PointLight) PointLight->SetIntensity(CurrentIntensity * FlickerScale);
		return;
	}

	if (FMath::FRand() < FlickerChance)
	{
		FlickerScale = FMath::RandRange(0.3f, 0.9f);
		FlickerTimer = 0.f;
		if (PointLight) PointLight->SetIntensity(CurrentIntensity * FlickerScale);
	}
}
