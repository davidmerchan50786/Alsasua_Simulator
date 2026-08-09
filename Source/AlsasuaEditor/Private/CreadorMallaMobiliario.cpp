// CreadorMallaMobiliario.cpp (sólo editor)
#include "CreadorMallaMobiliario.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "StaticMeshAttributes.h"
#include "MeshDescription.h"
#include "MeshDescriptionBuilder.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorAssetLibrary.h"
#include "UObject/Package.h"
#include "Misc/Paths.h"

namespace
{
	const TCHAR* CarpetaMobiliario = TEXT("/Game/Mobiliario");

	// Paleta del mobiliario de Alsasua. Va a color por vértice, y los
	// materiales la usan de tinte: M_Mobiliario sobre la veta de madera y
	// M_Metal sobre la chapa.
	const FLinearColor Madera      (0.42f, 0.26f, 0.14f);
	const FLinearColor HierroVerde (0.10f, 0.18f, 0.13f);   // verde Navarra
	const FLinearColor HierroNegro (0.06f, 0.06f, 0.07f);
	const FLinearColor Fundicion   (0.20f, 0.20f, 0.22f);
	const FLinearColor RojoCorreos (0.55f, 0.06f, 0.06f);
	const FLinearColor Piedra      (0.62f, 0.60f, 0.55f);
	const FLinearColor Terracota   (0.55f, 0.28f, 0.18f);

	/**
	 * Compone mallas a base de cajas.
	 *
	 * El orden de los vértices de cada cara es antihorario visto desde fuera.
	 * Las normales van explícitas por instancia, así que la iluminación es
	 * correcta en cualquier caso; si alguna cara se viera desde dentro, el
	 * arreglo es intercambiar dos índices en Triangulo().
	 */
	struct FConstructorMalla
	{
		FMeshDescription Desc;
		FMeshDescriptionBuilder Builder;
		FPolygonGroupID Grupo;

		FConstructorMalla()
		{
			FStaticMeshAttributes Atributos(Desc);
			Atributos.Register();

			Builder.SetMeshDescription(&Desc);
			Builder.EnablePolyGroups();
			Builder.SetNumUVLayers(1);
			Grupo = Builder.AppendPolygonGroup();
		}

		FConstructorMalla(const FConstructorMalla&) = delete;
		FConstructorMalla& operator=(const FConstructorMalla&) = delete;

		/** Una cara plana de cuatro esquinas con su normal y su color. */
		void Cara(const FVector& A, const FVector& B, const FVector& C, const FVector& D,
			const FVector& Normal, const FLinearColor& Color, float UVporCm)
		{
			// Ejes en el plano de la cara para sacar UV a escala de mundo: así
			// la textura tesela al mismo tamaño en todas las piezas.
			const FVector EjeU = (B - A).GetSafeNormal();
			const FVector EjeV = (D - A).GetSafeNormal();

			const FVector Esquinas[4] = { A, B, C, D };
			FVertexInstanceID Instancias[4];

			for (int32 i = 0; i < 4; ++i)
			{
				const FVertexID Vertice = Builder.AppendVertex(Esquinas[i]);
				const FVertexInstanceID Instancia = Builder.AppendInstance(Vertice);

				Builder.SetInstanceNormal(Instancia, Normal);
				Builder.SetInstanceColor(Instancia, FVector4f(Color.R, Color.G, Color.B, 1.f));

				const FVector Rel = Esquinas[i] - A;
				Builder.SetInstanceUV(Instancia,
					FVector2D(FVector::DotProduct(Rel, EjeU) * UVporCm,
					          FVector::DotProduct(Rel, EjeV) * UVporCm), 0);

				Instancias[i] = Instancia;
			}

			Builder.AppendTriangle(Instancias[0], Instancias[1], Instancias[2], Grupo);
			Builder.AppendTriangle(Instancias[0], Instancias[2], Instancias[3], Grupo);
		}

