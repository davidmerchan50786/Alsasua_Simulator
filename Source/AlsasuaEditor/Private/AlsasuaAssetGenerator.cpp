// AlsasuaAssetGenerator.cpp (sólo editor)
// Pipeline maestro de generación de assets visuales para Alsasua.
#include "AlsasuaAssetGenerator.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/TextureFactory.h"
#include "Factories/MaterialFactoryNew.h"
#include "Factories/StaticMeshFactory.h"
#include "EditorAssetLibrary.h"
#include "AssetImportTask.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "ProceduralMeshComponent.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "Modules/ModuleManager.h"

// Incluir los creadores de materiales
#include "CreadorMaterialEdificio.h"
#include "CreadorMaterialFachada.h"
#include "CreadorMaterialTejadoOrto.h"
#include "CreadorMaterialTerrenoOrto.h"
#include "CreadorMaterialArbol.h"
#include "CreadorMaterialAgua.h"
#include "CreadorMaterialCalles.h"
#include "CreadorMaterialMuroPiedra.h"
#include "CreadorMaterialTejas.h"
#include "CreadorMaterialMobiliario.h"

// Generadores de geo/meshes
#include "AlsasuaRiverMeshGenerator.h"
#include "AlsasuaBridgeGenerator.h"
#include "AlsasuaFoliageLoader.h"

// ═══════════════════════════════════════════════════════════════════════════
//  Utilidades
// ═══════════════════════════════════════════════════════════════════════════
void UAlsasuaAssetGenerator::CrearCarpeta(const FString& Ruta)
{
	if (!UEditorAssetLibrary::DoesAssetExist(Ruta))
	{
		UEditorAssetLibrary::MakeDirectory(Ruta);
	}
}

// ═══════════════════════════════════════════════════════════════════════════
//  Pipeline maestro
// ═══════════════════════════════════════════════════════════════════════════
bool UAlsasuaAssetGenerator::GenerarTodosLosAssets()
{
	UE_LOG(LogTemp, Log, TEXT("[AlsasuaAssetGenerator] ═══ INICIO ═══ Generando assets visuales..."));

	// Crear carpetas base
	CrearCarpeta(TEXT("/Game/Materiales"));
	CrearCarpeta(TEXT("/Game/Textures"));
	CrearCarpeta(TEXT("/Game/Meshes"));
	CrearCarpeta(TEXT("/Game/Meshes/Arboles"));
	CrearCarpeta(TEXT("/Game/Meshes/Mobiliario"));
	CrearCarpeta(TEXT("/Game/Meshes/Landmarks"));
	CrearCarpeta(TEXT("/Game/Foliage"));

	// Ejecutar cada paso en orden
	int32 PasosOK = 0;
	int32 TotalPasos = 9;

	if (ImportarOrtofoto())    { ++PasosOK; UE_LOG(LogTemp, Log, TEXT("[AssetGen] Paso 1/9 OK: Ortofoto importada")); }
	else { UE_LOG(LogTemp, Warning, TEXT("[AssetGen] Paso 1/9 SKIP: Ortofoto no encontrada o ya existe")); }

	if (CrearTodosLosMateriales()) { ++PasosOK; UE_LOG(LogTemp, Log, TEXT("[AssetGen] Paso 2/9 OK: Materiales base creados")); }
	else { UE_LOG(LogTemp, Warning, TEXT("[AssetGen] Paso 2/9 FAIL: Error creando materiales base")); }

	if (CrearMaterialesPBR()) { ++PasosOK; UE_LOG(LogTemp, Log, TEXT("[AssetGen] Paso 3/9 OK: Materiales PBR creados")); }
	else { UE_LOG(LogTemp, Warning, TEXT("[AssetGen] Paso 3/9 FAIL: Error creando materiales PBR")); }

	if (GenerarMeshesArboles()) { ++PasosOK; UE_LOG(LogTemp, Log, TEXT("[AssetGen] Paso 4/9 OK: Árboles generados")); }
	else { UE_LOG(LogTemp, Warning, TEXT("[AssetGen] Paso 4/9 FAIL: Error generando árboles")); }

	if (GenerarMobiliarioUrbano()) { ++PasosOK; UE_LOG(LogTemp, Log, TEXT("[AssetGen] Paso 5/9 OK: Mobiliario generado")); }
	else { UE_LOG(LogTemp, Warning, TEXT("[AssetGen] Paso 5/9 FAIL: Error generando mobiliario")); }

	if (GenerarLandmarks()) { ++PasosOK; UE_LOG(LogTemp, Log, TEXT("[AssetGen] Paso 6/9 OK: Landmarks generados")); }
	else { UE_LOG(LogTemp, Warning, TEXT("[AssetGen] Paso 6/9 FAIL: Error generando landmarks")); }

	if (GenerarRios()) { ++PasosOK; UE_LOG(LogTemp, Log, TEXT("[AssetGen] Paso 7/9 OK: Ríos generados")); }
	else { UE_LOG(LogTemp, Warning, TEXT("[AssetGen] Paso 7/9 FAIL: Error generando ríos")); }

	if (GenerarPuentes()) { ++PasosOK; UE_LOG(LogTemp, Log, TEXT("[AssetGen] Paso 8/9 OK: Puentes generados")); }
	else { UE_LOG(LogTemp, Warning, TEXT("[AssetGen] Paso 8/9 FAIL: Error generando puentes")); }

	if (ScanFoliage()) { ++PasosOK; UE_LOG(LogTemp, Log, TEXT("[AssetGen] Paso 9/9 OK: Foliage registrado")); }
	else { UE_LOG(LogTemp, Warning, TEXT("[AssetGen] Paso 9/9 WARN: Foliage no encontrado (importar desde Fab)")); }

	UE_LOG(LogTemp, Log, TEXT("[AlsasuaAssetGenerator] ═══ FIN ═══ %d/%d pasos completados."), PasosOK, TotalPasos);
	return PasosOK == TotalPasos;
}

