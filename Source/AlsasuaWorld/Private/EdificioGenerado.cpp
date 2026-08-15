// EdificioGenerado.cpp
#include "EdificioGenerado.h"
#include "ProceduralMeshComponent.h"

AEdificioGenerado::AEdificioGenerado()
{
	PrimaryActorTick.bCanEverTick = false;
	Malla = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Malla"));
	RootComponent = Malla;
	Malla->bUseAsyncCooking = true;
	Malla->SetCollisionProfileName(TEXT("BlockAll"));
	Tags.Add(TEXT("Edificio"));   // lo recoge el StreamerMundoEstatico
}

// --- Ear clipping para tapas (polígono simple, devuelve índices de triángulos) ---
static double Area2Local(const TArray<FVector2D>& P)
{
	double A = 0.0;
	for (int32 i = 0, n = P.Num(); i < n; ++i)
	{
		const FVector2D& a = P[i]; const FVector2D& b = P[(i + 1) % n];
		A += (a.X * b.Y - b.X * a.Y);
	}
	return A * 0.5;
}

static bool DentroTri(const FVector2D& a, const FVector2D& b, const FVector2D& c, const FVector2D& p)
{
	const double d1 = (p.X - b.X) * (a.Y - b.Y) - (a.X - b.X) * (p.Y - b.Y);
	const double d2 = (p.X - c.X) * (b.Y - c.Y) - (b.X - c.X) * (p.Y - c.Y);
	const double d3 = (p.X - a.X) * (c.Y - a.Y) - (c.X - a.X) * (p.Y - a.Y);
	const bool neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
	const bool pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
	return !(neg && pos);
}

static void Triangular(const TArray<FVector2D>& Poly, TArray<int32>& OutTris)
{
	const int32 N = Poly.Num();
	if (N < 3) return;
	TArray<int32> V; V.Reserve(N);
	const bool CCW = Area2Local(Poly) > 0.0;
	for (int32 i = 0; i < N; ++i) V.Add(CCW ? i : (N - 1 - i));   // trabajar en CCW

	int32 guard = 0;
	while (V.Num() > 2 && guard++ < N * N)
	{
		bool corto = false;
		const int32 m = V.Num();
		for (int32 i = 0; i < m; ++i)
		{
			const int32 i0 = V[(i + m - 1) % m], i1 = V[i], i2 = V[(i + 1) % m];
			const FVector2D& a = Poly[i0]; const FVector2D& b = Poly[i1]; const FVector2D& c = Poly[i2];
			// vértice convexo (CCW)
			if (((b.X - a.X) * (c.Y - a.Y) - (b.Y - a.Y) * (c.X - a.X)) <= 0.0) continue;
			// ningún otro vértice dentro
			bool ok = true;
			for (int32 j = 0; j < m; ++j)
			{
				const int32 vj = V[j];
				if (vj == i0 || vj == i1 || vj == i2) continue;
				if (DentroTri(a, b, c, Poly[vj])) { ok = false; break; }
			}
			if (!ok) continue;
			OutTris.Add(i0); OutTris.Add(i1); OutTris.Add(i2);
			V.RemoveAt(i);
			corto = true;
			break;
		}
		if (!corto) break;   // polígono degenerado; salir
	}
}

FMetricasTejado CalcularMetricasTejado(const TArray<FVector2D>& Poligono, float AlturaCm,
                                       EFormaTejado Forma, float EscalaTejado)
{
	FMetricasTejado M;
	if (Poligono.Num() < 3 || AlturaCm <= 0.f) return M;

	FVector2D mn = Poligono[0], mx = Poligono[0];
	for (const FVector2D& V : Poligono)
	{
		mn.X = FMath::Min(mn.X, V.X); mn.Y = FMath::Min(mn.Y, V.Y);
		mx.X = FMath::Max(mx.X, V.X); mx.Y = FMath::Max(mx.Y, V.Y);
	}

	M.RoofH = (Forma == EFormaTejado::Plano) ? 0.f
		: FMath::Clamp(FMath::Min(mx.X - mn.X, mx.Y - mn.Y) * 0.22f * EscalaTejado, 120.f, 700.f);
	// El tejado no se come más de medio edificio.
	M.RoofH = FMath::Min(M.RoofH, AlturaCm * 0.5f);
	M.Alero = AlturaCm - M.RoofH;
	return M;
}

