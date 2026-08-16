#include "TunelAlsasua.h"
#include "GeoDataAlsasua.h"
#include "CargarJsonComun.h"
#include "CargarMaterialComun.h"
#include "ProceduralMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"

ATunelAlsasua::ATunelAlsasua()
{
	PrimaryActorTick.bCanEverTick = false;
	Malla = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Malla"));
	SetRootComponent(Malla);
	// Sin tocar la movilidad, como ACalleGenerada y APoligonoSuelo: la sección se
	// crea en runtime desde el director y forzarla a Static aquí va a contrapelo
	// de eso.
}

void ATunelAlsasua::AnadirBoca(const FVector& CentroCm, const FVector2D& DirXY, float AnchoCm,
                               TArray<FVector>& V, TArray<int32>& T, TArray<FVector>& N,
                               TArray<FVector2D>& UV, TArray<FColor>& C)
{
	// Ejes locales de la boca: U cruza el vano, la normal mira hacia fuera.
	const FVector2D D = DirXY.GetSafeNormal();
	const FVector U(-D.Y, D.X, 0.0);          // perpendicular en planta
	const FVector NormalFuera(-D.X, -D.Y, 0.0);   // la boca mira hacia donde se entra

	const float SemiVano = AnchoCm * 0.5f;
	const float SemiExt  = SemiVano + MarcoCm;
	// Arranque del arco: por debajo el hastial es recto, como en los túneles de
	// la Sakana, y de ahí para arriba medio punto. El mínimo no es cosmético: el
	// de la A-10 mide 14 m de ancho, o sea 7 m de semivano, y restarlo de los
	// 6,2 m de altura libre por defecto daba un arranque NEGATIVO — el hastial
	// se metía bajo tierra y la boca salía del revés. Con el clamp, la clave
	// queda en AlturaRecta + SemiVano, que para ese túnel son 8,5 m: lo que mide
	// una boca de autovía de dos carriles.
	const float AlturaRecta = FMath::Max(150.f, AlturaLibreCm - SemiVano);
	const FColor Hormigon(150, 148, 143, 255);

	// Un anillo de puntos: para cada ángulo del contorno, el del vano y el del
	// marco. El marco es ese contorno escalado hacia fuera.
	TArray<FVector> Vano, Exterior;
	auto Contorno = [&](float Semi, float Recta, TArray<FVector>& Out)
	{
		Out.Reset();
		Out.Add(CentroCm - U * Semi);                         // pie izquierdo
		Out.Add(CentroCm - U * Semi + FVector(0, 0, Recta));  // arranque izquierdo
		for (int32 i = 1; i < SegmentosArco; ++i)
		{
			const float A = PI * (float)i / (float)SegmentosArco;   // 0..PI
			Out.Add(CentroCm - U * (Semi * FMath::Cos(A))
			        + FVector(0, 0, Recta + Semi * FMath::Sin(A)));
		}
		Out.Add(CentroCm + U * Semi + FVector(0, 0, Recta));  // arranque derecho
		Out.Add(CentroCm + U * Semi);                         // pie derecho
	};
	Contorno(SemiVano, AlturaRecta, Vano);
	Contorno(SemiExt,  AlturaRecta + MarcoCm, Exterior);

	// Cinta entre los dos contornos: eso es el frente del marco, con el vano
	// vacío en medio. Sin tapar el hueco, que es justo lo que se ve del túnel.
	//
	// Se emite por las dos caras, con su propio juego de vértices y su normal
	// invertida. No es por no saber de qué lado quedaba el frente —que también:
	// aquí no hay forma de comprobar el sentido de giro sin compilar y mirarlo—
	// sino porque un marco de boca se ve desde fuera al acercarte y desde dentro
	// al asomarte, y una sola cara desaparecería por culling en uno de los dos.
	// Son veinte triángulos más por boca.
	for (int32 Cara = 0; Cara < 2; ++Cara)
	{
		const FVector Nrm = (Cara == 0) ? NormalFuera : -NormalFuera;
		const int32 Base = V.Num();

		for (int32 i = 0; i < Vano.Num(); ++i)
		{
			const float U0 = (float)i / (float)FMath::Max(1, Vano.Num() - 1);
			V.Add(Vano[i]);      N.Add(Nrm);  C.Add(Hormigon);  UV.Add(FVector2D(U0, 0.f));
			V.Add(Exterior[i]);  N.Add(Nrm);  C.Add(Hormigon);  UV.Add(FVector2D(U0, 1.f));
		}
		for (int32 i = 0; i + 1 < Vano.Num(); ++i)
		{
			const int32 a = Base + i * 2, b = a + 1, c = a + 2, d = a + 3;
			if (Cara == 0) { T.Add(a); T.Add(c); T.Add(b);  T.Add(b); T.Add(c); T.Add(d); }
			else           { T.Add(a); T.Add(b); T.Add(c);  T.Add(b); T.Add(d); T.Add(c); }
		}
	}
}

