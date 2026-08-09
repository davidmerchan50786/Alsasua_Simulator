// CargadorCalles.cpp
#include "CargadorCalles.h"
#include "CalleGenerada.h"
#include "ArranqueMundo.h"
#include "GeoDataAlsasua.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/PlatformTime.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "CargarMaterialComun.h"

// Materiales de suelo creados por las utilidades de editor UCreadorMaterialCalles.
// AAA de librería si sus dependencias existen; si no, materiales propios.
static UMaterialInterface* CargarMaterialSueloCalles()
{
	return CargarMaterialConFallback(
		TEXT("/Game/Road/Material/MI/M_Asphalt_Master_Inst.M_Asphalt_Master_Inst"),
		TEXT("/Game/Materiales/M_Terreno_Calles.M_Terreno_Calles"),
		TEXT("/Game/Materiales/M_Edificio.M_Edificio"));
}
static UMaterialInterface* CargarMaterialSueloSendero()
{
	return CargarMaterialConFallback(
		TEXT("/Game/Road/Material/MI/M_Sidewalk_Master_Inst.M_Sidewalk_Master_Inst"),
		TEXT("/Game/Materiales/M_Terreno_Acera.M_Terreno_Acera"),
		TEXT("/Game/Materiales/M_Edificio.M_Edificio"));
}

void UCargadorCalles::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// Las construye ADirectorArranque tras generar el terreno.
}

void UCargadorCalles::PrepararCarga()
{
	if (bPreparado) return;
	bPreparado = true;
	const FString Ruta = FPaths::Combine(FPaths::ProjectContentDir(), RutaRelativa);
	FString Texto;
	if (!FFileHelper::LoadFileToString(Texto, *Ruta))
	{ UE_LOG(LogTemp, Error, TEXT("[Calles] no pude leer %s"), *Ruta); return; }
	const TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Texto);
	if (!FJsonSerializer::Deserialize(R, Items))
	{ UE_LOG(LogTemp, Error, TEXT("[Calles] JSON inválido")); Items.Empty(); }
}

void UCargadorCalles::ConstruirUna(const TSharedPtr<FJsonObject>& O)
{
	if (!O.IsValid()) return;
	const TArray<TSharedPtr<FJsonValue>>* Ps = nullptr;
	if (!O->TryGetArrayField(TEXT("points"), Ps) || !Ps || Ps->Num() < 2) return;

	TArray<FVector2D> XY; XY.Reserve(Ps->Num());
	FVector2D Centro(0, 0);
	for (const TSharedPtr<FJsonValue>& Pv : *Ps)
	{
		const TSharedPtr<FJsonObject> Po = Pv->AsObject();
		if (!Po.IsValid()) continue;
		// roads_unity.json es RELATIVO a la plaza: sumar OX/OZ.
		const FVector M = UAlsasuaGeoData::RelLocalToUE5(FVector(Po->GetNumberField(TEXT("x")), 0.0, Po->GetNumberField(TEXT("z"))));
		XY.Add(FVector2D(M.X, M.Y));
		Centro += FVector2D(M.X, M.Y);
	}
	if (XY.Num() < 2) return;
	Centro /= XY.Num();

	const double AnchoM = O->HasField(TEXT("width")) ? O->GetNumberField(TEXT("width")) : 4.0;

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACalleGenerada* C = GetWorld()->SpawnActor<ACalleGenerada>(
		ACalleGenerada::StaticClass(), FVector(Centro.X, Centro.Y, 0.f), FRotator::ZeroRotator, SP);
	if (!C) return;
	C->Id   = (int32)O->GetIntegerField(TEXT("id"));
	C->Tipo = O->HasField(TEXT("type")) ? O->GetStringField(TEXT("type")) : FString();
	// Color por tipo: peatonal/sendero en tono claro de adoquín; calzada en asfalto.
	if (C->Tipo.Contains(TEXT("pedestrian")) || C->Tipo.Contains(TEXT("path")) || C->Tipo.Contains(TEXT("foot")))
		C->ColorBase = FColor(96, 90, 82, 255);
	else
		C->ColorBase = FColor(42, 42, 46, 255);
	C->Construir(XY, (float)(AnchoM * 100.0));
	if (C->Malla)
	{
		static UMaterialInterface* MatCalle = CargarMaterialSueloCalles();
		static UMaterialInterface* MatSendero = CargarMaterialSueloSendero();
		const bool bPeatonal = C->Tipo.Contains(TEXT("pedestrian")) || C->Tipo.Contains(TEXT("path")) || C->Tipo.Contains(TEXT("foot"));
		if (UMaterialInterface* Mat = bPeatonal ? MatSendero : MatCalle) C->Malla->SetMaterial(0, Mat);
	}
	FEjeVial Eje; Eje.Puntos = XY; Eje.AnchoCm = (float)(AnchoM * 100.0);
	EjesViarios.Add(MoveTemp(Eje));   // eje + ancho disponible para el tráfico
	++Construidas;
}

bool UCargadorCalles::PasoPresupuesto(double PresupuestoMs)
{
	if (!bPreparado) PrepararCarga();
	const double t0 = FPlatformTime::Seconds();
	while (Idx < Items.Num())
	{
		ConstruirUna(Items[Idx++]->AsObject());
		if ((FPlatformTime::Seconds() - t0) * 1000.0 >= PresupuestoMs) break;
	}
	return Terminado();
}

int32 UCargadorCalles::Cargar()
{
	if (bHecho) return 0;
	bHecho = true;
	PrepararCarga();
	int32 IterGuard = 0;
	const int32 MaxIter = 10000;
	while (!PasoPresupuesto(1000.0) && ++IterGuard < MaxIter) {}
	if (IterGuard >= MaxIter) UE_LOG(LogTemp, Warning, TEXT("[Calles] Iteration guard reached (%d)"), MaxIter);
	UE_LOG(LogTemp, Log, TEXT("[Calles] %d calles construidas"), Construidas);
	return Construidas;
}