void AEdificioGenerado::Construir(const TArray<FVector2D>& In, float AlturaCm, FColor ColorMuro, FColor ColorTejado,
                                  EFormaTejado Forma, FVector2D EjeCaballete, float EscalaTejado)
{
	// quitar vértice de cierre duplicado si existe
	TArray<FVector2D> P = In;
	if (P.Num() > 1 && P[0].Equals(P.Last(), 1.f)) P.Pop();
	const int32 N = P.Num();
	if (N < 3) return;

	const FColor ColorSuelo(ColorMuro.R / 3, ColorMuro.G / 3, ColorMuro.B / 3, 255);  // zócalo oscuro

	// Altura total (LIDAR) = hasta el caballete. Repartimos en muro (alero) + tejado,
	// para que el remate quede en la cota real y no se sume por encima.
	const FMetricasTejado Met = CalcularMetricasTejado(P, AlturaCm, Forma, EscalaTejado);
	const float RoofH = Met.RoofH;
	const float Alero = Met.Alero;
	Tejado = FTejadoConstruido{ P, AlturaCm, Forma, EjeCaballete, EscalaTejado };

	TArray<FVector> Verts;
	TArray<int32>  Tris;
	TArray<FVector> Normales;
	TArray<FVector2D> UVs;
	TArray<FColor> Colores;

	// --- Muros (suben hasta el alero) ---
	for (int32 i = 0; i < N; ++i)
	{
		const FVector2D& A = P[i];
		const FVector2D& B = P[(i + 1) % N];
		const int32 base = Verts.Num();
		Verts.Add(FVector(A.X, A.Y, 0));
		Verts.Add(FVector(B.X, B.Y, 0));
		Verts.Add(FVector(B.X, B.Y, Alero));
		Verts.Add(FVector(A.X, A.Y, Alero));

		FVector borde(B.X - A.X, B.Y - A.Y, 0);
		FVector nrm = FVector::CrossProduct(borde, FVector::UpVector).GetSafeNormal();
		for (int32 k = 0; k < 4; ++k) Normales.Add(nrm);
		const float largo = borde.Size() / 100.f;   // UV en metros
		const float alto  = Alero / 100.f;
		UVs.Add({0,alto}); UVs.Add({largo,alto}); UVs.Add({largo,0}); UVs.Add({0,0});
		for (int32 k = 0; k < 4; ++k) Colores.Add(ColorMuro);

		Tris.Add(base + 0); Tris.Add(base + 2); Tris.Add(base + 1);
		Tris.Add(base + 0); Tris.Add(base + 3); Tris.Add(base + 2);
	}

	TArray<int32> CapTris; Triangular(P, CapTris);   // (para la tapa de suelo)

	// Suelo (sección 0, mira hacia abajo)
	const int32 baseSuelo = Verts.Num();
	for (int32 i = 0; i < N; ++i)
	{
		Verts.Add(FVector(P[i].X, P[i].Y, 0));
		Normales.Add(-FVector::UpVector);
		UVs.Add(P[i] / 100.f);
		Colores.Add(ColorSuelo);
	}
	for (int32 t = 0; t < CapTris.Num(); t += 3)   // winding invertido (mira abajo)
	{ Tris.Add(baseSuelo + CapTris[t]); Tris.Add(baseSuelo + CapTris[t + 2]); Tris.Add(baseSuelo + CapTris[t + 1]); }

	// --- Tejado: SECCIÓN 1 aparte (material de ortofoto), remata en AlturaCm ---
	TArray<FVector> RV; TArray<int32> RT; TArray<FVector> RN; TArray<FVector2D> RUV; TArray<FColor> RC;
	if (Forma == EFormaTejado::Plano)
	{
		for (int32 i = 0; i < N; ++i) { RV.Add(FVector(P[i].X, P[i].Y, Alero)); RN.Add(FVector::UpVector); RUV.Add(P[i] / 100.f); RC.Add(ColorTejado); }
		for (int32 t = 0; t < CapTris.Num(); t += 3) { RT.Add(CapTris[t]); RT.Add(CapTris[t + 1]); RT.Add(CapTris[t + 2]); }
	}
	else if (Forma == EFormaTejado::Dos_Aguas)
		TejadoDosAguas(P, Alero, RoofH, EjeCaballete, ColorTejado, RV, RT, RN, RUV, RC);
	else
		TejadoCuatroAguas(P, Alero, RoofH, ColorTejado, RV, RT, RN, RUV, RC);

	TArray<FProcMeshTangent> Tangentes;
	Malla->CreateMeshSection(0, Verts, Tris, Normales, UVs, Colores, Tangentes, true);   // muros + suelo
	Malla->CreateMeshSection(1, RV, RT, RN, RUV, RC, Tangentes, true);                   // tejado
}

