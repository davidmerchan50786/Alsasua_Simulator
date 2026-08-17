// CargadorEdificios.cpp
#include "CargadorEdificios.h"
#include "EdificioGenerado.h"
#include "ArranqueMundo.h"
#include "GeoDataAlsasua.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "CollisionQueryParams.h"
#include "HAL/PlatformTime.h"
#include "Math/RandomStream.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "MuestreadorAltura.h"
#include "CargarMaterialComun.h"
#include "AlturasLidarComun.h"
#include "World/AlsasuaFacadeGenerator.h"
#include "Engine/GameInstance.h"


// Paleta vasca por edificio (determinista por id): arenisca rojiza en muros,
// teja terracota o pizarra en tejados.
static FColor MuroDe(int32 Id)
{
	FRandomStream r(Id * 2654435761u + 11);
	return FColor(175 + r.RandRange(0, 45), 120 + r.RandRange(0, 40), 95 + r.RandRange(0, 35), 255);
}
static FColor TejadoDe(int32 Id)
{
	FRandomStream r(Id * 40503u + 7);
	if (r.GetFraction() < 0.72f)  // teja árabe
		return FColor(125 + r.RandRange(0, 55), 58 + r.RandRange(0, 30), 38 + r.RandRange(0, 26), 255);
	return FColor(60 + r.RandRange(0, 26), 66 + r.RandRange(0, 20), 74 + r.RandRange(0, 20), 255);  // pizarra
}
static UMaterialInterface* CargarMaterialEdificio()
{
	// Fachada con ventanas nocturnas si existe; si no, el material de suelo.
	if (UMaterialInterface* F = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Fachada.M_Fachada")))
		return F;
	return CargarMaterialConFallbackSeguro(
		TEXT("/Game/Materiales/M_Edificio.M_Edificio"),
		TEXT("/Game/Materiales/M_Edificio.M_Edificio"),
		TEXT("/Game/Materiales/M_Edificio.M_Edificio"));
}

// Color de fachada real (mat_r/g/b, 0..1) -> FColor; si falta, paleta por id.
static FColor MuroReal(const TSharedPtr<FJsonObject>& O, int32 Id)
{
	if (O->HasField(TEXT("mat_r")))
		return FLinearColor(
			(float)O->GetNumberField(TEXT("mat_r")),
			(float)O->GetNumberField(TEXT("mat_g")),
			(float)O->GetNumberField(TEXT("mat_b"))).ToFColor(true);   // lineal -> sRGB
	return MuroDe(Id);
}

// Color de tejado real (roof_*_real, 0..255) -> FColor; si falta, paleta por id.
static FColor TejadoReal(const TSharedPtr<FJsonObject>& O, int32 Id)
{
	if (O->HasField(TEXT("roof_r_real")))
		return FColor((uint8)O->GetIntegerField(TEXT("roof_r_real")),
		              (uint8)O->GetIntegerField(TEXT("roof_g_real")),
		              (uint8)O->GetIntegerField(TEXT("roof_b_real")), 255);
	return TejadoDe(Id);
}

// Forma LIDAR real -> enum + escala vertical del tejado.
static EFormaTejado FormaReal(const TSharedPtr<FJsonObject>& O, float& OutEscala)
{
	OutEscala = 1.f;
	const FString F = O->HasField(TEXT("lidar_forma")) ? O->GetStringField(TEXT("lidar_forma")) : FString();
	if (F == TEXT("flat"))         return EFormaTejado::Plano;
	if (F == TEXT("hipped"))       return EFormaTejado::Cuatro_Aguas;
	if (F == TEXT("steep_gabled")) { OutEscala = 1.7f; return EFormaTejado::Dos_Aguas; }
	if (F == TEXT("gabled"))       return EFormaTejado::Dos_Aguas;
	return EFormaTejado::Cuatro_Aguas;   // None/desconocido
}

// Fachada real del edificio, o null. El subsistema se busca una vez: es de
// GameInstance y GetSubsystem no es gratis por cada uno de los 1030 edificios.
const FBuildingFacadeEntry* UCargadorEdificios::FachadaDe(int32 IdEdificio) const
{
	if (!bFachadasBuscadas)
	{
		bFachadasBuscadas = true;
		if (const UWorld* W = GetWorld())
		{
			if (const UGameInstance* GI = W->GetGameInstance())
			{
				GenFachadas = GI->GetSubsystem<UAlsasuaFacadeGenerator>();
			}
		}
	}
	return GenFachadas ? GenFachadas->De(IdEdificio) : nullptr;
}

void UCargadorEdificios::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// La construye ADirectorArranque tras generar el terreno; aquí aún no existe (cota 0).
}