// ═══════════════════════════════════════════════════════════════════════════
//  Paso 1: Importar ortofoto
// ═══════════════════════════════════════════════════════════════════════════
bool UAlsasuaAssetGenerator::ImportarOrtofoto()
{
	const FString RutaOrigen = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Datos/ortofoto_unity.png"));
	const FString RutaDestino = TEXT("/Game/Textures/T_Ortofoto");

	if (UEditorAssetLibrary::DoesAssetExist(RutaDestino))
	{
		UE_LOG(LogTemp, Log, TEXT("[Ortofoto] T_Ortofoto ya existe, saltando importación."));
		return true;
	}

	if (!FPaths::FileExists(RutaOrigen))
	{
		UE_LOG(LogTemp, Error, TEXT("[Ortofoto] No se encontró %s"), *RutaOrigen);
		return false;
	}

	UTextureFactory* Factory = NewObject<UTextureFactory>();
	Factory->SuppressImportOverwriteWarning = true;

	UAssetImportTask* Task = NewObject<UAssetImportTask>();
	Task->Filename = RutaOrigen;
	Task->DestinationPath = TEXT("/Game/Textures");
	Task->DestinationName = TEXT("T_Ortofoto");
	Task->bReplaceExisting = false;
	Task->bAutomated = true;
	Task->Options = nullptr;
	Task->Factory = Factory;

	TArray<UAssetImportTask*> Tasks;
	Tasks.Add(Task);

	IAssetTools& AT = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	AT.ImportAssetTasks(Tasks);

	if (Task->ResultObject)
	{
		UEditorAssetLibrary::SaveAsset(RutaDestino, false);
		UE_LOG(LogTemp, Log, TEXT("[Ortofoto] T_Ortofoto importada desde %s"), *RutaOrigen);
		return true;
	}

	UE_LOG(LogTemp, Error, TEXT("[Ortofoto] Fallo al importar %s"), *RutaOrigen);
	return false;
}

// ═══════════════════════════════════════════════════════════════════════════
//  Paso 2: Crear todos los materiales
// ═══════════════════════════════════════════════════════════════════════════
bool UAlsasuaAssetGenerator::CrearTodosLosMateriales()
{
	bool bOK = true;

	// Orden importante: MPC_Clima primero, luego Fachada lo necesita.
	if (!UCreadorMaterialEdificio::CrearMaterialEdificio())
	{
		UE_LOG(LogTemp, Error, TEXT("[Materiales] Fallo creando M_Edificio + MPC_Clima"));
		bOK = false;
	}

	if (!UCreadorMaterialFachada::CrearMaterialFachada())
	{
		UE_LOG(LogTemp, Error, TEXT("[Materiales] Fallo creando M_Fachada"));
		bOK = false;
	}

	if (!UCreadorMaterialArbol::CrearMaterialArbol())
	{
		UE_LOG(LogTemp, Error, TEXT("[Materiales] Fallo creando M_Arbol"));
		bOK = false;
	}

	if (!UCreadorMaterialAgua::CrearMaterialAgua())
	{
		UE_LOG(LogTemp, Error, TEXT("[Materiales] Fallo creando M_AguaRio"));
		bOK = false;
	}

	// Estos requieren T_Ortofoto
	if (UEditorAssetLibrary::DoesAssetExist(TEXT("/Game/Textures/T_Ortofoto")))
	{
		if (!UCreadorMaterialTejadoOrto::CrearMaterialTejadoOrto())
		{
			UE_LOG(LogTemp, Error, TEXT("[Materiales] Fallo creando M_Tejado_Orto"));
			bOK = false;
		}
		if (!UCreadorMaterialTerrenoOrto::CrearMaterialTerrenoOrto())
		{
			UE_LOG(LogTemp, Error, TEXT("[Materiales] Fallo creando M_Terreno_Orto"));
			bOK = false;
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Materiales] T_Ortofoto no existe, saltando M_Tejado_Orto y M_Terreno_Orto"));
	}

	return bOK;
}

