// TerrenoLejano.cpp
#include "TerrenoLejano.h"
#include "TerrenoGenerado.h"
#include "CargarMaterialComun.h"
#include "ProceduralMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Materials/MaterialInterface.h"

namespace
{
// El mundo no mide la altura en metros sobre el mar, sino con el datum del
// landscape: worldZ_cm = alt_m*100 - 51133. Sale de la codificación del heightmap
// principal (alt = 495 + q/64, LocZ = 49567, ScaleZ = 200) y cuadra con las tres
// referencias que hay en el repo: lidar_dtm_meta (497,391 m -> -1393,9 cm y
// 1155,328 m -> 64399,8 cm) y PRIMER_COMPILADO_5_8 (plaza 531,94 m -> 2061 cm).
// Sin restarlo, el anillo entero flotaría 511 m por encima del pueblo.
constexpr double CotaBaseCm = 51133.0;
}

ATerrenoLejano::ATerrenoLejano()
{
	PrimaryActorTick.bCanEverTick = false;
	Malla = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("MallaRelieveLejano"));
	RootComponent = Malla;

	// Decorado: ni colisiona ni proyecta sombra. La colisión importa de verdad —
	// varios cargadores muestrean el suelo con LineTrace por ECC_Visibility sin
	// filtrar por actor, y un anillo sólido de 60 km sería lo más alto bajo el
	// trace en cuanto alguien mirase fuera del pueblo.
	Malla->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Malla->SetCastShadow(false);
	Malla->bUseAsyncCooking = true;
}

void ATerrenoLejano::BeginPlay()
{
	Super::BeginPlay();
	// Lo normal es que lo construya ADirectorArranque tras el terreno; si alguien
	// suelta el actor en el mapa a mano, que se construya solo.
	if (Malla && Malla->GetNumSections() == 0) Construir();
}

bool ATerrenoLejano::CargarDatos()
{
	const FString RutaM = FPaths::Combine(FPaths::ProjectContentDir(), RutaMeta);
	FString Texto;
	if (!FFileHelper::LoadFileToString(Texto, *RutaM))
	{
		UE_LOG(LogTemp, Warning, TEXT("TerrenoLejano: no encuentro %s; sin relieve lejano."), *RutaM);
		return false;
	}

	TSharedPtr<FJsonObject> Meta;
	const TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Texto);
	if (!FJsonSerializer::Deserialize(R, Meta) || !Meta.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("TerrenoLejano: %s no es JSON válido."), *RutaM);
		return false;
	}

	Resolucion = Meta->GetIntegerField(TEXT("resolucion"));

	const TSharedPtr<FJsonObject>* Mundo = nullptr;
	if (!Meta->TryGetObjectField(TEXT("mundo_cm"), Mundo) || !Mundo) return false;
	CentroX    = (*Mundo)->GetNumberField(TEXT("centroX"));
	CentroY    = (*Mundo)->GetNumberField(TEXT("centroY"));
	SemiladoCm = (*Mundo)->GetNumberField(TEXT("semilado"));

	const TSharedPtr<FJsonObject>* Cod = nullptr;
	if (Meta->TryGetObjectField(TEXT("codificacion"), Cod) && Cod)
	{
		DatumM = (*Cod)->GetNumberField(TEXT("datum_m"));
		PasoM  = (*Cod)->GetNumberField(TEXT("paso_m"));
	}

	const FString RutaR = FPaths::Combine(FPaths::ProjectContentDir(), RutaRAW);
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *RutaR))
	{
		UE_LOG(LogTemp, Warning, TEXT("TerrenoLejano: no encuentro %s."), *RutaR);
		return false;
	}
	const int32 Esperado = Resolucion * Resolucion * 2;
	if (Bytes.Num() != Esperado)
	{
		UE_LOG(LogTemp, Warning, TEXT("TerrenoLejano: %s mide %d bytes, esperaba %d."),
			*RutaR, Bytes.Num(), Esperado);
		return false;
	}

	Alturas.SetNumUninitialized(Resolucion * Resolucion);
	FMemory::Memcpy(Alturas.GetData(), Bytes.GetData(), Bytes.Num());
	return true;
}