		/** Triángulo suelto, para las tapas en abanico de los prismas. */
		void Triangulo(const FVector& A, const FVector& B, const FVector& C2,
			const FVector& Normal, const FLinearColor& Color, float UVporCm)
		{
			const FVector Esquinas[3] = { A, B, C2 };
			const FVector EjeU = (B - A).GetSafeNormal();
			const FVector EjeV = FVector::CrossProduct(Normal, EjeU).GetSafeNormal();

			FVertexInstanceID Instancias[3];
			for (int32 i = 0; i < 3; ++i)
			{
				const FVertexID Vertice = Builder.AppendVertex(Esquinas[i]);
				const FVertexInstanceID Instancia = Builder.AppendInstance(Vertice);

				Builder.SetInstanceNormal(Instancia, Normal);
				Builder.SetInstanceColor(Instancia, FVector4f(Color.R, Color.G, Color.B, 1.f));

				const FVector Rel = Esquinas[i] - A;
				Builder.SetInstanceUV(Instancia,
					FVector2D(FVector::DotProduct(Rel, EjeU) * UVporCm,
					          FVector::DotProduct(Rel, EjeV) * UVporCm), 0);

				Instancias[i] = Instancia;
			}

			Builder.AppendTriangle(Instancias[0], Instancias[1], Instancias[2], Grupo);
		}

		/** Caja alineada a los ejes. Centro y medidas en cm. */
		void Caja(const FVector& Centro, const FVector& Medidas, const FLinearColor& Color, float UVporCm = 0.01f)
		{
			const FVector H = Medidas * 0.5f;
			const float X0 = Centro.X - H.X, X1 = Centro.X + H.X;
			const float Y0 = Centro.Y - H.Y, Y1 = Centro.Y + H.Y;
			const float Z0 = Centro.Z - H.Z, Z1 = Centro.Z + H.Z;

			// Arriba y abajo
			Cara(FVector(X0, Y0, Z1), FVector(X1, Y0, Z1), FVector(X1, Y1, Z1), FVector(X0, Y1, Z1), FVector( 0,  0,  1), Color, UVporCm);
			Cara(FVector(X0, Y1, Z0), FVector(X1, Y1, Z0), FVector(X1, Y0, Z0), FVector(X0, Y0, Z0), FVector( 0,  0, -1), Color, UVporCm);
			// Laterales
			Cara(FVector(X0, Y0, Z0), FVector(X1, Y0, Z0), FVector(X1, Y0, Z1), FVector(X0, Y0, Z1), FVector( 0, -1,  0), Color, UVporCm);
			Cara(FVector(X1, Y1, Z0), FVector(X0, Y1, Z0), FVector(X0, Y1, Z1), FVector(X1, Y1, Z1), FVector( 0,  1,  0), Color, UVporCm);
			Cara(FVector(X1, Y0, Z0), FVector(X1, Y1, Z0), FVector(X1, Y1, Z1), FVector(X1, Y0, Z1), FVector( 1,  0,  0), Color, UVporCm);
			Cara(FVector(X0, Y1, Z0), FVector(X0, Y0, Z0), FVector(X0, Y0, Z1), FVector(X0, Y1, Z1), FVector(-1,  0,  0), Color, UVporCm);
		}