// ═══════════════════════════════════════════════════════════════════════════
//  Paso 3: Generar meshes de árboles procedurales
// ═══════════════════════════════════════════════════════════════════════════
// Genera un tronco+copa simplificado para cada especie arbórea de Alsasua.
static void GenerarArbolConico(UStaticMesh*& OutMesh, const FString& Nombre,
	float AlturaTotal, float RadioTronco, float RadioCopa, int32 Segmentos)
{
	OutMesh = nullptr;
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	const int32 AnillosTronco = 3;
	const int32 AnillosCopa = 5;

	// ── Tronco (cilindro) ──
	float Z = 0.f;
	for (int32 A = 0; A <= AnillosTronco; ++A)
	{
		float ZZ = Z + (AlturaTotal * 0.4f) * A / AnillosTronco;
		for (int32 S = 0; S <= Segmentos; ++S)
		{
			float Ang = 2.f * PI * S / Segmentos;
			Vertices.Add(FVector(FMath::Cos(Ang) * RadioTronco, FMath::Sin(Ang) * RadioTronco, ZZ));
			Normals.Add(FVector(FMath::Cos(Ang), FMath::Sin(Ang), 0.f).GetSafeNormal());
			UVs.Add(FVector2D((float)S / Segmentos, (float)A / AnillosTronco));
			VertexColors.Add(FColor(101, 67, 33)); // Marrón tronco
		}
	}

	int32 Off = 0;
	for (int32 A = 0; A < AnillosTronco; ++A)
	{
		for (int32 S = 0; S < Segmentos; ++S)
		{
			int32 I0 = Off + A * (Segmentos + 1) + S;
			int32 I1 = I0 + 1;
			int32 I2 = I0 + (Segmentos + 1);
			int32 I3 = I2 + 1;
			Triangles.Append({I0, I2, I1, I1, I2, I3});
		}
	}
	Off += (AnillosTronco + 1) * (Segmentos + 1);

	// ── Copa (cono) ──
	float ZBase = AlturaTotal * 0.35f;
	float ZTop = AlturaTotal;
	for (int32 A = 0; A <= AnillosCopa; ++A)
	{
		float T = (float)A / AnillosCopa;
		float ZZ = ZBase + (ZTop - ZBase) * T;
		float R = RadioCopa * (1.f - T);
		for (int32 S = 0; S <= Segmentos; ++S)
		{
			float Ang = 2.f * PI * S / Segmentos;
			Vertices.Add(FVector(FMath::Cos(Ang) * R, FMath::Sin(Ang) * R, ZZ));
			Normals.Add(FVector(FMath::Cos(Ang), FMath::Sin(Ang), 0.3f).GetSafeNormal());
			UVs.Add(FVector2D((float)S / Segmentos, T));
			VertexColors.Add(FColor(34, 120, 34)); // Verde copa
		}
	}

	for (int32 A = 0; A < AnillosCopa; ++A)
	{
		for (int32 S = 0; S < Segmentos; ++S)
		{
			int32 I0 = Off + A * (Segmentos + 1) + S;
			int32 I1 = I0 + 1;
			int32 I2 = I0 + (Segmentos + 1);
			int32 I3 = I2 + 1;
			Triangles.Append({I0, I2, I1, I1, I2, I3});
		}
	}

	// Construir mesh estática.
	UStaticMesh* Mesh = NewObject<UStaticMesh>();
	Mesh->GetStaticMaterials().Add(FStaticMaterial());

	FMeshDescription MD;
	FStaticMeshAttributes Attributes(MD);
	Attributes.CreateVertexAttributes();
	Attributes.CreatePolygonAttributes();
	Attributes.CreatePolygonGroupAttributes();
	Attributes.CreateEdgeAttributes();

	MeshDescription::VertexInstanceID VID = 0;
	TArray<MeshDescription::VertexInstanceID> VIDs;
	TArray<MeshDescription::VertexID> VIDs2;
	for (int32 i = 0; i < Vertices.Num(); ++i)
	{
		MeshDescription::VertexID VID2 = MD.AppendVertex(Vertices[i]);
		VIDs2.Add(VID2);
		MeshDescription::VertexInstanceID InstID = MD.AppendVertexInstance(VID2, Normals[i], UVs[i], VertexColors[i]);
		VIDs.Add(InstID);
	}

	MeshDescription::PolygonGroupID PGID = MD.AppendPolygonGroup();

	for (int32 i = 0; i < Triangles.Num(); i += 3)
	{
		TArray<MeshDescription::VertexInstanceID> PolyVIDs = {VIDs[Triangles[i]], VIDs[Triangles[i+1]], VIDs[Triangles[i+2]]};
		MD.AppendPolygon(PGID, PolyVIDs);
	}

	TArray<const FMeshDescription*> BulkData;
	BulkData.Add(&MD);
	Mesh->BuildFromMeshDescriptions(BulkData);

	FString RutaMesh = FString::Printf(TEXT("/Game/Meshes/Arboles/SM_%s"), *Nombre);
	if (UEditorAssetLibrary::DoesAssetExist(RutaMesh))
		UEditorAssetLibrary::DeleteAsset(RutaMesh);

	IAssetTools& AT = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	AT.CreateAsset(Nombre, TEXT("/Game/Meshes/Arboles"), UStaticMesh::StaticClass(), nullptr);

	// Copiar datos al asset recién creado.
	UStaticMesh* Created = LoadObject<UStaticMesh>(nullptr, *(RutaMesh + TEXT(".")) + Nombre);
	if (Created)
	{
		Created->BuildFromMeshDescriptions(BulkData);
		UEditorAssetLibrary::SaveAsset(RutaMesh, false);
		OutMesh = Created;
	}
}

bool UAlsasuaAssetGenerator::GenerarMeshesArboles()
{
	// Especies presentes en trees_unity.json de Alsasua.
	struct FSpecieArbol { FString Nombre; float Altura; float RadioTronco; float RadioCopa; FColor Hojas; };
	TArray<FSpecieArbol> Especies = {
		{TEXT("Tilia"),           8.5f, 0.15f, 2.8f, FColor(40, 130, 40)},
		{TEXT("Platanus"),       10.0f, 0.20f, 3.5f, FColor(50, 140, 35)},
		{TEXT("QuercusRobur"),    9.0f, 0.18f, 3.2f, FColor(35, 110, 30)},
		{TEXT("Pinus"),           7.5f, 0.12f, 2.0f, FColor(20, 80, 30)},
		{TEXT("Fagus"),           9.5f, 0.18f, 3.0f, FColor(45, 125, 40)},
		{TEXT("Betula"),          8.0f, 0.10f, 2.5f, FColor(55, 135, 45)},
		{TEXT("Populus"),        11.0f, 0.22f, 3.8f, FColor(60, 145, 50)},
		{TEXT("Salix"),           7.0f, 0.12f, 2.2f, FColor(45, 120, 35)},
		{TEXT("Prunus"),          6.5f, 0.10f, 2.0f, FColor(50, 130, 40)},
		{TEXT("Acer"),            8.0f, 0.16f, 2.8f, FColor(40, 115, 35)},
	};

	int32 Generados = 0;
	for (const FSpecieArbol& Sp : Especies)
	{
		UStaticMesh* Mesh = nullptr;
		GenerarArbolConico(Mesh, Sp.Nombre, Sp.Altura, Sp.RadioTronco, Sp.RadioCopa, 8);
		if (Mesh) ++Generados;
	}

	UE_LOG(LogTemp, Log, TEXT("[Árboles] %d/%d meshes generados en /Game/Meshes/Arboles/"), Generados, Especies.Num());
	return Generados == Especies.Num();
}