float ATerrenoLejano::AlturaMDT(double MundoX, double MundoY) const
{
	if (Resolucion <= 1) return 0.f;

	const double Rango = 2.0 * SemiladoCm;
	// Fila 0 al sur (así lo escribe DescargarRelieveLejano.py), luego v crece con Y.
	double u = (MundoX - (CentroX - SemiladoCm)) / Rango;
	double v = (MundoY - (CentroY - SemiladoCm)) / Rango;
	u = FMath::Clamp(u, 0.0, 1.0);
	v = FMath::Clamp(v, 0.0, 1.0);

	const double fx = u * (Resolucion - 1);
	const double fy = v * (Resolucion - 1);
	const int32 x0 = FMath::Clamp((int32)fx, 0, Resolucion - 1);
	const int32 y0 = FMath::Clamp((int32)fy, 0, Resolucion - 1);
	const int32 x1 = FMath::Min(x0 + 1, Resolucion - 1);
	const int32 y1 = FMath::Min(y0 + 1, Resolucion - 1);
	const double tx = fx - x0;
	const double ty = fy - y0;

	auto Alt = [&](int32 X, int32 Y) -> double
	{
		return DatumM + (double)Alturas[Y * Resolucion + X] * PasoM;   // metros
	};

	const double a = FMath::Lerp(Alt(x0, y0), Alt(x1, y0), tx);
	const double b = FMath::Lerp(Alt(x0, y1), Alt(x1, y1), tx);
	return (float)(FMath::Lerp(a, b, ty) * 100.0 - CotaBaseCm);        // cm de mundo
}