		/** Prisma de N lados alrededor del eje Z: papeleras, bolardos, macetas. */
		void Prisma(const FVector& Base, float RadioAbajo, float RadioArriba, float Altura,
			int32 Lados, const FLinearColor& Color, float UVporCm = 0.01f)
		{
			Lados = FMath::Max(Lados, 3);
			const float Paso = 2.f * PI / Lados;

			for (int32 i = 0; i < Lados; ++i)
			{
				const float A0 = i * Paso;
				const float A1 = (i + 1) * Paso;

				const FVector B0 = Base + FVector(FMath::Cos(A0) * RadioAbajo, FMath::Sin(A0) * RadioAbajo, 0.f);
				const FVector B1 = Base + FVector(FMath::Cos(A1) * RadioAbajo, FMath::Sin(A1) * RadioAbajo, 0.f);
				const FVector T1 = Base + FVector(FMath::Cos(A1) * RadioArriba, FMath::Sin(A1) * RadioArriba, Altura);
				const FVector T0 = Base + FVector(FMath::Cos(A0) * RadioArriba, FMath::Sin(A0) * RadioArriba, Altura);

				// Normal del lateral: perpendicular al borde, hacia fuera.
				const FVector Medio = ((B0 + B1) * 0.5f - Base);
				FVector Normal = FVector(Medio.X, Medio.Y, 0.f).GetSafeNormal();
				if (Normal.IsNearlyZero()) Normal = FVector(1, 0, 0);
				// Si el prisma es cónico la normal se inclina con la pared.
				const float Pendiente = (RadioAbajo - RadioArriba) / FMath::Max(Altura, 1.f);
				Normal = (Normal + FVector(0, 0, Pendiente)).GetSafeNormal();

				Cara(B0, B1, T1, T0, Normal, Color, UVporCm);
			}

			// Tapa superior en abanico desde el centro. Con Cara() salía un
			// cuarto vértice repetido y por tanto un triángulo de área cero.
			if (RadioArriba > KINDA_SMALL_NUMBER)
			{
				const FVector CentroArriba = Base + FVector(0, 0, Altura);
				for (int32 i = 0; i < Lados; ++i)
				{
					const float A0 = i * Paso;
					const float A1 = (i + 1) * Paso;
					const FVector P0 = CentroArriba + FVector(FMath::Cos(A0) * RadioArriba, FMath::Sin(A0) * RadioArriba, 0.f);
					const FVector P1 = CentroArriba + FVector(FMath::Cos(A1) * RadioArriba, FMath::Sin(A1) * RadioArriba, 0.f);
					Triangulo(CentroArriba, P0, P1, FVector(0, 0, 1), Color, UVporCm);
				}
			}
		}
	};

	/** Crea el asset y lo guarda. Material por ruta, para no forzar dependencias. */
	bool Guardar(FConstructorMalla& C, const FString& Nombre, const TCHAR* RutaMaterial)
	{
		const FString Ruta = FString(CarpetaMobiliario) / Nombre;

		if (!UEditorAssetLibrary::DoesDirectoryExist(CarpetaMobiliario))
		{
			UEditorAssetLibrary::MakeDirectory(CarpetaMobiliario);
		}
		if (UEditorAssetLibrary::DoesAssetExist(Ruta))
		{
			UEditorAssetLibrary::DeleteAsset(Ruta);
		}

		UPackage* Paquete = CreatePackage(*Ruta);
		if (!Paquete) return false;

		UStaticMesh* Malla = NewObject<UStaticMesh>(Paquete, FName(*Nombre), RF_Public | RF_Standalone);
		if (!Malla) return false;

		if (UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, RutaMaterial))
		{
			Malla->GetStaticMaterials().Add(FStaticMaterial(Mat));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Mobiliario] %s sin material (%s): crea los materiales primero."), *Nombre, RutaMaterial);
		}

		UStaticMesh::FBuildMeshDescriptionsParams Params;
		Params.bBuildSimpleCollision = true;
		Params.bFastBuild = true;

		TArray<const FMeshDescription*> Descripciones;
		Descripciones.Add(&C.Desc);
		Malla->BuildFromMeshDescriptions(Descripciones, Params);

		FAssetRegistryModule::AssetCreated(Malla);
		Paquete->MarkPackageDirty();
		UEditorAssetLibrary::SaveAsset(Ruta, false);

		UE_LOG(LogTemp, Log, TEXT("[Mobiliario] %s creado en %s"), *Nombre, *Ruta);
		return true;
	}

	const TCHAR* MatMadera = TEXT("/Game/Materiales/M_Mobiliario.M_Mobiliario");
	const TCHAR* MatMetal  = TEXT("/Game/Materiales/M_Metal.M_Metal");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Banco: 24 en street_furniture.json, "madera_piedra".