// ═══════════════════════════════════════════════════════════════════════════
//  Paso 4: Generar mobiliario urbano
// ═══════════════════════════════════════════════════════════════════════════
// Genera meshes procedurales simples para farolas, bancos, papeleras, fuentes.
static void GenerarCaja(UStaticMesh*& OutMesh, const FString& Nombre, float Ancho, float Alto, float Profundo, FColor Color)
{
	TArray<FVector> V;
	TArray<int32> T;
	TArray<FVector> N;
	TArray<FVector2D> UV;
	TArray<FColor> C;

	float hx = Ancho * 0.5f, hy = Profundo * 0.5f;
	// 6 caras, 4 vértices cada una = 24 vértices.
	// Frente (Z+)
	V.Append({FVector(-hx, -hy, 0), FVector(hx, -hy, 0), FVector(hx, -hy, Alto), FVector(-hx, -hy, Alto)});
	N.Append({FVector(0,-1,0), FVector(0,-1,0), FVector(0,-1,0), FVector(0,-1,0)});
	UV.Append({FVector2D(0,0), FVector2D(1,0), FVector2D(1,1), FVector2D(0,1)});
	C.Append({Color, Color, Color, Color});
	// Atrás (Z+)
	V.Append({FVector(hx, hy, 0), FVector(-hx, hy, 0), FVector(-hx, hy, Alto), FVector(hx, hy, Alto)});
	N.Append({FVector(0,1,0), FVector(0,1,0), FVector(0,1,0), FVector(0,1,0)});
	UV.Append({FVector2D(0,0), FVector2D(1,0), FVector2D(1,1), FVector2D(0,1)});
	C.Append({Color, Color, Color, Color});
	// Left
	V.Append({FVector(-hx, hy, 0), FVector(-hx, -hy, 0), FVector(-hx, -hy, Alto), FVector(-hx, hy, Alto)});
	N.Append({FVector(-1,0,0), FVector(-1,0,0), FVector(-1,0,0), FVector(-1,0,0)});
	UV.Append({FVector2D(0,0), FVector2D(1,0), FVector2D(1,1), FVector2D(0,1)});
	C.Append({Color, Color, Color, Color});
	// Right
	V.Append({FVector(hx, -hy, 0), FVector(hx, hy, 0), FVector(hx, hy, Alto), FVector(hx, -hy, Alto)});
	N.Append({FVector(1,0,0), FVector(1,0,0), FVector(1,0,0), FVector(1,0,0)});
	UV.Append({FVector2D(0,0), FVector2D(1,0), FVector2D(1,1), FVector2D(0,1)});
	C.Append({Color, Color, Color, Color});
	// Top
	V.Append({FVector(-hx, -hy, Alto), FVector(hx, -hy, Alto), FVector(hx, hy, Alto), FVector(-hx, hy, Alto)});
	N.Append({FVector(0,0,1), FVector(0,0,1), FVector(0,0,1), FVector(0,0,1)});
	UV.Append({FVector2D(0,0), FVector2D(1,0), FVector2D(1,1), FVector2D(0,1)});
	C.Append({Color, Color, Color, Color});
	// Bottom
	V.Append({FVector(-hx, hy, 0), FVector(hx, hy, 0), FVector(hx, -hy, 0), FVector(-hx, -hy, 0)});
	N.Append({FVector(0,0,-1), FVector(0,0,-1), FVector(0,0,-1), FVector(0,0,-1)});
	UV.Append({FVector2D(0,0), FVector2D(1,0), FVector2D(1,1), FVector2D(0,1)});
	C.Append({Color, Color, Color, Color});

	for (int32 Face = 0; Face < 6; ++Face)
	{
		int32 B = Face * 4;
		T.Append({B, B+1, B+2, B, B+2, B+3});
	}

	UStaticMesh* Mesh = NewObject<UStaticMesh>();
	Mesh->GetStaticMaterials().Add(FStaticMaterial());

	FMeshDescription MD;
	FStaticMeshAttributes Attr(MD);
	Attr.CreateVertexAttributes();
	Attr.CreatePolygonAttributes();
	Attr.CreatePolygonGroupAttributes();
	Attr.CreateEdgeAttributes();

	TArray<MeshDescription::VertexInstanceID> VIDs;
	for (int32 i = 0; i < V.Num(); ++i)
	{
		MeshDescription::VertexID VID2 = MD.AppendVertex(V[i]);
		MeshDescription::VertexInstanceID InstID = MD.AppendVertexInstance(VID2, N[i], UV[i], C[i]);
		VIDs.Add(InstID);
	}

	MeshDescription::PolygonGroupID PGID = MD.AppendPolygonGroup();
	for (int32 i = 0; i < T.Num(); i += 3)
	{
		MD.AppendPolygon(PGID, {VIDs[T[i]], VIDs[T[i+1]], VIDs[T[i+2]]});
	}

	TArray<const FMeshDescription*> BulkData;
	BulkData.Add(&MD);
	Mesh->BuildFromMeshDescriptions(BulkData);

	IAssetTools& AT = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	AT.CreateAsset(Nombre, TEXT("/Game/Meshes/Mobiliario"), UStaticMesh::StaticClass(), nullptr);
	FString Ruta = FString::Printf(TEXT("/Game/Meshes/Mobiliario/SM_%s"), *Nombre);
	if (UStaticMesh* Created = LoadObject<UStaticMesh>(nullptr, *(Ruta + TEXT(".")) + Nombre))
	{
		Created->BuildFromMeshDescriptions(BulkData);
		UEditorAssetLibrary::SaveAsset(Ruta, false);
		OutMesh = Created;
	}
}

