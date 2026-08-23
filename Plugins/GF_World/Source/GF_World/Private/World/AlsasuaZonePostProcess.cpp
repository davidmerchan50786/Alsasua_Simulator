#include "World/AlsasuaZonePostProcess.h"
#include "AlsasuaServiceRegistry.h"
#include "ContratosClima.h"
#include "Components/PostProcessComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void UAlsasuaZonePostProcess::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadNeighborhoodZones();

	CurrentSaturation = ExteriorSaturation;
	CurrentContrast = ExteriorContrast;
	CurrentBloom = ExteriorBloom;
	CurrentVignette = 0.f;
	CurrentTemperature = ExteriorTemperature;
	CurrentGrain = 0.f;
}

void UAlsasuaZonePostProcess::Tick(float DeltaTime)
{
	ApplyZoneBlending(DeltaTime);
	UpdatePostProcessVolume(DeltaTime);
}

void UAlsasuaZonePostProcess::RegisterZone(const FZoneColorGrading& Zone)
{
	RegisteredZones.Add(Zone);
}

void UAlsasuaZonePostProcess::UnregisterZone(FName ZoneID)
{
	RegisteredZones.RemoveAll([&ZoneID](const FZoneColorGrading& Z) { return Z.ZoneID == ZoneID; });
}

void UAlsasuaZonePostProcess::SetCurrentZoneType(EZoneType Type)
{
	if (Type != CurrentZoneType)
	{
		PreviousZoneType = CurrentZoneType;
		CurrentZoneType = Type;
		BlendAlpha = 0.f;
	}
}

void UAlsasuaZonePostProcess::LoadNeighborhoodZones()
{
	const FString JsonPath = FPaths::ProjectContentDir() / TEXT("Datos/nighborhoods.json");
	FString JsonContent;
	if (!FFileHelper::LoadFileToString(JsonContent, *JsonPath)) return;

	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) return;

	const TArray<TSharedPtr<FJsonValue>>* BarriosPtr;
	if (!Root->TryGetArrayField(TEXT("barrios"), BarriosPtr)) return;

	for (const auto& BVal : *BarriosPtr)
	{
		const TSharedPtr<FJsonObject> Barrio = BVal->AsObject();
		if (!Barrio.IsValid()) continue;

		FZoneColorGrading Zone;
		Zone.ZoneID = FName(*Barrio->GetStringField(TEXT("id")));

		const TSharedPtr<FJsonObject>& Centro = Barrio->GetObjectField(TEXT("centro"));
		if (Centro.IsValid())
		{
			double Cx = 0, Cz = 0;
			Centro->TryGetNumberField(TEXT("x"), Cx);
			Centro->TryGetNumberField(TEXT("z"), Cz);
			Zone.Center = FVector2D(Cx, Cz);
		}

		Zone.Radius = Barrio->GetNumberField(TEXT("radio_m"));

		const FString Tipo = Barrio->GetStringField(TEXT("tipo"));
		if (Tipo == TEXT("Centro"))
		{
			Zone.SaturationBoost = 0.05f;
			Zone.ContrastBoost = 0.1f;
			Zone.Bloom = 0.1f;
			Zone.Grain = 0.01f;
		}
		else if (Tipo == TEXT("Residencial"))
		{
			Zone.SaturationBoost = -0.05f;
			Zone.ContrastBoost = 0.f;
			Zone.Bloom = 0.f;
		}
		else if (Tipo == TEXT("Ensanche"))
		{
			Zone.SaturationBoost = 0.f;
			Zone.ContrastBoost = 0.05f;
			Zone.Bloom = 0.05f;
		}
		else if (Tipo == TEXT("Industrial"))
		{
			Zone.SaturationBoost = -0.15f;
			Zone.ContrastBoost = 0.15f;
			Zone.Grain = 0.03f;
			Zone.TemperatureTint = FLinearColor(0.85f, 0.9f, 1.0f, 1.0f);
		}

		RegisteredZones.Add(MoveTemp(Zone));
	}

	UE_LOG(LogTemp, Log, TEXT("[ZonePostProcess] %d barrios cargados"), RegisteredZones.Num());
}