// ── ConstruirConDetalle: versión con ventanas, puerta, cornisa, zócalo ─────
void AEdificioGenerado::ConstruirConDetalle(const TArray<FVector2D>& In, float AlturaCm,
                                            const FFachadaConfig& Config, FColor ColorTejado,
                                            EFormaTejado Forma, FVector2D EjeCaballete, float EscalaTejado)
{
	TArray<FVector2D> P = In;
	if (P.Num() > 1 && P[0].Equals(P.Last(), 1.f)) P.Pop();
	const int32 N = P.Num();
	if (N < 3) return;

	const FMetricasTejado Met = CalcularMetricasTejado(P, AlturaCm, Forma, EscalaTejado);
	const float RoofH = Met.RoofH;
	const float Alero = Met.Alero;
	Tejado = FTejadoConstruido{ P, AlturaCm, Forma, EjeCaballete, EscalaTejado };

	TArray<FVector> Verts;
	TArray<int32>  Tris;
	TArray<FVector> Normales;
	TArray<FVector2D> UVs;
	TArray<FColor> Colores;

	// Muros con detalle (ventanas, puerta, cornisa, zócalo).
	GenerarFachadaConDetalle(P, Alero, Config, Verts, Tris, Normales, UVs, Colores);

	// Suelo.
	TArray<int32> CapTris; Triangular(P, CapTris);
	const int32 baseSuelo = Verts.Num();
	for (int32 i = 0; i < N; ++i)
	{
		Verts.Add(FVector(P[i].X, P[i].Y, 0));
		Normales.Add(-FVector::UpVector);
		UVs.Add(P[i] / 100.f);
		Colores.Add(Config.ColorZocalo);
	}
	for (int32 t = 0; t < CapTris.Num(); t += 3)
	{ Tris.Add(baseSuelo + CapTris[t]); Tris.Add(baseSuelo + CapTris[t + 2]); Tris.Add(baseSuelo + CapTris[t + 1]); }

	// Tejado.
	TArray<FVector> RV; TArray<int32> RT; TArray<FVector> RN; TArray<FVector2D> RUV; TArray<FColor> RC;
	if (Forma == EFormaTejado::Plano)
	{
		for (int32 i = 0; i < N; ++i) { RV.Add(FVector(P[i].X, P[i].Y, Alero)); RN.Add(FVector::UpVector); RUV.Add(P[i] / 100.f); RC.Add(ColorTejado); }
		for (int32 t = 0; t < CapTris.Num(); t += 3) { RT.Add(CapTris[t]); RT.Add(CapTris[t + 1]); RT.Add(CapTris[t + 2]); }
	}
	else if (Forma == EFormaTejado::Dos_Aguas)
		TejadoDosAguas(P, Alero, RoofH, EjeCaballete, ColorTejado, RV, RT, RN, RUV, RC);
	else
		TejadoCuatroAguas(P, Alero, RoofH, ColorTejado, RV, RT, RN, RUV, RC);

	TArray<FProcMeshTangent> Tangentes;
	Malla->CreateMeshSection(0, Verts, Tris, Normales, UVs, Colores, Tangentes, true);
	Malla->CreateMeshSection(1, RV, RT, RN, RUV, RC, Tangentes, true);
}

