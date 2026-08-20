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
	if (E.StartsWith(TEXT("Cocos")) || E.StartsWith(TEXT("Phoenix")))
		return FLinearColor(0.22f, 0.48f, 0.16f);  // palmera
	return FLinearColor(0.24f, 0.40f, 0.16f);  // genérico
}

void UCargadorArboles::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// Los coloca ADirectorArranque tras generar el terreno.
}

float UCargadorArboles::AlturaSuelo(const FVector2D& XY) const
{
	const UWorld* W = GetWorld();
	if (!W) return 0.f;
	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(AlturaArbol), true);
	if (W->LineTraceSingleByChannel(Hit, FVector(XY.X, XY.Y, UAlsasuaGeoData::TraceUp), FVector(XY.X, XY.Y, UAlsasuaGeoData::TraceDown), ECC_Visibility, Q))
		return Hit.Location.Z;
	return 0.f;
}

/**
 * trees_unity.json trae el nombre científico completo ("Quercus robur",
 * "Pinus sylvestris", "Fagus sylvatica") y las mallas están indexadas por
 * arquetipo ("QuercusRobur", "Pinus", "Fagus"), así que Find() con la cadena
 * cruda no acertaba nunca y las 2783 instancias salían cilíndricas.
 */
static FString ArquetipoDe(const FString& Especie)
{
	if (Especie.StartsWith(TEXT("Quercus")))  return TEXT("QuercusRobur");
	if (Especie.StartsWith(TEXT("Pinus")))    return TEXT("Pinus");
	if (Especie.StartsWith(TEXT("Fagus")))    return TEXT("Fagus");
	if (Especie.StartsWith(TEXT("Betula")))   return TEXT("Betula");
	if (Especie.StartsWith(TEXT("Populus")))  return TEXT("Populus");
	if (Especie.StartsWith(TEXT("Salix")))    return TEXT("Salix");
	if (Especie.StartsWith(TEXT("Prunus")))   return TEXT("Prunus");
	if (Especie.StartsWith(TEXT("Platanus"))) return TEXT("Platanus");
	if (Especie.StartsWith(TEXT("Acer")))     return TEXT("Acer");
	// Palmeras DZ_Assets.
	if (Especie.StartsWith(TEXT("Cocos")))    return TEXT("CocosNucifera");
	if (Especie.StartsWith(TEXT("Phoenix")))  return TEXT("PhoenixCanariensis");
	// El fresno no tiene malla propia; comparte porte con el tilo.
	if (Especie.StartsWith(TEXT("Fraxinus"))) return TEXT("Tilia");
	if (Especie.StartsWith(TEXT("Tilia")))    return TEXT("Tilia");
	return TEXT("Tilia");
}

UHierarchicalInstancedStaticMeshComponent* UCargadorArboles::ComponenteDe(const FString& Especie)
{
	if (UHierarchicalInstancedStaticMeshComponent** F = PorEspecie.Find(Especie)) return *F;
	UHierarchicalInstancedStaticMeshComponent* C =
		NewObject<UHierarchicalInstancedStaticMeshComponent>(Host);

	// Usar mesh de especie si existe, fallback al cilindro.
	if (UStaticMesh** SpMesh = MallasPorEspecie.Find(ArquetipoDe(Especie)))
	{
		C->SetStaticMesh(*SpMesh);
	}
	else
	{
		C->SetStaticMesh(MallaDefecto);
	}
	C->SetCullDistances(0, 80000);
	C->SetCastShadow(true);
	C->RegisterComponent();
	C->AttachToComponent(Host->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);

	// Material: los meshes DZ_Assets traen materiales propios con viento.
	// Solo aplicar M_Arbol genérico a los meshes generados por AlsasuaAssetGenerator.
	const bool bMeshGenerado = MallasPorEspecie.Contains(ArquetipoDe(Especie));
	const bool bMeshDZ = EspeciesDZ.Contains(ArquetipoDe(Especie));
	if (bMeshGenerado && !bMeshDZ)
	{
		if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Arbol.M_Arbol")))
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, C);
			MID->SetVectorParameterValue(TEXT("Color"), ColorEspecie(Especie));
			C->SetMaterial(0, MID);
		}
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

	// Cargar meshes de especies generados por AlsasuaAssetGenerator.
	const FString RutaMeshes = TEXT("/Game/Meshes/Arboles/");
	const TArray<FString> Especies = {TEXT("Tilia"), TEXT("Platanus"), TEXT("QuercusRobur"),
		TEXT("Pinus"), TEXT("Fagus"), TEXT("Betula"), TEXT("Populus"),
		TEXT("Salix"), TEXT("Prunus"), TEXT("Acer")};
	for (const FString& Sp : Especies)
	{
		const FString Ruta = RutaMeshes + TEXT("SM_") + Sp + TEXT(".SM_") + Sp;
		if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Ruta))
		{
			MallasPorEspecie.Add(Sp, Mesh);
		}
	}

	// Fallback: meshes DZ_Assets de alta calidad (paquete externo).
	const TPair<FString, FString> DZFallbacks[] = {
		{TEXT("Pinus"),    TEXT("/Game/DZ_Assets/DZ_Trees/Meshes/Pine/SM_Pine_1.Pine_1")},
		{TEXT("QuercusRobur"), TEXT("/Game/DZ_Assets/DZ_Trees/Meshes/Cork_Oak/SM_Cork_Oak_1.Cork_Oak_1")},
		{TEXT("Populus"),   TEXT("/Game/DZ_Assets/DZ_Trees/Meshes/Aspen/SM_Columnar_Aspen_1.Aspen_1")},
		{TEXT("CocosNucifera"), TEXT("/Game/DZ_Assets/DZ_Trees/Meshes/Coconut_Tree/SM_Coconut_Tree_1.Coconut_Tree_1")},
		{TEXT("PhoenixCanariensis"), TEXT("/Game/DZ_Assets/DZ_Trees/Meshes/Windmill_Palm/SM_Windmill_Palm_1.Windmill_Palm_1")},
	};
	for (const auto& Pair : DZFallbacks)
	{
		if (!MallasPorEspecie.Contains(Pair.Key))
			if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, *Pair.Value))
			{
				MallasPorEspecie.Add(Pair.Key, M);
				EspeciesDZ.Add(Pair.Key);
			}
	}

	Host = W->SpawnActor<AActor>();
	if (Host)
	{
#if WITH_EDITOR
		Host->SetActorLabel(TEXT("Arboleda_Alsasua"));
#endif
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
	// El 0.4 en XY existía para adelgazar el cilindro de respaldo y que
	// pareciera un tronco. Con malla real de especie hay que escalar uniforme
	// o el árbol sale aplastado.
	const bool bMallaReal = MallasPorEspecie.Contains(ArquetipoDe(Especie));
	T.SetScale3D(bMallaReal ? FVector(Escala) : FVector(Escala * 0.4f, Escala * 0.4f, Escala));

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
	int32 IterGuard = 0;
	const int32 MaxIter = 10000;
	while (!PasoPresupuesto(1000.0) && ++IterGuard < MaxIter) {}
	if (IterGuard >= MaxIter) UE_LOG(LogTemp, Warning, TEXT("[Arboles] Iteration guard reached (%d)"), MaxIter);
	UE_LOG(LogTemp, Log, TEXT("[Arboles] %d árboles en %d especies"), Sembrados, PorEspecie.Num());
	return Sembrados;
}