// Contador del log de abajo. Es estático de unidad de traducción, así que
// sobrevive entre sesiones de PIE en el mismo proceso del editor: se reinicia en
// PrepararCarga() o el resumen iría sumando el de la partida anterior.
namespace { int32 GSustituidas = 0; }


float UCargadorEdificios::AlturaSuelo(const FVector2D& XY) const
{
	const UWorld* W = GetWorld();
	if (!W) return 0.f;
	if (const UMuestreadorAltura* Muestreador = W->GetSubsystem<UMuestreadorAltura>())
	{
		const float Altura = Muestreador->AlturaMundo(FVector(XY.X, XY.Y, 0.f));
		if (!FMath::IsNearlyZero(Altura)) return Altura;
	}
	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(AlturaSuelo), true);
	if (W->LineTraceSingleByChannel(Hit, FVector(XY.X, XY.Y, UAlsasuaGeoData::TraceUp), FVector(XY.X, XY.Y, UAlsasuaGeoData::TraceDown), ECC_Visibility, Q))
		return Hit.Location.Z;
	return 0.f;
}

void UCargadorEdificios::PrepararCarga()
{
	if (bPreparado) return;
	bPreparado = true;
	GSustituidas = 0;

	const FString Ruta = FPaths::Combine(FPaths::ProjectContentDir(), RutaRelativa);
	FString Texto;
	if (!FFileHelper::LoadFileToString(Texto, *Ruta))
	{ UE_LOG(LogTemp, Error, TEXT("[Edificios] no pude leer %s"), *Ruta); return; }

	const TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Texto);
	if (!FJsonSerializer::Deserialize(R, Items))
	{ UE_LOG(LogTemp, Error, TEXT("[Edificios] JSON inválido")); Items.Empty(); }
}