void UAlsasuaZonePostProcess::ApplyZoneBlending(float DeltaTime)
{
	APlayerController* PC = nullptr;
	if (UWorld* W = GetWorld())
	{
		PC = W->GetFirstPlayerController();
	}
	if (!PC || !PC->GetPawn()) return;

	const FVector PlayerLoc = PC->GetPawn()->GetActorLocation();
	const FVector2D PlayerPos2D(PlayerLoc.X, PlayerLoc.Y);

	FName NearestZone;
	float NearestDistSq = MAX_FLT;

	for (const FZoneColorGrading& Zone : RegisteredZones)
	{
		const float DistSq = FVector2D::DistSquared(PlayerPos2D, Zone.Center);
		if (DistSq < NearestDistSq && DistSq < FMath::Square(Zone.Radius))
		{
			NearestDistSq = DistSq;
			NearestZone = Zone.ZoneID;
		}
	}

	if (!NearestZone.IsNone() && NearestZone != ActiveZoneID)
	{
		PreviousZoneID = ActiveZoneID;
		ActiveZoneID = NearestZone;
		BlendAlpha = 0.f;

		for (const FZoneColorGrading& Zone : RegisteredZones)
		{
			if (Zone.ZoneID == ActiveZoneID)
			{
				SetCurrentZoneType(EZoneType::Interior);
				break;
			}
		}
	}
	else if (NearestZone.IsNone() && !ActiveZoneID.IsNone())
	{
		PreviousZoneID = ActiveZoneID;
		ActiveZoneID = NAME_None;
		BlendAlpha = 0.f;
		SetCurrentZoneType(EZoneType::Exterior);
	}
}