int32 ATerrenoLejano::Construir()
{
	if (!CargarDatos()) return 0;

	UWorld* World = GetWorld();
	if (!World) return 0;

	// El agujero tiene que ser exactamente el terreno jugable. Se lo preguntamos a
	// él en vez de repetir la fórmula, que es lo que se desalinea con el tiempo.
	ATerrenoGenerado* Cerca = nullptr;
	for (TActorIterator<ATerrenoGenerado> It(World); It; ++It) { Cerca = *It; break; }

	double HuecoCm = 360000.0;   // por si aún no hay terreno: (4033-1)*178.5714/2
	if (Cerca)
	{
		HuecoCm = Cerca->MitadMundo();
		const FVector C = Cerca->CentroMundo();
		CentroX = C.X;
		CentroY = C.Y;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("TerrenoLejano: sin ATerrenoGenerado; uso el hueco por defecto."));
	}

	const double CeldaCm = FMath::Max(1.f, CeldaM) * 100.0;
	const int32 NCeldas = FMath::Max(2, FMath::RoundToInt((2.0 * SemiladoCm) / CeldaCm));
	const int32 NVerts = NCeldas + 1;
	const double X0 = CentroX - SemiladoCm;
	const double Y0 = CentroY - SemiladoCm;

	// Índice de celda donde empieza y acaba el agujero. Redondeamos hacia fuera para
	// no dejar una franja de anillo por debajo del terreno jugable.
	const int32 HuecoIni = FMath::FloorToInt((SemiladoCm - HuecoCm) / CeldaCm);
	const int32 HuecoFin = NCeldas - HuecoIni;   // simétrico

	const float BandaCm = FMath::Max(1.f, BandaFusionM) * 100.f;

	TArray<FVector> Verts;      Verts.SetNumUninitialized(NVerts * NVerts);
	TArray<FVector> Normales;   Normales.Init(FVector::UpVector, NVerts * NVerts);
	TArray<FVector2D> UVs;      UVs.SetNumUninitialized(NVerts * NVerts);

	const FVector Origen = GetActorLocation();

	for (int32 j = 0; j < NVerts; ++j)
	{
		for (int32 i = 0; i < NVerts; ++i)
		{
			const double WX = X0 + i * CeldaCm;
			const double WY = Y0 + j * CeldaCm;

			double Z = AlturaMDT(WX, WY);

			// Cerca del borde del terreno jugable, fundimos hacia la altura que
			// tiene ESE terreno en el punto del borde más cercano (proyección
			// radial en Chebyshev, que es la forma del cuadrado). Así en el borde
			// exacto las dos superficies valen lo mismo y no queda escalón.
			const double dx = WX - CentroX;
			const double dy = WY - CentroY;
			const double Cheb = FMath::Max(FMath::Abs(dx), FMath::Abs(dy));
			if (Cerca && Cheb > KINDA_SMALL_NUMBER && Cheb < HuecoCm + BandaCm)
			{
				const double k = HuecoCm / Cheb;               // lleva el punto al borde
				const float ZBorde = Cerca->AlturaEnMundo((float)(CentroX + dx * k),
				                                          (float)(CentroY + dy * k));
				const double t = FMath::Clamp((Cheb - HuecoCm) / (double)BandaCm, 0.0, 1.0);
				Z = FMath::Lerp((double)ZBorde, Z, t);
			}

			const int32 Idx = j * NVerts + i;
			Verts[Idx] = FVector(WX, WY, Z) - Origen;
			UVs[Idx] = FVector2D((double)i / NCeldas, (double)j / NCeldas);
		}
	}

	TArray<int32> Tris;
	Tris.Reserve(NCeldas * NCeldas * 6);
	for (int32 j = 0; j < NCeldas; ++j)
	{
		for (int32 i = 0; i < NCeldas; ++i)
		{
			// Agujero: fuera las celdas que caen dentro del terreno jugable.
			if (i >= HuecoIni && i < HuecoFin && j >= HuecoIni && j < HuecoFin) continue;

			const int32 v00 = j * NVerts + i;
			const int32 v10 = v00 + 1;
			const int32 v01 = v00 + NVerts;
			const int32 v11 = v01 + 1;
			Tris.Add(v00); Tris.Add(v01); Tris.Add(v11);
			Tris.Add(v00); Tris.Add(v11); Tris.Add(v10);
		}
	}

	// Normales por diferencias centradas sobre la malla ya construida.
	for (int32 j = 0; j < NVerts; ++j)
	{
		for (int32 i = 0; i < NVerts; ++i)
		{
			const int32 i0 = FMath::Max(i - 1, 0), i1 = FMath::Min(i + 1, NVerts - 1);
			const int32 j0 = FMath::Max(j - 1, 0), j1 = FMath::Min(j + 1, NVerts - 1);
			const double dzdx = Verts[j * NVerts + i1].Z - Verts[j * NVerts + i0].Z;
			const double dzdy = Verts[j1 * NVerts + i].Z - Verts[j0 * NVerts + i].Z;
			const double ex = (i1 - i0) * CeldaCm;
			const double ey = (j1 - j0) * CeldaCm;
			Normales[j * NVerts + i] =
				FVector(-dzdx * ey, -dzdy * ex, ex * ey).GetSafeNormal();
		}
	}

	Malla->ClearAllMeshSections();
	// Una sola sección: es un draw call para los 60 km. Ver la regla de draw calls
	// del RESUMEN_TECNICO — aquí es fácil respetarla y no hay motivo para trocear.
	Malla->CreateMeshSection(0, Verts, Tris, Normales, UVs,
		TArray<FColor>(), TArray<FProcMeshTangent>(), /*bCreateCollision*/ false);

	if (UMaterialInterface* Mat = CargarMaterialConFallbackSeguro(
			TEXT("/Game/Materiales/M_Relieve_Lejano.M_Relieve_Lejano"),
			TEXT("/Game/Materiales/M_Relieve_Lejano.M_Relieve_Lejano"),
			TEXT("/Game/Materiales/M_Terreno_Orto.M_Terreno_Orto")))
	{
		Malla->SetMaterial(0, Mat);
	}

	const int32 NumTris = Tris.Num() / 3;
	UE_LOG(LogTemp, Log, TEXT("TerrenoLejano: anillo de %.0f km con hueco de %.1f km, %d tris, 1 seccion."),
		(2.0 * SemiladoCm) / 100000.0, (2.0 * HuecoCm) / 100000.0, NumTris);
	return NumTris;
}