bool UAlsasuaAssetGenerator::GenerarMobiliarioUrbano()
{
	// Helper: build mesh from arrays of verts/tris/colors
	auto BuildStaticMesh = [&](const FString& Nombre, const TArray<FVector>& Verts, const TArray<int32>& Tris,
		const TArray<FColor>& Colors) -> UStaticMesh*
	{
		if (Verts.Num() < 3 || Tris.Num() < 3) return nullptr;

		TArray<FVector> Norms;
		TArray<FVector2D> UVs;
		for (int32 i = 0; i < Verts.Num(); ++i)
		{
			Norms.Add(FVector(0, 0, 1));
			UVs.Add(FVector2D(Verts[i].X * 0.01f, Verts[i].Y * 0.01f));
		}

		FMeshDescription MD;
		FStaticMeshAttributes Attr(MD);
		Attr.CreateVertexAttributes();
		Attr.CreatePolygonAttributes();
		Attr.CreatePolygonGroupAttributes();
		Attr.CreateEdgeAttributes();

		TArray<MeshDescription::VertexInstanceID> VIDs;
		for (int32 i = 0; i < Verts.Num(); ++i)
		{
			MeshDescription::VertexID VID2 = MD.AppendVertex(Verts[i]);
			MeshDescription::VertexInstanceID InstID = MD.AppendVertexInstance(VID2, Norms[i], UVs[i], Colors[i]);
			VIDs.Add(InstID);
		}

		MeshDescription::PolygonGroupID PGID = MD.AppendPolygonGroup();
		for (int32 i = 0; i < Tris.Num(); i += 3)
		{
			MD.AppendPolygon(PGID, {VIDs[Tris[i]], VIDs[Tris[i+1]], VIDs[Tris[i+2]]});
		}

		TArray<const FMeshDescription*> BulkData;
		BulkData.Add(&MD);

		IAssetTools& AT = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		AT.CreateAsset(Nombre, TEXT("/Game/Meshes/Mobiliario"), UStaticMesh::StaticClass(), nullptr);
		FString Ruta = FString::Printf(TEXT("/Game/Meshes/Mobiliario/SM_%s"), *Nombre);
		if (UStaticMesh* Created = LoadObject<UStaticMesh>(nullptr, *(Ruta + TEXT(".")) + Nombre))
		{
			Created->BuildFromMeshDescriptions(BulkData);
			UEditorAssetLibrary::SaveAsset(Ruta, false);
			return Created;
		}
		return nullptr;
	};

	// Multi-box combiner: accumulates verts/tris/colors from multiple boxes
	auto AddBox = [&](TArray<FVector>& V, TArray<int32>& T, TArray<FColor>& C,
		FVector Center, float Hx, float Hy, float Hz, FColor Col)
	{
		int32 B = V.Num();
		FVector X(Hx, 0, 0), Y(0, Hy, 0), Z(0, 0, Hz);
		// 8 vertices
		V.Append({Center - X - Y - Z, Center + X - Y - Z, Center + X + Y - Z, Center - X + Y - Z,
		           Center - X - Y + Z, Center + X - Y + Z, Center + X + Y + Z, Center - X + Y + Z});
		for (int32 k = 0; k < 8; ++k) C.Add(Col);
		// 12 triangles (6 faces)
		int32 F2[6][4] = {{0,1,2,3},{4,5,6,7},{0,1,5,4},{2,3,7,6},{0,3,7,4},{1,2,6,5}};
		for (int32 f = 0; f < 6; ++f)
		{
			T.Append({B+F2[f][0], B+F2[f][1], B+F2[f][2], B+F2[f][0], B+F2[f][2], B+F2[f][3]});
		}
	};

	int32 Generados = 0;
	UStaticMesh* M = nullptr;

	// ══ FAROLA: poste + brazo + linterna ══
	{
		TArray<FVector> V; TArray<int32> T; TArray<FColor> C;
		const FColor MetalOscuro(50, 50, 55);
		const FColor MetalClaro(80, 80, 85);
		const FColor Linterna(255, 240, 200);

		AddBox(V, T, C, FVector(0, 0, 150), 6, 6, 150, MetalOscuro);   // poste base
		AddBox(V, T, C, FVector(0, 0, 350), 4, 4, 200, MetalOscuro);   // poste alto
		AddBox(V, T, C, FVector(0, 0, 560), 8, 8, 10, MetalClaro);     // unión brazo
		AddBox(V, T, C, FVector(0, 60, 565), 4, 60, 4, MetalClaro);    // brazo horizontal
		AddBox(V, T, C, FVector(0, 120, 555), 15, 8, 12, Linterna);    // linterna
		AddBox(V, T, C, FVector(0, 120, 548), 18, 12, 4, MetalClaro);  // deflector

		M = BuildStaticMesh(TEXT("Farola"), V, T, C);
		if (M) ++Generados;
	}

	// ══ BANCO: asiento + respaldo + 4 patas ══
	{
		TArray<FVector> V; TArray<int32> T; TArray<FColor> C;
		const FColor Madera(120, 80, 40);
		const FColor Metal(60, 60, 65);

		// Asiento
		AddBox(V, T, C, FVector(0, 0, 45), 90, 20, 4, Madera);
		// Respaldo
		AddBox(V, T, C, FVector(0, -18, 60), 90, 3, 25, Madera);
		// Patas (4)
		AddBox(V, T, C, FVector(-75, -12, 0), 4, 4, 22, Metal);
		AddBox(V, T, C, FVector(75, -12, 0), 4, 4, 22, Metal);
		AddBox(V, T, C, FVector(-75, 12, 0), 4, 4, 22, Metal);
		AddBox(V, T, C, FVector(75, 12, 0), 4, 4, 22, Metal);
		// Soporte lateral
		AddBox(V, T, C, FVector(-75, 0, 22), 4, 12, 3, Metal);
		AddBox(V, T, C, FVector(75, 0, 22), 4, 12, 3, Metal);

		M = BuildStaticMesh(TEXT("Banco"), V, T, C);
		if (M) ++Generados;
	}

	// ══ PAPELERA: cuerpo + tapa + abertura ══
	{
		TArray<FVector> V; TArray<int32> T; TArray<FColor> C;
		const FColor Gris(65, 65, 70);
		const FColor Tapa(50, 50, 55);
		const FColor Interior(30, 30, 30);

		AddBox(V, T, C, FVector(0, 0, 40), 18, 18, 40, Gris);       // cuerpo
		AddBox(V, T, C, FVector(0, 0, 82), 20, 20, 3, Tapa);        // tapa
		AddBox(V, T, C, FVector(0, 0, 86), 8, 2, 5, Gris);          // asa tapa
		AddBox(V, T, C, FVector(0, 18, 50), 10, 2, 20, Interior);   // abertura frontal

		M = BuildStaticMesh(TEXT("Papelera"), V, T, C);
		if (M) ++Generados;
	}

	// ══ FUENTE: base + taza + columnita ══
	{
		TArray<FVector> V; TArray<int32> T; TArray<FColor> C;
		const FColor Piedra(180, 170, 155);
		const FColor PiedraOsc(140, 130, 120);

		AddBox(V, T, C, FVector(0, 0, 15), 100, 100, 30, Piedra);     // base
		AddBox(V, T, C, FVector(0, 0, 35), 80, 80, 10, Piedra);       // taza
		AddBox(V, T, C, FVector(0, 0, 40), 60, 60, 5, PiedraOsc);    // interior taza
		AddBox(V, T, C, FVector(0, 0, 60), 8, 8, 25, Piedra);        // columnita
		AddBox(V, T, C, FVector(0, 0, 88), 20, 20, 5, PiedraOsc);    // copa superior

		M = BuildStaticMesh(TEXT("Fuente"), V, T, C);
		if (M) ++Generados;
	}

	// ══ PARADA BUS: poste + techo + panel + banco ══
	{
		TArray<FVector> V; TArray<int32> T; TArray<FColor> C;
		const FColor Metal(70, 70, 75);
		const FColor Cristal(100, 140, 180);
		const FColor Techo(80, 80, 85);
		const FColor Asiento(100, 80, 50);

		AddBox(V, T, C, FVector(-80, 0, 0), 4, 4, 150, Metal);       // poste izq
		AddBox(V, T, C, FVector(80, 0, 0), 4, 4, 150, Metal);        // poste der
		AddBox(V, T, C, FVector(0, 0, 155), 90, 40, 4, Techo);       // techo
		AddBox(V, T, C, FVector(0, -35, 90), 80, 2, 80, Cristal);    // panel trasero
		AddBox(V, T, C, FVector(0, 30, 30), 60, 4, 4, Asiento);      // banco

		M = BuildStaticMesh(TEXT("ParadaBus"), V, T, C);
		if (M) ++Generados;
	}

	// ══ SEMAFORO: poste + 3 cajas señal + brazo ══
	{
		TArray<FVector> V; TArray<int32> T; TArray<FColor> C;
		const FColor MetalOsc(40, 40, 45);
		const FColor Caja(50, 50, 55);
		const FColor Rojo(200, 30, 30);
		const FColor Amarillo(220, 180, 30);
		const FColor Verde(30, 180, 40);

		AddBox(V, T, C, FVector(0, 0, 150), 5, 5, 150, MetalOsc);    // poste
		AddBox(V, T, C, FVector(0, 0, 310), 10, 10, 5, MetalOsc);    // unión
		AddBox(V, T, C, FVector(0, 0, 340), 15, 12, 45, Caja);       // caja señales
		AddBox(V, T, C, FVector(0, 0, 360), 8, 6, 8, Rojo);          // luz roja
		AddBox(V, T, C, FVector(0, 0, 340), 8, 6, 8, Amarillo);      // luz amarilla
		AddBox(V, T, C, FVector(0, 0, 320), 8, 6, 8, Verde);         // luz verde
		AddBox(V, T, C, FVector(0, 40, 310), 3, 40, 3, MetalOsc);    // brazo

		M = BuildStaticMesh(TEXT("Semaforo"), V, T, C);
		if (M) ++Generados;
	}

	UE_LOG(LogTemp, Log, TEXT("[Mobiliario] %d meshes detallados generados en /Game/Meshes/Mobiliario/"), Generados);
	return Generados > 0;
}