void UAlsasuaZonePostProcess::UpdatePostProcessVolume(float DeltaTime)
{
    // Day/night factor from sun elevation (0 = night, 1 = day), via the
    // published "Clima.TiempoDelDia" contract; day if the plugin is asleep.
    float DayFactor = 1.f;
    if (UWorld* W = GetWorld())
    {
        UAlsasuaServiceRegistry* Registro = UAlsasuaServiceRegistry::Get(W);
        ITimeOfDayService* Tiempo = Registro ? Registro->PedirComo<ITimeOfDayService>("Clima.TiempoDelDia") : nullptr;
        if (Tiempo)
        {
            DayFactor = FMath::Clamp(Tiempo->GetSunPitch() / 10.f, 0.f, 1.f);
        }
    }

    // Exterior base values modulated by day/night.
    float TargetS = FMath::Lerp(NightSaturation, ExteriorSaturation, DayFactor);
    float TargetC = ExteriorContrast;
    float TargetB = FMath::Lerp(NightBloom, ExteriorBloom, DayFactor);
    float TargetV = FMath::Lerp(NightVignette, 0.f, DayFactor);
    float TargetT = FMath::Lerp(NightTemperature, ExteriorTemperature, DayFactor);
    float TargetG = 0.f;

    switch (CurrentZoneType)
    {
    case EZoneType::Exterior:
        TargetS = ExteriorSaturation;
        TargetC = ExteriorContrast;
        TargetB = ExteriorBloom;
        TargetV = 0.f;
        TargetT = ExteriorTemperature;
        TargetG = 0.f;
        break;
    case EZoneType::Interior:
        TargetS = InteriorSaturation;
        TargetC = InteriorContrast;
        TargetB = InteriorBloom;
        TargetV = InteriorVignette;
        TargetT = InteriorTemperature;
        TargetG = 0.f;
        break;
    case EZoneType::InteriorOscuro:
        TargetS = DarkSaturation;
        TargetC = DarkContrast;
        TargetB = 0.f;
        TargetV = DarkVignette;
        TargetT = DarkTemperature;
        TargetG = DarkGrain;
        break;
    case EZoneType::Sotano:
        TargetS = SotanoSaturation;
        TargetC = 1.5f;
        TargetB = 0.f;
        TargetV = SotanoVignette;
        TargetT = SotanoTemperature;
        TargetG = SotanoGrain;
        break;
    }

    for (const FZoneColorGrading& Zone : RegisteredZones)
    {
        if (Zone.ZoneID == ActiveZoneID)
        {
            TargetS += Zone.SaturationBoost;
            TargetC += Zone.ContrastBoost;
            TargetB += Zone.Bloom;
            TargetV += Zone.VignetteIntensity;
            TargetG += Zone.Grain;
            break;
        }
    }

    const float Speed = FMath::Max(BlendSpeed, 0.1f);
    CurrentSaturation = FMath::FInterpTo(CurrentSaturation, TargetS, DeltaTime, Speed);
    CurrentContrast = FMath::FInterpTo(CurrentContrast, TargetC, DeltaTime, Speed);
    CurrentBloom = FMath::FInterpTo(CurrentBloom, TargetB, DeltaTime, Speed);
    CurrentVignette = FMath::FInterpTo(CurrentVignette, TargetV, DeltaTime, Speed);
    CurrentTemperature = FMath::FInterpTo(CurrentTemperature, TargetT, DeltaTime, Speed);
    CurrentGrain = FMath::FInterpTo(CurrentGrain, TargetG, DeltaTime, Speed);

    // Write computed values to the first PostProcessVolume.
    UWorld* W = GetWorld();
    if (!W) return;

    TArray<AActor*> PPVols;
    UGameplayStatics::GetAllActorsOfClass(W, APostProcessVolume::StaticClass(), PPVols);
    APostProcessVolume* PPV = PPVols.Num() > 0 ? Cast<APostProcessVolume>(PPVols[0]) : nullptr;
    if (!PPV) return;

    FPostProcessSettings& S = PPV->Settings;

    // Temperature tint as ColorSaturation override (warm=boost R/G, cold=boost B).
    const float T = CurrentTemperature / 6500.f;
    const FLinearColor TempTint(T > 1.f ? 1.f : T * 1.1f, T > 1.f ? T * 0.95f : 1.f, 1.f / (T * 0.3f + 0.7f));

    S.bOverride_ColorSaturation = true;
    S.ColorSaturation = FLinearColor(CurrentSaturation, CurrentSaturation, CurrentSaturation, 1.f) * TempTint;

    S.bOverride_ColorContrast = true;
    const float C = CurrentContrast;
    S.ColorContrast = FLinearColor(C, C, C, 1.f);

    S.bOverride_BloomIntensity = true;
    S.BloomIntensity = CurrentBloom;

    S.bOverride_VignetteIntensity = true;
    S.VignetteIntensity = CurrentVignette;

    S.bOverride_FilmGrainIntensity = true;
    S.FilmGrainIntensity = CurrentGrain;
}

float UAlsasuaZonePostProcess::TemperatureToTint(float Kelvin) const
{
	const float Temp = Kelvin / 100.0f;
	float R, G, B;

	if (Temp <= 66.0f)
	{
		R = 1.0f;
		G = FMath::Clamp(99.4708025861f * FMath::Loge(Temp) - 161.1195681661f, 0.0f, 255.0f) / 255.0f;
		B = Temp <= 19.0f ? 0.0f : FMath::Clamp(138.5177312231f * FMath::Loge(Temp - 10.0f) - 305.0447927307f, 0.0f, 255.0f) / 255.0f;
	}
	else
	{
		R = FMath::Clamp(329.698727446f * FMath::Pow(Temp - 60.0f, -0.1332047592f), 0.0f, 255.0f) / 255.0f;
		G = FMath::Clamp(288.1221695283f * FMath::Pow(Temp - 60.0f, -0.0755148492f), 0.0f, 255.0f) / 255.0f;
		B = 1.0f;
	}

	return (R + G + B) / 3.0f;
}
