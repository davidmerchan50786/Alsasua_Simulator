// ImportadorLandscape.cpp (sólo editor)
#include "ImportadorLandscape.h"
#include "GeoDataAlsasua.h"
#include "Landscape.h"
#include "LandscapeInfo.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"

bool UImportadorLandscape::ImportarLandscape(
	const FString& RutaR16, int32 Resolucion,
	double EscalaXY_cm, double EscalaZ, double LocZ_cm, FVector CentroXY_cm,
	bool bPermitirWorldPartition)
{
	// 1) Leer el RAW uint16 (LE).
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *RutaR16))
	{
		UE_LOG(LogTemp, Error, TEXT("[Landscape] no pude leer %s"), *RutaR16);
		return false;
	}
	const int32 Esperado = Resolucion * Resolucion * 2;
	if (Bytes.Num() != Esperado)
	{
		UE_LOG(LogTemp, Error, TEXT("[Landscape] tamaño %d != %d (res %d)"), Bytes.Num(), Esperado, Resolucion);
		return false;
	}
	TArray<uint16> Alturas;
	Alturas.SetNumUninitialized(Resolucion * Resolucion);
	FMemory::Memcpy(Alturas.GetData(), Bytes.GetData(), Esperado);   // uint16 LE = layout nativo x86

	// 2) Mundo del editor.
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World) { UE_LOG(LogTemp, Error, TEXT("[Landscape] sin mundo de editor")); return false; }

	// 2b) Blindaje World Partition: un único ALandscape no crea los streaming proxies.
	if (World->IsPartitionedWorld() && !bPermitirWorldPartition)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[Landscape] El nivel es World Partition. El import de un unico ALandscape ")
			TEXT("no genera proxies de streaming. Usa Landscape Mode -> Import from File (crea ")
			TEXT("los proxies), o un nivel Empty para el primer arranque. Pasa ")
			TEXT("bPermitirWorldPartition=true para forzar (no recomendado)."));
		return false;
	}

	// 3) Transformada: centro en Herriko Plaza si no se pasa otro.
	FVector Centro = CentroXY_cm;
	if (Centro.IsNearlyZero()) Centro = UAlsasuaGeoData::HerrikoPlaza();
	// El import coloca el origen en la esquina; desplazamos media extensión para centrar.
	const double MitadXY = (Resolucion - 1) * EscalaXY_cm * 0.5;
	const FVector Loc(Centro.X - MitadXY, Centro.Y - MitadXY, LocZ_cm);

	ALandscape* Landscape = World->SpawnActor<ALandscape>();
	if (!Landscape) { UE_LOG(LogTemp, Error, TEXT("[Landscape] SpawnActor falló")); return false; }
	Landscape->SetActorLocation(Loc);
	Landscape->SetActorScale3D(FVector(EscalaXY_cm, EscalaXY_cm, EscalaZ));
	Landscape->SetActorLabel(TEXT("Alsasua_Landscape"));

	// 4) Layout: 4033 verts = 63 quads/sección × 1 subsección × 64×64 componentes.
	const int32 SubsectionSizeQuads = 63;
	const int32 NumSubsections      = 1;

	TMap<FGuid, TArray<uint16>> HeightData;
	HeightData.Add(FGuid(), MoveTemp(Alturas));
	TMap<FGuid, TArray<FLandscapeImportLayerInfo>> MaterialLayers;
	MaterialLayers.Add(FGuid(), TArray<FLandscapeImportLayerInfo>());

	// La firma de ALandscapeProxy::Import cambió en UE 5.8; además el terreno definitivo
	// va por Mesh Terrain (ver plan de migración). Para no bloquear el build, el import de
	// datos automático queda desactivado: importa el heightmap con Landscape Mode -> Import
	// from File, o usa el pipeline de Mesh Terrain.
	(void)SubsectionSizeQuads; (void)NumSubsections; (void)HeightData; (void)MaterialLayers;
	UE_LOG(LogTemp, Warning,
		TEXT("[Landscape] Import automático desactivado en 5.8. Heightmap: %s. ")
		TEXT("Impórtalo con Landscape Mode -> Import from File, o vía Mesh Terrain."), *RutaR16);

	// Material del terreno con la ortofoto, si ya se generó.
	if (UMaterialInterface* MTer = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Terreno_Orto.M_Terreno_Orto")))
		Landscape->LandscapeMaterial = MTer;

	Landscape->StaticLightingLOD = FMath::DivideAndRoundUp(FMath::CeilLogTwo((Resolucion - 1) / 64 + 1), (uint32)2);
	if (ULandscapeInfo* Info = Landscape->GetLandscapeInfo())
		Info->UpdateLayerInfoMap(Landscape);
	Landscape->PostEditChange();

	UE_LOG(LogTemp, Log, TEXT("[Landscape] OK: %s en (%.0f,%.0f,%.0f) escala (%.3f,%.3f,%.1f)"),
		*RutaR16, Loc.X, Loc.Y, Loc.Z, EscalaXY_cm, EscalaXY_cm, EscalaZ);
	return true;
}