// ── GenerarFachadaConDetalle: muros con ventanas, puerta, cornisa, zócalo ──
void AEdificioGenerado::GenerarFachadaConDetalle(const TArray<FVector2D>& P, float Alero,
                                                  const FFachadaConfig& Config,
                                                  TArray<FVector>& V, TArray<int32>& T,
                                                  TArray<FVector>& N, TArray<FVector2D>& UV, TArray<FColor>& C)
{
	const int32 n = P.Num();
	const float ZCaloH = FMath::Min(60.f, Alero * 0.1f);     // zócalo: 60cm o 10% del muro
	const float CornisaZ = Alero - Config.AltoCornisa;         // inicio de la cornisa

	for (int32 i = 0; i < n; ++i)
	{
		const FVector2D& A = P[i];
		const FVector2D& B = P[(i + 1) % n];
		const float Lado = FVector2D(B - A).Size();
		const FVector Dir3D(B.X - A.X, B.Y - A.Y, 0);
		const FVector Nrm = FVector::CrossProduct(Dir3D, FVector::UpVector).GetSafeNormal();
		const float LadoM = Lado / 100.f;

		// ── Zócalo (banda inferior oscura) ──
		{
			const int32 b = V.Num();
			V.Add(FVector(A.X, A.Y, 0)); V.Add(FVector(B.X, B.Y, 0));
			V.Add(FVector(B.X, B.Y, ZCaloH)); V.Add(FVector(A.X, A.Y, ZCaloH));
			for (int32 k = 0; k < 4; ++k) { N.Add(Nrm); C.Add(Config.ColorZocalo); }
			UV.Add({0,0}); UV.Add({LadoM,0}); UV.Add({LadoM, ZCaloH/100.f}); UV.Add({0, ZCaloH/100.f});
			T.Add(b); T.Add(b+2); T.Add(b+1); T.Add(b); T.Add(b+3); T.Add(b+2);
		}

		// ── Muro principal (zócalo → cornisa) con ventanas ──
		const float MuroH = CornisaZ - ZCaloH;
		if (MuroH > 50.f)
		{
			const float MuroHm = MuroH / 100.f;
			const float Z0 = ZCaloH;

			// ¿Poner puerta en este lado? Solo en el lado más largo.
			float MaxLado = 0;
			for (int32 j = 0; j < n; ++j)
			{
				const float L = FVector2D(P[(j+1)%n] - P[j]).Size();
				MaxLado = FMath::Max(MaxLado, L);
			}
			const bool bPuedePuerta = Config.bPonerPuerta && (Lado >= MaxLado * 0.95f);

			if (bPuedePuerta)
			{
				// Puerta centrada.
				const float PuertaW = Config.AnchoPuerta / 100.f;
				const float PuertaH = FMath::Min(Config.AltoPuerta, MuroH * 0.85f) / 100.f;
				const float CentroU = LadoM * 0.5f;
				const float PuertaX0 = CentroU - PuertaW * 0.5f;
				const float PuertaX1 = CentroU + PuertaW * 0.5f;
				const float Prof = Config.ProfundidadRebaje / 100.f;

				// Muro izquierdo de la puerta.
				if (PuertaX0 > 0.01f)
				{
					const int32 b = V.Num();
					V.Add(FVector(A.X, A.Y, Z0)); V.Add(FVector(B.X, B.Y, Z0));
					V.Add(FVector(B.X, B.Y, Z0 + MuroH)); V.Add(FVector(A.X, A.Y, Z0 + MuroH));
					for (int32 k = 0; k < 4; ++k) { N.Add(Nrm); C.Add(Config.ColorMuro); }
					UV.Add({0,0}); UV.Add({PuertaX0,0}); UV.Add({PuertaX0, MuroHm}); UV.Add({0, MuroHm});
					T.Add(b); T.Add(b+2); T.Add(b+1); T.Add(b); T.Add(b+3); T.Add(b+2);
				}

				// Muro derecho de la puerta.
				if (PuertaX1 < LadoM - 0.01f)
				{
					const int32 b = V.Num();
					V.Add(FVector(A.X, A.Y, Z0)); V.Add(FVector(B.X, B.Y, Z0));
					V.Add(FVector(B.X, B.Y, Z0 + MuroH)); V.Add(FVector(A.X, A.Y, Z0 + MuroH));
					for (int32 k = 0; k < 4; ++k) { N.Add(Nrm); C.Add(Config.ColorMuro); }
					UV.Add({PuertaX1,0}); UV.Add({LadoM,0}); UV.Add({LadoM, MuroHm}); UV.Add({PuertaX1, MuroHm});
					T.Add(b); T.Add(b+2); T.Add(b+1); T.Add(b); T.Add(b+3); T.Add(b+2);
				}

				// Rejte de puerta (fondo oscuro).
				{
					const int32 b = V.Num();
					V.Add(FVector(A.X, A.Y, Z0)); V.Add(FVector(B.X, B.Y, Z0));
					V.Add(FVector(B.X, B.Y, Z0 + PuertaH)); V.Add(FVector(A.X, A.Y, Z0 + PuertaH));
					for (int32 k = 0; k < 4; ++k) { N.Add(Nrm); C.Add(Config.ColorPuerta); }
					UV.Add({PuertaX0,0}); UV.Add({PuertaX1,0}); UV.Add({PuertaX1, PuertaH}); UV.Add({PuertaX0, PuertaH});
					T.Add(b); T.Add(b+2); T.Add(b+1); T.Add(b); T.Add(b+3); T.Add(b+2);
				}
			}
			else
			{
				// Muro sólido sin puerta.
				const int32 b = V.Num();
				V.Add(FVector(A.X, A.Y, Z0)); V.Add(FVector(B.X, B.Y, Z0));
				V.Add(FVector(B.X, B.Y, Z0 + MuroH)); V.Add(FVector(A.X, A.Y, Z0 + MuroH));
				for (int32 k = 0; k < 4; ++k) { N.Add(Nrm); C.Add(Config.ColorMuro); }
				UV.Add({0,0}); UV.Add({LadoM,0}); UV.Add({LadoM, MuroHm}); UV.Add({0, MuroHm});
				T.Add(b); T.Add(b+2); T.Add(b+1); T.Add(b); T.Add(b+3); T.Add(b+2);
			}

			// ── Ventanas (solo si hay espacio suficiente) ──
			const float VentW = Config.AnchoVentana / 100.f;
			const float VentH = Config.AltoVentana / 100.f;
			const float EspX = Config.EspaciadoX / 100.f;
			const float EspY = Config.EspaciadoY / 100.f;
			const float Prof = Config.ProfundidadRebaje / 100.f;

			if (EspX > VentW + 0.5f && EspY > VentH + 0.5f && LadoM > VentW + 1.f)
			{
				const int32 NumCols = FMath::Max(1, FMath::FloorToInt((LadoM - VentW) / EspX) + 1);
				const int32 NumFilas = FMath::Max(1, FMath::FloorToInt((MuroH - Config.OffsetPrimerPiso - VentH) / EspY) + 1);
				const float StartX = (LadoM - (NumCols - 1) * EspX) * 0.5f;

				for (int32 row = 0; row < NumFilas; ++row)
				{
					for (int32 col = 0; col < NumCols; ++col)
					{
						const float WX0 = StartX + col * EspX - VentW * 0.5f;
						const float WX1 = WX0 + VentW;
						const float WZ0 = Z0 + Config.OffsetPrimerPiso + row * EspY;
						const float WZ1 = WZ0 + VentH;

						if (WX0 < 0.f || WX1 > LadoM || WZ1 > Z0 + MuroH) continue;

						// Ventana: rectángulo oscuro (vidrio).
						const int32 b = V.Num();
						V.Add(FVector(A.X, A.Y, Z0)); V.Add(FVector(B.X, B.Y, Z0));
						V.Add(FVector(B.X, B.Y, Z0 + MuroH)); V.Add(FVector(A.X, A.Y, Z0 + MuroH));
						for (int32 k = 0; k < 4; ++k) { N.Add(Nrm); C.Add(Config.ColorVentana); }
						UV.Add({WX0, WZ0 - Z0}); UV.Add({WX1, WZ0 - Z0});
						UV.Add({WX1, WZ1 - Z0}); UV.Add({WX0, WZ1 - Z0});
						T.Add(b); T.Add(b+2); T.Add(b+1); T.Add(b); T.Add(b+3); T.Add(b+2);
					}
				}
			}
		}

		// ── Cornisa (banda decorativa bajo el alero) ──
		{
			const int32 b = V.Num();
			V.Add(FVector(A.X, A.Y, CornisaZ)); V.Add(FVector(B.X, B.Y, CornisaZ));
			V.Add(FVector(B.X, B.Y, Alero)); V.Add(FVector(A.X, A.Y, Alero));
			for (int32 k = 0; k < 4; ++k) { N.Add(Nrm); C.Add(Config.ColorCornisa); }
			UV.Add({0,0}); UV.Add({LadoM,0}); UV.Add({LadoM, Config.AltoCornisa/100.f}); UV.Add({0, Config.AltoCornisa/100.f});
			T.Add(b); T.Add(b+2); T.Add(b+1); T.Add(b); T.Add(b+3); T.Add(b+2);
		}
	}
}

