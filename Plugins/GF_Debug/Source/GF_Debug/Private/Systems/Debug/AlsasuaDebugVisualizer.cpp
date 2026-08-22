// AlsasuaDebugVisualizer.cpp
// Capa de depuración visual: tráfico, semáforos, biomas, chunks y vehículos,
// cada uno con su par DebugShow*/DebugHide* de consola. Dibuja a 2 Hz para no
// comerse el frame rate cuando todo está encendido.
#include "Systems/Debug/AlsasuaDebugVisualizer.h"
#include "DrawDebugHelpers.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "World/AlsasuaDynamicTrafficSystem.h"
#include "World/AlsasuaTrafficLightSystem.h"

namespace
{
	// Flags estáticos: las consolas no conocen instancias, y que sobrevivan al
	// PIE es justo lo que se quiere de un debug.
	bool bShowTraffic = false;
	bool bShowLights = false;
	bool bShowBiomes = false;
	bool bShowChunks = false;
	bool bShowVehicles = false;

	/** Vida de los primitivos: algo más que el periodo de 0.5 s para que no
	 *  parpadeen entre volcados. */
	constexpr float Duracion = 0.55f;

	FAutoConsoleCommand CShowTraffic(TEXT("DebugShowTraffic"), TEXT("Dibuja rutas y vehículos de tráfico"), FConsoleCommandDelegate::CreateLambda([] { bShowTraffic = true; }));
	FAutoConsoleCommand CHideTraffic(TEXT("DebugHideTraffic"), TEXT("Oculta el debug de tráfico"), FConsoleCommandDelegate::CreateLambda([] { bShowTraffic = false; }));
	FAutoConsoleCommand CShowLights(TEXT("DebugShowLights"), TEXT("Dibuja semáforos con su fase"), FConsoleCommandDelegate::CreateLambda([] { bShowLights = true; }));
	FAutoConsoleCommand CHideLights(TEXT("DebugHideLights"), TEXT("Oculta el debug de semáforos"), FConsoleCommandDelegate::CreateLambda([] { bShowLights = false; }));
	FAutoConsoleCommand CShowBiomes(TEXT("DebugShowBiomes"), TEXT("Dibuja la rejilla de biomas"), FConsoleCommandDelegate::CreateLambda([] { bShowBiomes = true; }));
	FAutoConsoleCommand CHideBiomes(TEXT("DebugHideBiomes"), TEXT("Oculta el debug de biomas"), FConsoleCommandDelegate::CreateLambda([] { bShowBiomes = false; }));
	FAutoConsoleCommand CShowChunks(TEXT("DebugShowChunks"), TEXT("Dibuca los límites de chunk de World Partition"), FConsoleCommandDelegate::CreateLambda([] { bShowChunks = true; }));
	FAutoConsoleCommand CHideChunks(TEXT("DebugHideChunks"), TEXT("Oculta el debug de chunks"), FConsoleCommandDelegate::CreateLambda([] { bShowChunks = false; }));
	FAutoConsoleCommand CShowVehicles(TEXT("DebugShowVehicles"), TEXT("Dibuca velocidad y colas de vehículos"), FConsoleCommandDelegate::CreateLambda([] { bShowVehicles = true; }));
	FAutoConsoleCommand CHideVehicles(TEXT("DebugHideVehicles"), TEXT("Oculta el debug de vehículos"), FConsoleCommandDelegate::CreateLambda([] { bShowVehicles = false; }));
}

void UAlsasuaDebugVisualizer::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

bool UAlsasuaDebugVisualizer::AlgoActivo()
{
	return bShowTraffic || bShowLights || bShowBiomes || bShowChunks || bShowVehicles;
}

bool UAlsasuaDebugVisualizer::IsTickable() const
{
	const UWorld* Mundo = GetWorld();
	if (IsTemplate() || !Mundo || !Mundo->HasBegunPlay())
	{
		return false;
	}
	// Los flags viven en el anónimo de arriba; esta comprobación barata evita
	// acumular el reloj cuando nada está encendido.
	return bShowTraffic || bShowLights || bShowBiomes || bShowChunks || bShowVehicles;
}

UAlsasuaDynamicTrafficSystem* UAlsasuaDebugVisualizer::ObtenerTrafico() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UAlsasuaDynamicTrafficSystem>() : nullptr;
}

