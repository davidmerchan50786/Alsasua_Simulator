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
	return LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Edificio.M_Edificio"));
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

void UCargadorEdificios::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// Si hay director de arranque, él decide el ritmo; no auto-cargamos.
	if (bAutoCargar && !ArranqueMundo::HayDirector) Cargar();
}

float UCargadorEdificios::AlturaSuelo(const FVector2D& XY) const
{
	const UWorld* W = GetWorld();
	if (!W) return 0.f;
	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(AlturaSuelo), true);
	if (W->LineTraceSingleByChannel(Hit, FVector(XY.X, XY.Y, 500000.f), FVector(XY.X, XY.Y, -500000.f), ECC_Visibility, Q))
		return Hit.Location.Z;
	return 0.f;
}

void UCargadorEdificios::PrepararCarga()
{
	if (bPreparado) return;
	bPreparado = true;

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
		const double ux = Po->GetNumberField(TEXT("x")) + UAlsasuaGeoData::OX;
		const double uz = Po->GetNumberField(TEXT("z")) + UAlsasuaGeoData::OZ;
		const FVector M = UAlsasuaGeoData::UnityaUnreal(FVector(ux, 0.0, uz));
		MundoXY.Add(FVector2D(M.X, M.Y));
		Centro += FVector2D(M.X, M.Y);
	}
	if (MundoXY.Num() < 3) return;
	Centro /= MundoXY.Num();

	const double AlturaM = O->HasField(TEXT("height")) ? O->GetNumberField(TEXT("height")) : 6.0;
	const float  Suelo   = AlturaSuelo(Centro);

	TArray<FVector2D> Local; Local.Reserve(MundoXY.Num());
	for (const FVector2D& V : MundoXY) Local.Add(V - Centro);

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AEdificioGenerado* E = GetWorld()->SpawnActor<AEdificioGenerado>(
		AEdificioGenerado::StaticClass(), FVector(Centro.X, Centro.Y, Suelo), FRotator::ZeroRotator, SP);
	if (!E) return;

	E->Id            = (int32)O->GetIntegerField(TEXT("id"));
	E->NombreEdificio = O->HasField(TEXT("name")) ? O->GetStringField(TEXT("name")) : FString();
	E->Plantas       = O->HasField(TEXT("levels")) ? (int32)O->GetNumberField(TEXT("levels")) : 1;

	// Eje real del caballete (Unity dx,dz) -> dirección en el plano XY de Unreal (X=Z, Y=X).
	FVector2D Eje(1, 0);
	if (O->HasField(TEXT("lidar_eje_x")) && O->HasField(TEXT("lidar_eje_z")))
		Eje = FVector2D(O->GetNumberField(TEXT("lidar_eje_z")), O->GetNumberField(TEXT("lidar_eje_x")));

	float EscalaTejado = 1.f;
	const EFormaTejado Forma = FormaReal(O, EscalaTejado);

	E->Construir(Local, (float)(AlturaM * 100.0),
		MuroReal(O, E->Id), TejadoReal(O, E->Id), Forma, Eje, EscalaTejado);

	// Sección 0 = muros+suelo (fachada); sección 1 = tejado (ortofoto real si existe).
	if (E->Malla)
	{
		static UMaterialInterface* MatMuro = CargarMaterialEdificio();
		if (MatMuro) E->Malla->SetMaterial(0, MatMuro);
		static UMaterialInterface* MatTejado = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Tejado_Orto.M_Tejado_Orto"));
		if (MatTejado) E->Malla->SetMaterial(1, MatTejado);
		else if (MatMuro) E->Malla->SetMaterial(1, MatMuro);   // respaldo: color de tejado por vértice
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
	while (!PasoPresupuesto(1000.0)) {}   // sin límite práctico
	UE_LOG(LogTemp, Log, TEXT("[Edificios] %d edificios construidos"), Construidos);
	return Construidos;
}