// Tejado a cuatro aguas (hip): faldones desde el alero a un vértice central.
void AEdificioGenerado::TejadoCuatroAguas(const TArray<FVector2D>& P, float AlturaCm, float RoofH, FColor Color,
	TArray<FVector>& V, TArray<int32>& T, TArray<FVector>& N, TArray<FVector2D>& UV, TArray<FColor>& C)
{
	const int32 n = P.Num();
	FVector2D mn = P[0], mx = P[0];
	for (const FVector2D& v : P) { mn.X = FMath::Min(mn.X, v.X); mn.Y = FMath::Min(mn.Y, v.Y); mx.X = FMath::Max(mx.X, v.X); mx.Y = FMath::Max(mx.Y, v.Y); }
	const FVector2D Centro = (mn + mx) * 0.5f;
	const FVector Apex(Centro.X, Centro.Y, AlturaCm + RoofH);

	for (int32 i = 0; i < n; ++i)
	{
		const FVector A(P[i].X, P[i].Y, AlturaCm);
		const FVector B(P[(i + 1) % n].X, P[(i + 1) % n].Y, AlturaCm);
		FVector nrm = FVector::CrossProduct(B - A, Apex - A).GetSafeNormal();
		const bool flip = nrm.Z < 0.f; if (flip) nrm = -nrm;
		const int32 b = V.Num();
		V.Add(A); V.Add(B); V.Add(Apex);
		for (int32 k = 0; k < 3; ++k) { N.Add(nrm); C.Add(Color); }
		UV.Add({0,0}); UV.Add({1,0}); UV.Add({0.5f,1});
		if (flip) { T.Add(b); T.Add(b + 2); T.Add(b + 1); } else { T.Add(b); T.Add(b + 1); T.Add(b + 2); }
	}
}