void UAlsasuaDebugVisualizer::Tick(float DeltaTime)
{
	DesdeUltimaPasada += DeltaTime;
	if (DesdeUltimaPasada < 0.5f)
	{
		return;
	}
	DesdeUltimaPasada = 0.f;

	UWorld* Mundo = GetWorld();
	if (!Mundo)
	{
		return;
	}

	if (bShowTraffic)  DibujarTrafico();
	if (bShowLights)   DibujarSemaforos();
	if (bShowBiomes)   DibujarBiomas();
	if (bShowChunks)   DibujarChunks();
	if (bShowVehicles) DibujarVehiculos();
}

void UAlsasuaDebugVisualizer::DibujarTrafico()
{
	UWorld* Mundo = GetWorld();
	UAlsasuaDynamicTrafficSystem* Trafico = ObtenerTrafico();
	if (!Trafico)
	{
		return; // sin subsistema de tráfico, silencio
	}

	for (const FVehiclePath& Vehiculo : Trafico->GetVehiculos())
	{
		const FColor Color = Vehiculo.ColorCarroceria.ToFColor(true);

		// La ruta restante del vehículo, elevada un pelo para no pelearse con el asfalto.
		for (int32 i = 1; i < Vehiculo.Puntos.Num(); ++i)
		{
			DrawDebugLine(Mundo, Vehiculo.Puntos[i - 1] + FVector(0, 0, 100.f), Vehiculo.Puntos[i] + FVector(0, 0, 100.f), Color, false, Duracion, 0, 4.f);
		}

		FVector Pos = Vehiculo.ActorAsociado.IsValid()
			? Vehiculo.ActorAsociado->GetActorLocation()
			: (Vehiculo.Puntos.Num() > 0 ? Vehiculo.Puntos.Last() : FVector::ZeroVector);
		if (!Pos.IsZero())
		{
			DrawDebugSphere(Mundo, Pos + FVector(0, 0, 50.f), 60.f, 8, Color, false, Duracion);
		}
	}
}

void UAlsasuaDebugVisualizer::DibujarSemaforos()
{
	UWorld* Mundo = GetWorld();
	UGameInstance* GI = GetGameInstance();
	UAlsasuaTrafficLightSystem* Luces = GI ? GI->GetSubsystem<UAlsasuaTrafficLightSystem>() : nullptr;
	if (!Luces)
	{
		return;
	}

	for (const FTrafficLight& Semaforo : Luces->GetSemaforos())
	{
		FColor Color;
		switch (Semaforo.Fase)
		{
		case EFaseSemaforo::Verde: Color = FColor::Green; break;
		case EFaseSemaforo::Ambar: Color = FColor(255, 165, 0); break;
		default:                   Color = FColor::Red; break;
		}

		DrawDebugSphere(Mundo, Semaforo.Posicion + FVector(0, 0, 300.f), 80.f, 10, Color, false, Duracion);

		const FString Texto = FString::Printf(TEXT("%s — %s"), *Semaforo.Calle, *UEnum::GetDisplayValueAsText(Semaforo.Fase).ToString());
		DrawDebugString(Mundo, Semaforo.Posicion + FVector(0, 0, 450.f), Texto, nullptr, FColor::White, Duracion, true);
	}
}

void UAlsasuaDebugVisualizer::DibujarBiomas()
{
	// No existe aún UBiomeSubsystem en el proyecto, así que siempre cae al
	// fallback por altura: bandas de color según la Z del suelo. Cuando el
	// subsystema exista, basta pedirle aquí la muestra antes del trace.
	UWorld* Mundo = GetWorld();
	APlayerController* PC = Mundo ? Mundo->GetFirstPlayerController() : nullptr;
	AActor* Vista = PC && PC->GetViewTarget() ? PC->GetViewTarget() : nullptr;
	if (!Vista)
	{
		return;
	}
	const FVector Centro = Vista->GetActorLocation();

	constexpr int32 Lado = 9;          // rejilla 9x9 alrededor de la cámara
	constexpr float Paso = 2000.f;     // 20 m entre muestras
	constexpr float AltoMuestra = 4000.f;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(DbgBioma), /*bTraceComplex*/ false);
	auto ColorPorAltura = [](float Z)
	{
		if (Z < 200.f)   return FColor(60, 120, 220);    // agua
		if (Z < 600.f)   return FColor(120, 200, 90);    // pradera baja
		if (Z < 1200.f)  return FColor(40, 150, 70);     // bosque
		if (Z < 1800.f)  return FColor(190, 170, 90);    // matorral / tierra
		if (Z < 2600.f)  return FColor(130, 130, 130);   // roca
		return FColor::White;                            // nieve
	};

	for (int32 gx = -Lado / 2; gx <= Lado / 2; ++gx)
	{
		for (int32 gy = -Lado / 2; gy <= Lado / 2; ++gy)
		{
			const FVector Base(Centro.X + gx * Paso, Centro.Y + gy * Paso, Centro.Z + AltoMuestra);
			FHitResult Golpe;
			if (!Mundo->LineTraceSingleByChannel(Golpe, Base, Base - FVector(0, 0, AltoMuestra * 2.f), ECC_Visibility, Params))
			{
				continue;
			}
			DrawDebugSolidBox(Mundo, Golpe.ImpactPoint + FVector(0, 0, 25.f), FVector(Paso * 0.45f, Paso * 0.45f, 25.f), ColorPorAltura(Golpe.ImpactPoint.Z), false, Duracion);
		}
	}
}