// ═══════════════════════════════════════════════════════════════════════════
//  Paso 5: Generar landmarks de Alsasua
// ═══════════════════════════════════════════════════════════════════════════
bool UAlsasuaAssetGenerator::GenerarLandmarks()
{
	// Helper: build mesh from arrays of verts/tris/colors
	auto BuildStaticMesh = [&](const FString& Nombre, const TArray<FVector>& Verts, const TArray<int32>& Tris,
		const TArray<FColor>& Colors) -> UStaticMesh*
	{
		if (Verts.Num() < 3 || Tris.Num() < 3) return nullptr;

		TArray<FVector> Norms;
		TArray<FVector2D> UVs;
		for (int32 i = 0; i < Verts.Num(); ++i)
		{
			Norms.Add(FVector(0, 0, 1));
			UVs.Add(FVector2D(Verts[i].X * 0.005f, Verts[i].Y * 0.005f));
		}

		FMeshDescription MD;
		FStaticMeshAttributes Attr(MD);
		Attr.CreateVertexAttributes();
		Attr.CreatePolygonAttributes();
		Attr.CreatePolygonGroupAttributes();
		Attr.CreateEdgeAttributes();

		TArray<MeshDescription::VertexInstanceID> VIDs;
		for (int32 i = 0; i < Verts.Num(); ++i)
		{
			MeshDescription::VertexID VID2 = MD.AppendVertex(Verts[i]);
			MeshDescription::VertexInstanceID InstID = MD.AppendVertexInstance(VID2, Norms[i], UVs[i], Colors[i]);
			VIDs.Add(InstID);
		}

		MeshDescription::PolygonGroupID PGID = MD.AppendPolygonGroup();
		for (int32 i = 0; i < Tris.Num(); i += 3)
		{
			MD.AppendPolygon(PGID, {VIDs[Tris[i]], VIDs[Tris[i+1]], VIDs[Tris[i+2]]});
		}

		TArray<const FMeshDescription*> BulkData;
		BulkData.Add(&MD);

		IAssetTools& AT = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		AT.CreateAsset(Nombre, TEXT("/Game/Meshes/Landmarks"), UStaticMesh::StaticClass(), nullptr);
		FString Ruta = FString::Printf(TEXT("/Game/Meshes/Landmarks/SM_%s"), *Nombre);
		if (UStaticMesh* Created = LoadObject<UStaticMesh>(nullptr, *(Ruta + TEXT(".")) + Nombre))
		{
			Created->BuildFromMeshDescriptions(BulkData);
			UEditorAssetLibrary::SaveAsset(Ruta, false);
			return Created;
		}
		return nullptr;
	};

	auto AddBox = [&](TArray<FVector>& V, TArray<int32>& T, TArray<FColor>& C,
		FVector Center, float Hx, float Hy, float Hz, FColor Col)
	{
		int32 B = V.Num();
		FVector X(Hx, 0, 0), Y(0, Hy, 0), Z(0, 0, Hz);
		V.Append({Center - X - Y - Z, Center + X - Y - Z, Center + X + Y - Z, Center - X + Y - Z,
		           Center - X - Y + Z, Center + X - Y + Z, Center + X + Y + Z, Center - X + Y + Z});
		for (int32 k = 0; k < 8; ++k) C.Add(Col);
		int32 F2[6][4] = {{0,1,2,3},{4,5,6,7},{0,1,5,4},{2,3,7,6},{0,3,7,4},{1,2,6,5}};
		for (int32 f = 0; f < 6; ++f)
		{
			T.Append({B+F2[f][0], B+F2[f][1], B+F2[f][2], B+F2[f][0], B+F2[f][2], B+F2[f][3]});
		}
	};

	int32 Generados = 0;
	UStaticMesh* M = nullptr;

	// ══ IGLESIA: nave + cabecera + torre-campanario + tejado ══
	{
		TArray<FVector> V; TArray<int32> T; TArray<FColor> C;
		const FColor Piedra(200, 190, 175);
		const FColor PiedraOscura(170, 160, 150);
		const FColor Tejado(160, 60, 40);
		const FColor Cristal(180, 200, 220);

		// Nave principal (12x25x10m)
		AddBox(V, T, C, FVector(0, 0, 500), 600, 1250, 500, Piedra);
		// Tejado nave (a dos aguas simulado con caja aplanada)
		AddBox(V, T, C, FVector(0, 0, 1020), 650, 1300, 80, Tejado);
		// Cabecera (ábside, más estrecho)
		AddBox(V, T, C, FVector(0, 1300, 450), 500, 400, 450, PiedraOscura);
		// Torre-campanario (4x4x18m)
		AddBox(V, T, C, FVector(-500, -200, 900), 200, 200, 900, PiedraOscura);
		// Campanario tejado piramidal
		AddBox(V, T, C, FVector(-500, -200, 1820), 250, 250, 100, Tejado);
		// Ventanales (cristal)
		AddBox(V, T, C, FVector(610, -400, 500), 10, 30, 150, Cristal);
		AddBox(V, T, C, FVector(610, 0, 500), 10, 30, 150, Cristal);
		AddBox(V, T, C, FVector(610, 400, 500), 10, 30, 150, Cristal);
		AddBox(V, T, C, FVector(-610, -400, 500), 10, 30, 150, Cristal);
		AddBox(V, T, C, FVector(-610, 0, 500), 10, 30, 150, Cristal);
		AddBox(V, T, C, FVector(-610, 400, 500), 10, 30, 150, Cristal);
		// Puerta principal
		AddBox(V, T, C, FVector(0, -1260, 200), 120, 10, 300, PiedraOscura);

		M = BuildStaticMesh(TEXT("Iglesia_Completa"), V, T, C);
		if (M) ++Generados;
	}

	// ══ AYUNTAMIENTO: planta baja soportales + planta noble + tejado ══
	{
		TArray<FVector> V; TArray<int32> T; TArray<FColor> C;
		const FColor Silleria(210, 200, 185);
		const FColor SilleriaOscura(185, 175, 165);
		const FColor Tejado(140, 70, 50);

		// Cuerpo principal (20x10x8m)
		AddBox(V, T, C, FVector(0, 0, 400), 1000, 500, 400, Silleria);
		// Tejado
		AddBox(V, T, C, FVector(0, 0, 820), 1050, 550, 60, Tejado);
		// Soportales (columnas)
		for (int32 i = -3; i <= 3; ++i)
		{
			AddBox(V, T, C, FVector(i * 130, -510, 150), 20, 20, 300, SilleriaOscura);
		}
		// Arco soportal
		AddBox(V, T, C, FVector(0, -510, 310), 950, 20, 30, SilleriaOscura);
		// Ventanas planta noble
		for (int32 i = -2; i <= 2; ++i)
		{
			AddBox(V, T, C, FVector(i * 180, -510, 500), 40, 15, 80, Cristal);
		}
		// Escaleras
		AddBox(V, T, C, FVector(0, -520, 0), 200, 60, 40, SilleriaOscura);
		// Escudo/heráldico
		AddBox(V, T, C, FVector(0, -515, 380), 60, 10, 60, FColor(180, 30, 30));

		M = BuildStaticMesh(TEXT("Ayuntamiento"), V, T, C);
		if (M) ++Generados;
	}

	// ══ ESTACIÓN TREN: cuerpo largo + andén + marquesina ══
	{
		TArray<FVector> V; TArray<int32> T; TArray<FColor> C;
		const FColor Ladrillo(195, 185, 170);
		const FColor Metal(100, 100, 105);
		const FColor Cristal(180, 200, 220);

		// Cuerpo estación (30x10x7m)
		AddBox(V, T, C, FVector(0, 0, 350), 1500, 500, 350, Ladrillo);
		// Tejado
		AddBox(V, T, C, FVector(0, 0, 720), 1550, 550, 50, FColor(140, 70, 50));
		// Andén (extiende más allá del cuerpo)
		AddBox(V, T, C, FVector(0, 600, 5), 1600, 400, 10, Metal);
		// Marquesina andén (estructura metálica)
		AddBox(V, T, C, FVector(0, 600, 250), 1600, 400, 15, Metal);
		// Soportes marquesina
		for (int32 i = -5; i <= 5; ++i)
		{
			AddBox(V, T, C, FVector(i * 250, 400, 130), 8, 8, 250, Metal);
			AddBox(V, T, C, FVector(i * 250, 800, 130), 8, 8, 250, Metal);
		}
		// Ventanas
		for (int32 i = -4; i <= 4; ++i)
		{
			AddBox(V, T, C, FVector(i * 150, -510, 350), 50, 10, 70, Cristal);
		}
		// Reloj
		AddBox(V, T, C, FVector(0, -515, 600), 60, 10, 60, FColor(240, 240, 230));

		M = BuildStaticMesh(TEXT("EstacionTren"), V, T, C);
		if (M) ++Generados;
	}

	// ══ FRONTÓN: pista + gradas + techo ══
	{
		TArray<FVector> V; TArray<int32> T; TArray<FColor> C;
		const FColor MuroBlanco(230, 228, 220);
		const FColor MuroLateral(200, 195, 185);
		const FColor Grada(170, 160, 150);

		// Pista (15x30m)
		AddBox(V, T, C, FVector(0, 0, 5), 750, 1500, 5, Grada);
		// Muro frontal
		AddBox(V, T, C, FVector(-760, 0, 600), 10, 1500, 600, MuroBlanco);
		// Muros laterales
		AddBox(V, T, C, FVector(0, -1510, 600), 750, 10, 600, MuroLateral);
		AddBox(V, T, C, FVector(0, 1510, 600), 750, 10, 600, MuroLateral);
		// Grada
		for (int32 i = 0; i < 5; ++i)
		{
			AddBox(V, T, C, FVector(600, 0, 50 + i * 60), 100, 1400, 50, Grada);
		}
		// Techo
		AddBox(V, T, C, FVector(0, 0, 1250), 800, 1550, 30, MuroLateral);

		M = BuildStaticMesh(TEXT("Fronton"), V, T, C);
		if (M) ++Generados;
	}

	// ══ MONUMENTO NOGAL: base + columna + copa nogal ══
	{
		TArray<FVector> V; TArray<int32> T; TArray<FColor> C;
		const FColor Piedra(160, 140, 120);
		const FColor Bronce(120, 90, 50);
		const FColor Copa(40, 100, 30);

		// Base escalonada
		AddBox(V, T, C, FVector(0, 0, 15), 200, 200, 30, Piedra);
		AddBox(V, T, C, FVector(0, 0, 40), 150, 150, 20, Piedra);
		// Columna
		AddBox(V, T, C, FVector(0, 0, 200), 25, 25, 160, Piedra);
		// Copa nogal (esfera simulada con caja)
		AddBox(V, T, C, FVector(0, 0, 380), 100, 100, 80, Copa);
		// Placa conmemorativa
		AddBox(V, T, C, FVector(0, -105, 120), 60, 5, 40, Bronce);

		M = BuildStaticMesh(TEXT("MonumentoNogal"), V, T, C);
		if (M) ++Generados;
	}

	// ══ TORRE BASALUZ: torre circular + aspillera + merlones ══
	{
		TArray<FVector> V; TArray<int32> T; TArray<FColor> C;
		const FColor Piedra(170, 160, 145);
		const FColor PiedraOscura(140, 130, 120);

		// Torre base (cilíndrica simulada con 8 cajas cruzadas)
		for (int32 i = 0; i < 4; ++i)
		{
			float Ang = PI * i / 4;
			FVector Offset(FMath::Cos(Ang) * 80, FMath::Sin(Ang) * 80, 0);
			AddBox(V, T, C, Offset + FVector(0, 0, 250), 60, 60, 250, Piedra);
		}
		// Base torre
		AddBox(V, T, C, FVector(0, 0, 15), 200, 200, 30, PiedraOscura);
		// Coronación
		AddBox(V, T, C, FVector(0, 0, 515), 180, 180, 30, PiedraOscura);
		// Merlones
		for (int32 i = 0; i < 8; ++i)
		{
			float Ang = PI * i / 4;
			FVector Offset(FMath::Cos(Ang) * 140, FMath::Sin(Ang) * 140, 0);
			AddBox(V, T, C, Offset + FVector(0, 0, 555), 25, 25, 40, PiedraOscura);
		}
		// Aspillera
		AddBox(V, T, C, FVector(0, -165, 350), 8, 20, 60, FColor(20, 20, 25));

		M = BuildStaticMesh(TEXT("TorreBasaluz"), V, T, C);
		if (M) ++Generados;
	}

	UE_LOG(LogTemp, Log, TEXT("[Landmarks] %d landmarks detallados generados en /Game/Meshes/Landmarks/"), Generados);
	return Generados > 0;
}

