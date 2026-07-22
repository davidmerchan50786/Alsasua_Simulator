// CargadorArboles.cpp
#include "CargadorArboles.h"
#include "ArranqueMundo.h"
#include "GeoDataAlsasua.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "CollisionQueryParams.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

// Tono de copa por especie (follaje navarro).
static FLinearColor ColorEspecie(const FString& E)
{
	if (E.StartsWith(TEXT("Pinus")))   return FLinearColor(0.10f, 0.26f, 0.15f);  // pino, verde oscuro
	if (E.StartsWith(TEXT("Fagus")))   return FLinearColor(0.22f, 0.40f, 0.13f);  // haya
	if (E.StartsWith(TEXT("Quercus"))) return FLinearColor(0.18f, 0.36f, 0.12f);  // roble/encina
	if (E.StartsWith(TEXT("Betula")))  return FLinearColor(0.34f, 0.52f, 0.20f);  // abedul, claro
	if (E.StartsWith(TEXT("Populus")) || E.StartsWith(TEXT("Salix"))) return FLinearColor(0.38f, 0.52f, 0.18f); // chopo/sauce
	if (E.StartsWith(TEXT("Prunus")))  return FLinearColor(0.30f, 0.42f, 0.22f);
	return FLinearColor(0.24f, 0.40f, 0.16f);  // genérico
}

void UCargadorArboles::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (bAutoCargar && !ArranqueMundo::HayDirector) Cargar();
}

float UCargadorArboles::AlturaSuelo(const FVector2D& XY) const
{
	const UWorld* W = GetWorld();
	if (!W) return 0.f;
	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(AlturaArbol), true);
	if (W->LineTraceSingleByChannel(Hit, FVector(XY.X, XY.Y, 500000.f), FVector(XY.X, XY.Y, -500000.f), ECC_Visibility, Q))
		return Hit.Location.Z;
	return 0.f;
}

UHierarchicalInstancedStaticMeshComponent* UCargadorArboles::ComponenteDe(const FString& Especie)
{
	if (UHierarchicalInstancedStaticMeshComponent** F = PorEspecie.Find(Especie)) return *F;
	UHierarchicalInstancedStaticMeshComponent* C =
		NewObject<UHierarchicalInstancedStaticMeshComponent>(Host);
	C->SetStaticMesh(MallaDefecto);
	C->SetCullDistances(0, 30000);
	C->SetCastShadow(true);
	C->RegisterComponent();
	C->AttachToComponent(Host->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);

	// Material con color de copa por especie (instancia dinámica del material base).
	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Arbol.M_Arbol")))
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, C);
		MID->SetVectorParameterValue(TEXT("Color"), ColorEspecie(Especie));
		C->SetMaterial(0, MID);
	}

	PorEspecie.Add(Especie, C);
	return C;
}

void UCargadorArboles::PrepararCarga()
{
	if (bPreparado) return;
	bPreparado = true;

	UWorld* W = GetWorld();
	if (!W) return;

	MallaDefecto = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Host = W->SpawnActor<AActor>();
	if (Host)
	{
		Host->SetActorLabel(TEXT("Arboleda_Alsasua"));
		USceneComponent* Root = NewObject<USceneComponent>(Host, TEXT("Root"));
		Root->RegisterComponent();
		Host->SetRootComponent(Root);
	}

	const FString Ruta = FPaths::Combine(FPaths::ProjectContentDir(), RutaRelativa);
	FString Texto;
	if (!FFileHelper::LoadFileToString(Texto, *Ruta))
	{ UE_LOG(LogTemp, Error, TEXT("[Arboles] no pude leer %s"), *Ruta); return; }
	const TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Texto);
	if (!FJsonSerializer::Deserialize(R, Items))
	{ UE_LOG(LogTemp, Error, TEXT("[Arboles] JSON inválido")); Items.Empty(); }
}

void UCargadorArboles::SembrarUno(const TSharedPtr<FJsonObject>& O)
{
	if (!O.IsValid() || !Host) return;
	// trees_unity.json está en mundo Unity ABSOLUTO (no se suma OX/OZ).
	const double ux = O->GetNumberField(TEXT("x"));
	const double uz = O->GetNumberField(TEXT("z"));
	const FVector M = UAlsasuaGeoData::UnityaUnreal(FVector(ux, 0.0, uz));
	const FVector2D XY(M.X, M.Y);

	const double AlturaM = O->HasField(TEXT("altura")) ? O->GetNumberField(TEXT("altura")) : AlturaReferenciaMalla;
	const FString Especie = O->HasField(TEXT("especie")) ? O->GetStringField(TEXT("especie")) : TEXT("generico");

	const float Suelo  = AlturaSuelo(XY);
	const float Escala = FMath::Max(0.2f, (float)AlturaM / FMath::Max(1.f, AlturaReferenciaMalla));
	const float Yaw    = FMath::FRandRange(0.f, 360.f);

	FTransform T;
	T.SetLocation(FVector(XY.X, XY.Y, Suelo));
	T.SetRotation(FQuat(FRotator(0, Yaw, 0)));
	T.SetScale3D(FVector(Escala * 0.4f, Escala * 0.4f, Escala));

	ComponenteDe(Especie)->AddInstance(T, /*bWorldSpace=*/true);
	++Sembrados;
}

bool UCargadorArboles::PasoPresupuesto(double PresupuestoMs)
{
	if (!bPreparado) PrepararCarga();
	const double t0 = FPlatformTime::Seconds();
	while (Idx < Items.Num())
	{
		SembrarUno(Items[Idx++]->AsObject());
		if ((FPlatformTime::Seconds() - t0) * 1000.0 >= PresupuestoMs) break;
	}
	return Terminado();
}

int32 UCargadorArboles::Cargar()
{
	if (bHecho) return 0;
	bHecho = true;
	PrepararCarga();
	while (!PasoPresupuesto(1000.0)) {}
	UE_LOG(LogTemp, Log, TEXT("[Arboles] %d árboles en %d especies"), Sembrados, PorEspecie.Num());
	return Sembrados;
}
