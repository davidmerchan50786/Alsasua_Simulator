// TrianguladorPoligono.h (capa WORLD)
// Ear-clipping para polígonos simples (sin auto-intersección). Devuelve índices
// de triángulos sobre el array de entrada. Compartido por edificios y suelos.
#pragma once

#include "CoreMinimal.h"

namespace TrianguladorPoligono
{
	inline double Area2(const TArray<FVector2D>& P)
	{
		double A = 0.0;
		for (int32 i = 0, n = P.Num(); i < n; ++i)
		{ const FVector2D& a = P[i]; const FVector2D& b = P[(i + 1) % n]; A += (a.X * b.Y - b.X * a.Y); }
		return A * 0.5;
	}

	inline bool DentroTri(const FVector2D& a, const FVector2D& b, const FVector2D& c, const FVector2D& p)
	{
		const double d1 = (p.X - b.X) * (a.Y - b.Y) - (a.X - b.X) * (p.Y - b.Y);
		const double d2 = (p.X - c.X) * (b.Y - c.Y) - (b.X - c.X) * (p.Y - c.Y);
		const double d3 = (p.X - a.X) * (c.Y - a.Y) - (c.X - a.X) * (p.Y - a.Y);
		const bool neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
		const bool pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
		return !(neg && pos);
	}

	// Triangula P (CCW o CW) -> OutTris (índices a P). Trabaja internamente en CCW.
	inline void Triangular(const TArray<FVector2D>& P, TArray<int32>& OutTris)
	{
		const int32 N = P.Num();
		if (N < 3) return;
		TArray<int32> V; V.Reserve(N);
		const bool CCW = Area2(P) > 0.0;
		for (int32 i = 0; i < N; ++i) V.Add(CCW ? i : (N - 1 - i));

		int32 guard = 0;
		while (V.Num() > 2 && guard++ < N * N)
		{
			bool corto = false;
			const int32 m = V.Num();
			for (int32 i = 0; i < m; ++i)
			{
				const int32 i0 = V[(i + m - 1) % m], i1 = V[i], i2 = V[(i + 1) % m];
				const FVector2D& a = P[i0]; const FVector2D& b = P[i1]; const FVector2D& c = P[i2];
				if (((b.X - a.X) * (c.Y - a.Y) - (b.Y - a.Y) * (c.X - a.X)) <= 0.0) continue;   // convexo CCW
				bool ok = true;
				for (int32 j = 0; j < m; ++j)
				{
					const int32 vj = V[j];
					if (vj == i0 || vj == i1 || vj == i2) continue;
					if (DentroTri(a, b, c, P[vj])) { ok = false; break; }
				}
				if (!ok) continue;
				OutTris.Add(i0); OutTris.Add(i1); OutTris.Add(i2);
				V.RemoveAt(i); corto = true; break;
			}
			if (!corto) break;
		}
	}
}