// ═══════════════════════════════════════════════════════════════════════════
//  Paso 6: Generar ríos (lecho + bancas + superficie agua)
// ═══════════════════════════════════════════════════════════════════════════
bool UAlsasuaAssetGenerator::GenerarRios()
{
	bool bLecho = UAlsasuaRiverMeshGenerator::GenerarLechoRio();
	bool bBancas = UAlsasuaRiverMeshGenerator::GenerarBancasRio();
	if (bLecho) UE_LOG(LogTemp, Log, TEXT("[AssetGen] Paso 7/9: Lechos de río generados"));
	if (bBancas) UE_LOG(LogTemp, Log, TEXT("[AssetGen] Paso 7/9: Bancas de río generadas"));
	return bLecho || bBancas;
}

// ═══════════════════════════════════════════════════════════════════════════
//  Paso 7: Generar puentes (piedra arco + hierro celosía)
// ═══════════════════════════════════════════════════════════════════════════
bool UAlsasuaAssetGenerator::GenerarPuentes()
{
	return UAlsasuaBridgeGenerator::GenerarPuentesMejorados();
}

// ═══════════════════════════════════════════════════════════════════════════
//  Paso 8: Scan Fab Megascans foliage
// ═══════════════════════════════════════════════════════════════════════════
bool UAlsasuaAssetGenerator::ScanFoliage()
{
	return UAlsasuaFoliageLoader::ScanAndRegisterFoliage();
}