int32 ATunelAlsasua::Construir()
{
	UWorld* W = GetWorld();
	if (!W) return 0;

	TArray<TSharedPtr<FJsonValue>> Arr;
	if (!JsonDatos::CargarArray(TEXT("Datos/tunnels_unity.json"), Arr, { TEXT("tunnels") }))
		return 0;

	TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
	TArray<FVector2D> UV; TArray<FColor> C;
	int32 Bocas = 0;

	for (const TSharedPtr<FJsonValue>& Val : Arr)
	{
		const TSharedPtr<FJsonObject> O = Val->AsObject();
		if (!O.IsValid()) continue;

		const TArray<TSharedPtr<FJsonValue>>* Pts = nullptr;
		if (!O->TryGetArrayField(TEXT("pts"), Pts) || !Pts || Pts->Num() < 6) continue;

		// pts plano [x,y,z,...] en local ABSOLUTO, con la vertical en medio. // ejes ok
		TArray<FVector2D> XY;
		for (int32 i = 0; i + 2 < Pts->Num(); i += 3)
		{
			const FVector M = UAlsasuaGeoData::UnityaUnreal(
				FVector((*Pts)[i]->AsNumber(), 0.0, (*Pts)[i + 2]->AsNumber()));
			XY.Add(FVector2D(M.X, M.Y));
		}
		if (XY.Num() < 2) continue;

		const double AnchoM = O->HasField(TEXT("width")) ? O->GetNumberField(TEXT("width")) : 6.0;
		const float AnchoCm = (float)(AnchoM * 100.0);

		// El trazado se sale del túnel por los dos lados: length_m es lo que mide
		// la galería y la polilínea llega más lejos. Las bocas van a media
		// longitud declarada del centro del trazado, no en sus extremos.
		double Largo = 0.0;
		TArray<double> Acum; Acum.Add(0.0);
		for (int32 i = 1; i < XY.Num(); ++i)
		{
			Largo += FVector2D::Distance(XY[i - 1], XY[i]);
			Acum.Add(Largo);
		}
		const double LargoTunelCm = O->HasField(TEXT("length_m"))
			? O->GetNumberField(TEXT("length_m")) * 100.0 : Largo;
		const double Mitad = FMath::Min(LargoTunelCm, Largo) * 0.5;

		auto EnDistancia = [&](double S, FVector2D& OutXY, FVector2D& OutDir)
		{
			S = FMath::Clamp(S, 0.0, Largo);
			int32 i = 1;
			while (i < Acum.Num() - 1 && Acum[i] < S) ++i;
			const double Tramo = Acum[i] - Acum[i - 1];
			const double f = Tramo > 1e-3 ? (S - Acum[i - 1]) / Tramo : 0.0;
			OutDir = (XY[i] - XY[i - 1]).GetSafeNormal();
			OutXY = XY[i - 1] + (XY[i] - XY[i - 1]) * f;
		};

		const double Centro = Largo * 0.5;
		for (int32 Lado = 0; Lado < 2; ++Lado)
		{
			FVector2D P, Dir;
			EnDistancia(Centro + (Lado == 0 ? -Mitad : Mitad), P, Dir);
			if (Lado == 0) Dir = -Dir;   // cada boca mira hacia su lado

			const float Suelo = UAlsasuaGeoData::AlturaSueloUE5(W, P.X, P.Y);
			AnadirBoca(FVector(P.X, P.Y, Suelo), Dir, AnchoCm, V, T, N, UV, C);
			++Bocas;
		}
	}

	if (Bocas == 0) return 0;

	// Una sola sección para las diez bocas: un draw call. La regla 1 del
	// RESUMEN_TECNICO vale igual aunque sean diez piezas y no sesenta mil.
	TArray<FProcMeshTangent> Tan;
	Malla->CreateMeshSection(0, V, T, N, UV, C, Tan, /*bCreateCollision=*/true);

	if (UMaterialInterface* Mat = CargarMaterialConFallbackSeguro(
			TEXT("/Game/Materiales/M_Hormigon.M_Hormigon"),
			TEXT("/Game/Materiales/M_Hormigon.M_Hormigon"),
			TEXT("/Game/Materiales/M_Edificio.M_Edificio")))
	{
		Malla->SetMaterial(0, Mat);
	}

	UE_LOG(LogTemp, Log, TEXT("[Tunel] %d bocas de túnel en una sección."), Bocas);
	return Bocas;
}