// Tejado a dos aguas: caballete a lo largo del eje LIDAR real; cada alero sube
// a su proyección en el caballete (los extremos forman los hastiales triangulares).
void AEdificioGenerado::TejadoDosAguas(const TArray<FVector2D>& P, float AlturaCm, float RoofH, FVector2D Eje, FColor Color,
	TArray<FVector>& V, TArray<int32>& T, TArray<FVector>& N, TArray<FVector2D>& UV, TArray<FColor>& C)
{
	const int32 n = P.Num();
	FVector2D mn = P[0], mx = P[0];
	for (const FVector2D& v : P) { mn.X = FMath::Min(mn.X, v.X); mn.Y = FMath::Min(mn.Y, v.Y); mx.X = FMath::Max(mx.X, v.X); mx.Y = FMath::Max(mx.Y, v.Y); }
	const FVector2D Centro = (mn + mx) * 0.5f;
	FVector2D d = Eje.GetSafeNormal(); if (d.IsNearlyZero()) d = FVector2D(1, 0);

	float tmin = FVector2D::DotProduct(P[0] - Centro, d), tmax = tmin;
	for (const FVector2D& v : P) { const float t = FVector2D::DotProduct(v - Centro, d); tmin = FMath::Min(tmin, t); tmax = FMath::Max(tmax, t); }
	const FVector2D R0 = Centro + d * tmin, R1 = Centro + d * tmax;   // caballete (XY)

	auto EnCaballete = [&](const FVector2D& Q)
	{
		const FVector2D ab = R1 - R0; const float L2 = FVector2D::DotProduct(ab, ab);
		const float t = (L2 > 1.f) ? FMath::Clamp(FVector2D::DotProduct(Q - R0, ab) / L2, 0.f, 1.f) : 0.f;
		return R0 + ab * t;
	};

	for (int32 i = 0; i < n; ++i)
	{
		const FVector2D a = P[i], b = P[(i + 1) % n];
		const FVector2D ra = EnCaballete(a), rb = EnCaballete(b);
		const FVector A(a.X, a.Y, AlturaCm),  B(b.X, b.Y, AlturaCm);
		const FVector RA(ra.X, ra.Y, AlturaCm + RoofH), RB(rb.X, rb.Y, AlturaCm + RoofH);
		FVector nrm = FVector::CrossProduct(B - A, RA - A).GetSafeNormal();
		if (nrm.IsNearlyZero()) continue;
		const bool flip = nrm.Z < 0.f; if (flip) nrm = -nrm;
		const int32 base = V.Num();
		V.Add(A); V.Add(B); V.Add(RB); V.Add(RA);   // quad alero->caballete (degenera a triángulo en hastiales)
		for (int32 k = 0; k < 4; ++k) { N.Add(nrm); C.Add(Color); }
		UV.Add({0,0}); UV.Add({1,0}); UV.Add({1,1}); UV.Add({0,1});
		if (flip) { T.Add(base); T.Add(base + 2); T.Add(base + 1); T.Add(base); T.Add(base + 3); T.Add(base + 2); }
		else      { T.Add(base); T.Add(base + 1); T.Add(base + 2); T.Add(base); T.Add(base + 2); T.Add(base + 3); }
	}
}