//  Listones de madera sobre dos pies de fundición, 180 x 45, asiento a 45 cm.
// ─────────────────────────────────────────────────────────────────────────────
bool UCreadorMallaMobiliario::GenerarBanco()
{
	FConstructorMalla C;

	// Cinco listones de asiento con hueco entre ellos.
	for (int32 i = 0; i < 5; ++i)
	{
		const float Y = -18.f + i * 9.f;
		C.Caja(FVector(0.f, Y, 45.f), FVector(180.f, 7.f, 4.f), Madera);
	}
	// Tres listones de respaldo, inclinados de forma sencilla por altura.
	for (int32 i = 0; i < 3; ++i)
	{
		C.Caja(FVector(0.f, 20.f + i * 1.5f, 60.f + i * 9.f), FVector(180.f, 6.f, 4.f), Madera);
	}
	// Pies: pata y zapata a cada lado.
	for (int32 s = -1; s <= 1; s += 2)
	{
		const float X = s * 75.f;
		C.Caja(FVector(X, 0.f, 22.f), FVector(8.f, 42.f, 44.f), Fundicion);
		C.Caja(FVector(X, 0.f, 2.f),  FVector(14.f, 48.f, 4.f), Fundicion);
		C.Caja(FVector(X, 24.f, 62.f), FVector(6.f, 6.f, 40.f), Fundicion);
	}

	return Guardar(C, TEXT("SM_Banco"), MatMadera);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Papelera: 97 piezas, la más repetida del pueblo.
//  Cubo troncocónico de 12 lados sobre poste, con aro superior.
// ─────────────────────────────────────────────────────────────────────────────
bool UCreadorMallaMobiliario::GenerarPapelera()
{
	FConstructorMalla C;

	C.Prisma(FVector(0.f, 0.f, 40.f), 16.f, 19.f, 45.f, 12, HierroVerde);   // cubo
	C.Prisma(FVector(0.f, 0.f, 85.f), 20.f, 20.f, 3.f,  12, HierroNegro);   // aro
	C.Caja(FVector(0.f, 0.f, 20.f), FVector(8.f, 8.f, 40.f), HierroNegro);  // poste
	C.Caja(FVector(0.f, 0.f, 1.5f), FVector(20.f, 20.f, 3.f), Fundicion);   // base

	return Guardar(C, TEXT("SM_Papelera"), MatMetal);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Bolardo: 19 piezas. Pilona de fundición de 90 cm con remate.
// ─────────────────────────────────────────────────────────────────────────────
bool UCreadorMallaMobiliario::GenerarBolardo()
{
	FConstructorMalla C;

	C.Prisma(FVector(0.f, 0.f, 3.f),  9.f, 7.f, 80.f, 10, HierroNegro);
	C.Prisma(FVector(0.f, 0.f, 83.f), 8.f, 4.f, 7.f,  10, HierroNegro);     // remate
	C.Prisma(FVector(0.f, 0.f, 0.f),  12.f, 11.f, 3.f, 10, Fundicion);      // zócalo

	return Guardar(C, TEXT("SM_Bolardo"), MatMetal);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Maceta: 6 piezas. Jardinera troncocónica de terracota.
// ─────────────────────────────────────────────────────────────────────────────
bool UCreadorMallaMobiliario::GenerarMaceta()
{
	FConstructorMalla C;

	C.Prisma(FVector(0.f, 0.f, 0.f),  22.f, 28.f, 40.f, 12, Terracota);
	C.Prisma(FVector(0.f, 0.f, 40.f), 29.f, 29.f, 4.f,  12, Terracota);     // borde
	// La tierra, un poco hundida respecto al borde.
	C.Prisma(FVector(0.f, 0.f, 36.f), 26.f, 26.f, 2.f,  12, FLinearColor(0.16f, 0.11f, 0.07f));

	return Guardar(C, TEXT("SM_Maceta"), MatMadera);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Boca de incendio: 8 piezas. Hidrante bajo con dos salidas.
// ─────────────────────────────────────────────────────────────────────────────
bool UCreadorMallaMobiliario::GenerarBocaIncendio()
{
	FConstructorMalla C;
	const FLinearColor Rojo(0.5f, 0.05f, 0.04f);

	C.Prisma(FVector(0.f, 0.f, 0.f),  14.f, 13.f, 4.f, 10, Fundicion);      // brida
	C.Prisma(FVector(0.f, 0.f, 4.f),  11.f, 10.f, 55.f, 10, Rojo);          // cuerpo
	C.Prisma(FVector(0.f, 0.f, 59.f), 12.f, 8.f, 8.f, 10, Rojo);            // casquete
	// Salidas laterales.
	C.Caja(FVector( 13.f, 0.f, 35.f), FVector(10.f, 9.f, 9.f), Fundicion);
	C.Caja(FVector(-13.f, 0.f, 35.f), FVector(10.f, 9.f, 9.f), Fundicion);

	return Guardar(C, TEXT("SM_BocaIncendio"), MatMetal);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tapa de alcantarilla: 6 piezas. Disco de fundición casi a ras de suelo.
// ─────────────────────────────────────────────────────────────────────────────
bool UCreadorMallaMobiliario::GenerarTapaAlcantarilla()
{
	FConstructorMalla C;

	C.Prisma(FVector(0.f, 0.f, 0.f), 32.f, 32.f, 2.f, 20, Fundicion);       // cerco
	C.Prisma(FVector(0.f, 0.f, 2.f), 29.f, 29.f, 1.5f, 20, Fundicion);      // tapa

	return Guardar(C, TEXT("SM_TapaAlcantarilla"), MatMetal);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Buzón de Correos: 4 piezas. El amarillo de Correos va en la instancia;
//  aquí el color por vértice deja el rojo oscuro de los antiguos.
// ─────────────────────────────────────────────────────────────────────────────
bool UCreadorMallaMobiliario::GenerarBuzonCorreos()
{
	FConstructorMalla C;

	C.Caja(FVector(0.f, 0.f, 65.f), FVector(45.f, 30.f, 60.f), RojoCorreos);   // cuerpo
	C.Caja(FVector(0.f, 0.f, 96.f), FVector(48.f, 33.f, 4.f),  HierroNegro);   // tapa
	C.Caja(FVector(0.f, -15.f, 88.f), FVector(30.f, 2.f, 3.f), HierroNegro);   // ranura
	C.Caja(FVector(0.f, 0.f, 17.f), FVector(10.f, 10.f, 35.f), HierroNegro);   // poste
	C.Caja(FVector(0.f, 0.f, 1.5f), FVector(24.f, 24.f, 3.f),  Fundicion);     // base

	return Guardar(C, TEXT("SM_BuzonCorreos"), MatMetal);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Parada de bus: 12 piezas. Marquesina de 4 postes, techo y banco corrido.
// ─────────────────────────────────────────────────────────────────────────────
bool UCreadorMallaMobiliario::GenerarParadaBus()
{
	FConstructorMalla C;

	// Postes en las cuatro esquinas.
	for (int32 sx = -1; sx <= 1; sx += 2)
	{
		for (int32 sy = -1; sy <= 1; sy += 2)
		{
			C.Caja(FVector(sx * 190.f, sy * 60.f, 115.f), FVector(10.f, 10.f, 230.f), HierroVerde);
		}
	}

	C.Caja(FVector(0.f, 0.f, 236.f), FVector(410.f, 145.f, 12.f), HierroVerde);   // techo
	C.Caja(FVector(0.f, 65.f, 140.f), FVector(390.f, 4.f, 180.f), Piedra);        // trasera (cristal)
	C.Caja(FVector(0.f, 20.f, 45.f), FVector(340.f, 40.f, 5.f), Madera);          // asiento
	for (int32 s = -1; s <= 1; s += 2)
	{
		C.Caja(FVector(s * 150.f, 20.f, 22.f), FVector(8.f, 36.f, 44.f), HierroVerde);
	}

	return Guardar(C, TEXT("SM_ParadaBus"), MatMadera);
}

int32 UCreadorMallaMobiliario::GenerarTodas()
{
	int32 Ok = 0;
	Ok += GenerarBanco()             ? 1 : 0;
	Ok += GenerarPapelera()          ? 1 : 0;
	Ok += GenerarBolardo()           ? 1 : 0;
	Ok += GenerarMaceta()            ? 1 : 0;
	Ok += GenerarBocaIncendio()      ? 1 : 0;
	Ok += GenerarTapaAlcantarilla()  ? 1 : 0;
	Ok += GenerarBuzonCorreos()      ? 1 : 0;
	Ok += GenerarParadaBus()         ? 1 : 0;

	UE_LOG(LogTemp, Log, TEXT("[Mobiliario] %d/8 mallas generadas en %s"), Ok, CarpetaMobiliario);
	return Ok;
}
