// MegaMarchSubsystem.cpp
// La manifestación de Megasion: cubos cálidos que crecen hasta 200 y marchan
// en círculo lento alrededor del centro dado, con bob vertical para que no
// parezcan una fila de muebles.
#include "Systemics/Megasion/MegaMarchSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	// Geometría de la formación: rejilla cuadrada bajo el radio dado.
	constexpr float SeparacionSlot = 150.f;   // cm entre manifestantes
	constexpr float JitterRadio = 100.f;      // cm de dispersión aleatoria
	constexpr float AmplitudBob = 15.f;       // cm de sube-baja
	constexpr float AmplitudWobble = 0.03f;   // rad de vaivén angular

	constexpr int32 SpawnsPorFrame = 10;      // techo para no engullir un frame al arrancar

	FVector PosicionSlot(int32 Indice, int32 Columnas)
	{
		const int32 Col = Indice % Columnas;
		const int32 Fila = Indice / Columnas;
		const float Mitad = (Columnas - 1) * 0.5f;
		return FVector((Col - Mitad) * SeparacionSlot, (Fila - Mitad) * SeparacionSlot, 0.f);
	}
}

void UMegaMarchSubsystem::StartMegaMarch(FVector Center)
{
	if (bActive)
	{
		StopMegaMarch();
	}

	UWorld* Mundo = GetWorld();
	if (!Mundo || !Mundo->HasBegunPlay())
	{
		return; // sin mundo en juego no hay spawn posible
	}

	if (!MeshCubo)
	{
		MeshCubo = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	}
	if (!MaterialBase)
	{
		MaterialBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}
	if (!MeshCubo)
	{
		UE_LOG(LogTemp, Warning, TEXT("MegaMarch: no se encontró /Engine/BasicShapes/Cube.Cube"));
		return;
	}

	ProtestCenter = Center;
	TiempoTranscurrido = 0.f;
	AnguloFormacion = 0.f;
	bActive = true;

	SpawnMarchador(0); // al menos uno visible desde el primer frame
}

void UMegaMarchSubsystem::StopMegaMarch()
{
	for (AActor* Manifestante : ProtesterActors)
	{
		if (IsValid(Manifestante))
		{
			Manifestante->Destroy();
		}
	}
	ProtesterActors.Empty();
	Marchers.Empty();
	bActive = false;
}

bool UMegaMarchSubsystem::SpawnMarchador(int32 Indice)
{
	UWorld* Mundo = GetWorld();
	if (!Mundo || !MeshCubo || Indice >= MaxProtesters)
	{
		return false;
	}

	const int32 Columnas = FMath::Max(1, FMath::CeilToInt(FMath::Sqrt((float)MaxProtesters)));
	const FVector Slot = PosicionSlot(Indice, Columnas);

	// Slot de rejilla convertido a polar con dispersión aleatoria: así la
	// formación gira como bloque pero nadie está clavado a su vecino.
	const float Radio = FMath::Max(Slot.Size2D() + FMath::FRandRange(-JitterRadio, JitterRadio), 1.f);
	const float Angulo = FMath::Atan2(Slot.Y, Slot.X) + FMath::FRandRange(-AmplitudWobble, AmplitudWobble);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AStaticMeshActor* Manifestante = Mundo->SpawnActor<AStaticMeshActor>(
		ProtestCenter + FVector(FMath::Cos(Angulo) * Radio, FMath::Sin(Angulo) * Radio, ProtestCenter.Z),
		FRotator::ZeroRotator, Params);
	if (!Manifestante)
	{
		return false;
	}

	UStaticMeshComponent* Malla = Manifestante->GetStaticMeshComponent();
	Malla->SetMobility(EComponentMobility::Movable);
	Malla->SetStaticMesh(MeshCubo);
	Malla->SetWorldScale3D(FVector(FMath::FRandRange(0.7f, 1.3f)));
	Malla->SetSimulatePhysics(false);

	// Color cálido: tono 0-60 grados recorre rojo → amarillo.
	if (UMaterialInstanceDynamic* MID = Malla->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MaterialBase))
	{
		MID->SetVectorParameterValue(TEXT("Color"),
			FLinearColor::MakeFromHSV8(FMath::FRandRange(0.f, 60.f), 0.85f, 0.95f));
	}

	ProtesterActors.Add(Manifestante);
	Marchers.Add({Angulo, Radio, FMath::FRandRange(0.f, 2.f * PI)});
	return true;
}

void UMegaMarchSubsystem::Tick(float DeltaTime)
{
	TickMarch(DeltaTime);
}

float UMegaMarchSubsystem::GetProtestIntensity() const
{
	if (!bActive || MaxProtesters <= 0)
	{
		return 0.f;
	}
	return FMath::Clamp((float)ProtesterActors.Num() / (float)MaxProtesters, 0.f, 1.f);
}

void UMegaMarchSubsystem::TickMarch(float DeltaTime)
{
	if (!bActive)
	{
		return;
	}
	TiempoTranscurrido += DeltaTime;

	// Crecimiento: la multitud llega escalonada durante los primeros segundos.
	const float Fraccion = FMath::Clamp(TiempoTranscurrido / SegundosCrecimiento, 0.f, 1.f);
	const int32 Objetivo = FMath::Min(MaxProtesters, FMath::FloorToInt(MaxProtesters * Fraccion));
	int32 Presupuesto = SpawnsPorFrame;
	while (ProtesterActors.Num() < Objetivo && Presupuesto-- > 0 && SpawnMarchador(ProtesterActors.Num()))
	{
	}

	// Velocidad tangencial constante → velocidad angular v/r.
	AnguloFormacion += (MarchSpeed / FMath::Max(ProtestRadius, 1.f)) * DeltaTime;

	for (int32 i = 0; i < Marchers.Num(); ++i)
	{
		AActor* Manifestante = ProtesterActors[i].Get();
		if (!IsValid(Manifestante))
		{
			continue;
		}
		const FMarcher& M = Marchers[i];
		const float Angulo = AnguloFormacion + M.Angulo + FMath::Sin(TiempoTranscurrido * 0.7f + M.FaseBob) * AmplitudWobble;
		const float Bob = FMath::Sin(TiempoTranscurrido * 2.f + M.FaseBob) * AmplitudBob;

		Manifestante->SetActorLocation(
			ProtestCenter + FVector(FMath::Cos(Angulo) * M.Radio, FMath::Sin(Angulo) * M.Radio, Bob));
	}
}