// ═══════════════════════════════════════════════════════════════════════════
//  Paso 9: Crear materiales PBR (calles, acera, muro piedra, tejas)
// ═══════════════════════════════════════════════════════════════════════════
bool UAlsasuaAssetGenerator::CrearMaterialesPBR()
{
	bool bOK = true;

	if (!UCreadorMaterialCalles::CrearMaterialCalles())
	{
		UE_LOG(LogTemp, Error, TEXT("[MaterialesPBR] Fallo creando M_Terreno_Calles"));
		bOK = false;
	}

	if (!UCreadorMaterialCalles::CrearMaterialAcera())
	{
		UE_LOG(LogTemp, Error, TEXT("[MaterialesPBR] Fallo creando M_Terreno_Acera"));
		bOK = false;
	}

	if (!UCreadorMaterialMuroPiedra::CrearMaterialMuroPiedra())
	{
		UE_LOG(LogTemp, Error, TEXT("[MaterialesPBR] Fallo creando M_Muro_Piedra"));
		bOK = false;
	}

	if (!UCreadorMaterialTejas::CrearMaterialTejas())
	{
		UE_LOG(LogTemp, Error, TEXT("[MaterialesPBR] Fallo creando M_Techo_Tejas"));
		bOK = false;
	}

	if (!UCreadorMaterialMobiliario::CrearMaterialMobiliario())
	{
		UE_LOG(LogTemp, Error, TEXT("[MaterialesPBR] Fallo creando M_Mobiliario"));
		bOK = false;
	}

	return bOK;
}