void UCargadorEdificios::ConstruirUno(const TSharedPtr<FJsonObject>& O)
{
	if (!O.IsValid()) return;
	const TArray<TSharedPtr<FJsonValue>>* Vs = nullptr;
	if (!O->TryGetArrayField(TEXT("vertices"), Vs) || !Vs || Vs->Num() < 3) return;

	TArray<FVector2D> MundoXY; MundoXY.Reserve(Vs->Num());
	FVector2D Centro(0, 0);
	for (const TSharedPtr<FJsonValue>& Pv : *Vs)
	{
		const TSharedPtr<FJsonObject> Po = Pv->AsObject();
		if (!Po.IsValid()) continue;
		// buildings_final.json es RELATIVO a Herriko Plaza: sumar OX/OZ -> mundo Unity absoluto.
		const FVector M = UAlsasuaGeoData::RelLocalToUE5(FVector(Po->GetNumberField(TEXT("x")), 0.0, Po->GetNumberField(TEXT("z"))));
		MundoXY.Add(FVector2D(M.X, M.Y));
		Centro += FVector2D(M.X, M.Y);
	}
	if (MundoXY.Num() < 3) return;
	Centro /= MundoXY.Num();

	double AlturaM = O->HasField(TEXT("height")) ? O->GetNumberField(TEXT("height")) : 6.0;
	int32 PlantasJson = O->HasField(TEXT("levels")) ? (int32)O->GetNumberField(TEXT("levels")) : 1;

	// Si hay huella medida por LiDAR a menos de 15 m, manda ella: es medida real
	// del vuelo de 2017 frente a la estimación de OSM, que se queda ~3 m corta.
	{
		float AltLidar = 0.f;
		int32 PlantasLidar = 0;
		if (AlturasLidar::Buscar(Centro, AltLidar, PlantasLidar))
		{
			AlturaM = AltLidar;
			PlantasJson = PlantasLidar;
			++GSustituidas;
		}
	}

	const float  Suelo   = AlturaSuelo(Centro) + 8.f;   // alzado sobre el terreno (anti z-fighting)

	TArray<FVector2D> Local; Local.Reserve(MundoXY.Num());
	for (const FVector2D& V : MundoXY) Local.Add(V - Centro);

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AEdificioGenerado* E = GetWorld()->SpawnActor<AEdificioGenerado>(
		AEdificioGenerado::StaticClass(), FVector(Centro.X, Centro.Y, Suelo), FRotator::ZeroRotator, SP);
	if (!E) return;

	E->Id            = (int32)O->GetIntegerField(TEXT("id"));
	E->NombreEdificio = O->HasField(TEXT("name")) ? O->GetStringField(TEXT("name")) : FString();
	O->TryGetStringField(TEXT("barrio"), E->Barrio);
	E->Plantas       = PlantasJson;   // ya lleva la medida LiDAR si la había

	// Eje real del caballete (Unity dx,dz) -> dirección en el plano XY de Unreal (X=Z, Y=X).
	FVector2D Eje(1, 0);
	if (O->HasField(TEXT("lidar_eje_x")) && O->HasField(TEXT("lidar_eje_z")))
		Eje = FVector2D(O->GetNumberField(TEXT("lidar_eje_z")), O->GetNumberField(TEXT("lidar_eje_x")));

	float EscalaTejado = 1.f;
	const EFormaTejado Forma = FormaReal(O, EscalaTejado);

	// Edificios importantes (con nombre, >2 plantas, o >8m) usan fachada detallada.
	const bool bImportante = !E->NombreEdificio.IsEmpty() || E->Plantas > 2 || AlturaM > 8.0;
	const FColor ColorTejado = TejadoReal(O, E->Id);

	if (bImportante)
	{
		E->bDetalleActivo = true;

		// Configuración adaptativa según tamaño del edificio.
		FFachadaConfig Cfg;
		const float LadoGrande = FMath::Max(
			FVector2D(MundoXY[0] - MundoXY[1]).Size(),
			FVector2D(MundoXY[1] - MundoXY[2]).Size()) / 100.f;

		// Escalar espaciado de ventanas según tamaño.
		const float Factor = FMath::Clamp(LadoGrande / 15.f, 0.6f, 2.0f);
		Cfg.EspaciadoX = 300.f * Factor;
		// El ternario que había aquí comparaba Plantas > 0, que con levels >= 1 es
		// siempre cierto: daba 320 fijo. Ahora la altura de planta sale del dato.
		Cfg.EspaciadoY = 320.f;
		Cfg.AnchoVentana = FMath::Clamp(120.f * Factor, 80.f, 200.f);
		Cfg.AltoVentana = FMath::Clamp(160.f * Factor, 100.f, 220.f);
		Cfg.OffsetPrimerPiso = (E->Plantas > 1) ? 180.f : 120.f;

		// Y encima, lo que building_facades.json sí mide: altura de planta y
		// tamaño de ventana. Sus 24891 ventanas se leían para generar una malla
		// aparte que caía a 1 cm del centro del edificio (o sea dentro, sin que
		// se viera nada), mientras el muro de verdad se labraba con las cuentas
		// aproximadas de arriba. Ahora el dato entra en el muro que se ve.
		if (const FBuildingFacadeEntry* Fach = FachadaDe(E->Id))
		{
			// Altura de planta real, acotada a lo que puede ser una planta: el
			// fichero tiene 151 edificios entre 0,8 y 2,4 m, que es altura_total
			// partida por un recuento de plantas que no cuadra.
			Cfg.EspaciadoY = FMath::Clamp(Fach->AlturaPorNivel * 100.f, 240.f, 420.f);

			if (Fach->Ventanas.Num() > 0)
			{
				float SumaAncho = 0.f, SumaAlto = 0.f;
				for (const FWindowData& V : Fach->Ventanas) { SumaAncho += V.Ancho; SumaAlto += V.Alto; }
				Cfg.AnchoVentana = FMath::Clamp(SumaAncho / Fach->Ventanas.Num() * 100.f, 60.f, 260.f);
				Cfg.AltoVentana  = FMath::Clamp(SumaAlto  / Fach->Ventanas.Num() * 100.f, 80.f, 300.f);
			}

			// El espaciado horizontal se queda en la heurística del tamaño: el
			// número de ventanas del fichero no es un recuento del edificio sino
			// una muestra (8,5 por planta para 6,2 m de perímetro por hueco), y
			// usarlo como densidad dejaba las ventanas a 6,25 m unas de otras.
		}
		Cfg.ColorMuro = MuroReal(O, E->Id);
		Cfg.ColorPuerta = FColor(static_cast<uint8>(Cfg.ColorMuro.R * 0.5f), static_cast<uint8>(Cfg.ColorMuro.G * 0.4f), static_cast<uint8>(Cfg.ColorMuro.B * 0.3f), 255);
		Cfg.bPonerPuerta = (AlturaM < 25.0); // no poner puerta en edificios muy altos

		E->ConstruirConDetalle(Local, (float)(AlturaM * 100.0), Cfg, ColorTejado, Forma, Eje, EscalaTejado);
	}
	else
	{
		E->Construir(Local, (float)(AlturaM * 100.0),
			MuroReal(O, E->Id), ColorTejado, Forma, Eje, EscalaTejado);
	}

	// Sección 0 = muros+suelo (fachada); sección 1 = tejado (ortofoto real si existe).
	if (E->Malla)
	{
		static UMaterialInterface* MatMuro = nullptr;
		static UMaterialInterface* MatFachada = nullptr;
		static bool bBuscados = false;
		if (!bBuscados)
		{
			bBuscados = true;
			MatFachada = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Fachada.M_Fachada"));
			MatMuro = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Edificio.M_Edificio"));
			if (!MatMuro) MatMuro = MatFachada;
		}

		UMaterialInterface* MatUsar = MatFachada ? MatFachada : MatMuro;
		if (MatUsar) E->Malla->SetMaterial(0, MatUsar);

		static UMaterialInterface* MatTejado = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Tejado_Orto.M_Tejado_Orto"));
		if (MatTejado) E->Malla->SetMaterial(1, MatTejado);
		else if (MatMuro) E->Malla->SetMaterial(1, MatMuro);

		// Barrio-specific material override
		const FString Barrio = O->HasField(TEXT("barrio")) ? O->GetStringField(TEXT("barrio")) : FString();
		const FString MatType = O->HasField(TEXT("material_type")) ? O->GetStringField(TEXT("material_type")) : FString();

		if (!Barrio.IsEmpty() && !MatType.IsEmpty())
		{
			FString MatPath = FString::Printf(TEXT("/Game/Materiales/M_%s_%s"), *MatType, *Barrio);
			UMaterialInterface* MatBarrio = LoadObject<UMaterialInterface>(nullptr, *MatPath);
			if (!MatBarrio)
			{
				MatPath = FString::Printf(TEXT("/Game/Materiales/M_%s"), *MatType);
				MatBarrio = LoadObject<UMaterialInterface>(nullptr, *MatPath);
			}
			if (MatBarrio) E->Malla->SetMaterial(0, MatBarrio);
		}
	}
	++Construidos;
}

bool UCargadorEdificios::PasoPresupuesto(double PresupuestoMs)
{
	if (!bPreparado) PrepararCarga();
	const double t0 = FPlatformTime::Seconds();
	while (Idx < Items.Num())
	{
		ConstruirUno(Items[Idx++]->AsObject());
		if ((FPlatformTime::Seconds() - t0) * 1000.0 >= PresupuestoMs) break;
	}
	return Terminado();
}

int32 UCargadorEdificios::Cargar()
{
	if (bHecho) return 0;
	bHecho = true;
	PrepararCarga();
	int32 IterGuard = 0;
	const int32 MaxIter = 10000;
	while (!PasoPresupuesto(1000.0) && ++IterGuard < MaxIter) {}
	if (IterGuard >= MaxIter) UE_LOG(LogTemp, Warning, TEXT("[Edificios] Iteration guard reached (%d)"), MaxIter);
	UE_LOG(LogTemp, Log, TEXT("[Edificios] %d edificios construidos (%d con altura medida por LiDAR)"),
		Construidos, GSustituidas);
	return Construidos;
}
