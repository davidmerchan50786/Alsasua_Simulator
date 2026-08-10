// ConstructorMallaComun.h (sólo editor)
// Construcción de mallas a base de cajas y prismas, compartida por los
// generadores de mobiliario y de landmarks.
//
// El orden de los vértices de cada cara es antihorario visto desde fuera. Las
// normales van explícitas por instancia, así que la iluminación es correcta en
// cualquier caso; si alguna cara se viera desde dentro, el arreglo es
// intercambiar dos índices en Triangulo().
#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "MaterialEditingLibrary.h"
#include "StaticMeshAttributes.h"
#include "MeshDescription.h"
#include "MeshDescriptionBuilder.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorAssetLibrary.h"
#include "UObject/Package.h"

namespace AlsasuaMalla
{
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

			// Con radio 0 arriba (cono: chapiteles, remates) los dos vértices
			// altos colapsan en el vértice y el cuadrilátero deja un triángulo
			// de área cero por lado. Lo mismo invertido con radio 0 abajo.
			if (RadioArriba <= KINDA_SMALL_NUMBER)
			{
				Triangulo(B0, B1, T0, Normal, Color, UVporCm);
			}
			else if (RadioAbajo <= KINDA_SMALL_NUMBER)
			{
				Triangulo(B0, T1, T0, Normal, Color, UVporCm);
			}
			else
			{
				Cara(B0, B1, T1, T0, Normal, Color, UVporCm);
			}
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
inline bool Guardar(FConstructorMalla& C, const TCHAR* Carpeta, const FString& Nombre, const TCHAR* RutaMaterial)
{
	const FString Ruta = FString(Carpeta) / Nombre;

	if (!UEditorAssetLibrary::DoesDirectoryExist(Carpeta))
	{
		UEditorAssetLibrary::MakeDirectory(Carpeta);
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
		UE_LOG(LogTemp, Warning, TEXT("[Malla] %s sin material (%s): crea los materiales primero."), *Nombre, RutaMaterial);
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

	UE_LOG(LogTemp, Log, TEXT("[Malla] %s creado en %s"), *Nombre, *Ruta);
	return true;
}

}