void UAlsasuaDebugVisualizer::DibujarChunks()
{
	// ponytail: rejilla de celda fija (512 m, la celda por defecto de World
	// Partition); si algún día se cambia CellSize en el WorldPartition, ajustar aquí.
	UWorld* Mundo = GetWorld();
	APlayerController* PC = Mundo ? Mundo->GetFirstPlayerController() : nullptr;
	AActor* Vista = PC && PC->GetViewTarget() ? PC->GetViewTarget() : nullptr;
	if (!Vista)
	{
		return;
	}
	const FVector Centro = Vista->GetActorLocation();

	constexpr float Celda = 51200.f; // 512 m en cm
	const int32 CX = FMath::FloorToInt(Centro.X / Celda);
	const int32 CY = FMath::FloorToInt(Centro.Y / Celda);

	for (int32 x = CX - 1; x <= CX + 1; ++x)
	{
		for (int32 y = CY - 1; y <= CY + 1; ++y)
		{
			const FVector CentroCelda((x + 0.5f) * Celda, (y + 0.5f) * Celda, Centro.Z);
			DrawDebugBox(Mundo, CentroCelda, FVector(Celda * 0.5f, Celda * 0.5f, 2048.f), FColor::Cyan, false, Duracion, 0, 3.f);
		}
	}
}

void UAlsasuaDebugVisualizer::DibujarVehiculos()
{
	UWorld* Mundo = GetWorld();
	UAlsasuaDynamicTrafficSystem* Trafico = ObtenerTrafico();
	if (!Trafico)
	{
		return;
	}

	// Colas: vehículos detenidos agrupados por calle, conectados en fila.
	TMap<FString, TArray<const FVehiclePath*, TInlineAllocator<8>>> Colas;

	for (const FVehiclePath& Vehiculo : Trafico->GetVehiculos())
	{
		FVector Pos = Vehiculo.ActorAsociado.IsValid()
			? Vehiculo.ActorAsociado->GetActorLocation()
			: (Vehiculo.Puntos.Num() > 0 ? Vehiculo.Puntos.Last() : FVector::ZeroVector);
		if (Pos.IsZero())
		{
			continue;
		}

		DrawDebugString(Mundo, Pos + FVector(0, 0, 180.f),
			FString::Printf(TEXT("%.0f cm/s%s"), Vehiculo.Velocidad, Vehiculo.bEnMarcha ? TEXT("") : TEXT(" (parado)")),
			nullptr, FColor::White, Duracion, true);

		if (!Vehiculo.bEnMarcha && !Vehiculo.Calle.IsEmpty())
		{
			Colas.FindOrAdd(Vehiculo.Calle).Add(&Vehiculo);
		}
	}

	for (const auto& Par : Colas)
	{
		const TArray<const FVehiclePath*, TInlineAllocator<8>>& Cola = Par.Value;
		for (int32 i = 1; i < Cola.Num(); ++i)
		{
			auto PosDe = [](const FVehiclePath* V)
			{
				return V->ActorAsociado.IsValid() ? V->ActorAsociado->GetActorLocation()
					: (V->Puntos.Num() > 0 ? V->Puntos.Last() : FVector::ZeroVector);
			};
			DrawDebugLine(Mundo, PosDe(Cola[i - 1]) + FVector(0, 0, 80.f), PosDe(Cola[i]) + FVector(0, 0, 80.f), FColor::Red, false, Duracion, 0, 2.f);
		}
	}
}
