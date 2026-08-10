// CreadorMallaArbol.cpp (sólo editor)
#include "CreadorMallaArbol.h"
#include "ConstructorMallaComun.h"

namespace
{
	const TCHAR* CarpetaArboles = TEXT("/Game/Meshes/Arboles");
	const TCHAR* MatArbol = TEXT("/Game/Materiales/M_Arbol.M_Arbol");

	// AlturaReferenciaMalla de UCargadorArboles: todas las especies se
	// construyen a esta altura y el cargador escala cada instancia con la
	// "altura" real del censo.
	constexpr float AlturaRef = 1000.f;   // 10 m en cm

	// Cortezas. La copa la tinta el cargador por especie con el parámetro
	// Color del material, así que aquí el verde es sólo el valor base.
	const FLinearColor CortezaOscura(0.20f, 0.14f, 0.10f);   // roble, haya
	const FLinearColor CortezaRojiza(0.34f, 0.19f, 0.12f);   // pino silvestre
	const FLinearColor CortezaClara (0.78f, 0.76f, 0.70f);   // abedul
	const FLinearColor CortezaGris  (0.44f, 0.42f, 0.38f);   // plátano, chopo
	const FLinearColor Copa         (0.24f, 0.40f, 0.16f);

	/** Pocos lados: son 2783 instancias y hay que mantener el triángulo a raya. */
	constexpr int32 LadosTronco = 6;
	constexpr int32 LadosCopa   = 8;

	/**
	 * Copa redondeada como pila de prismas: ensancha y luego cierra. Da una
	 * masa creíble a bajo coste, sin necesitar una esfera.
	 */
	void CopaRedonda(AlsasuaMalla::FConstructorMalla& C, float Base, float Alto, float Radio,
		const FLinearColor& Color)
	{
		// Perfil relativo de la copa: cada tramo va de un radio al siguiente.
		const float Perfil[] = { 0.45f, 0.85f, 1.0f, 0.88f, 0.5f, 0.f };
		constexpr int32 Tramos = UE_ARRAY_COUNT(Perfil) - 1;
		const float AltoTramo = Alto / Tramos;

		for (int32 i = 0; i < Tramos; ++i)
		{
			C.Prisma(FVector(0.f, 0.f, Base + i * AltoTramo),
				Radio * Perfil[i], Radio * Perfil[i + 1], AltoTramo, LadosCopa, Color);
		}
	}

	/** Copa cónica en pisos, para coníferas. */
	void CopaConica(AlsasuaMalla::FConstructorMalla& C, float Base, float Alto, float Radio,
		const FLinearColor& Color)
	{
		constexpr int32 Pisos = 3;
		const float AltoPiso = Alto / Pisos;
		for (int32 i = 0; i < Pisos; ++i)
		{
			// Cada piso arranca ancho y cierra, y el siguiente solapa: da el
			// escalonado de las ramas de un pino.
			const float R = Radio * (1.f - i * 0.28f);
			C.Prisma(FVector(0.f, 0.f, Base + i * AltoPiso * 0.85f),
				R, R * 0.25f, AltoPiso * 1.15f, LadosCopa, Color);
		}
	}

	/** Tronco troncocónico, opcionalmente en dos tramos para dar entasis. */
	void Tronco(AlsasuaMalla::FConstructorMalla& C, float Alto, float RadioBase, float RadioAlto,
		const FLinearColor& Color)
	{
		const float Medio = Alto * 0.55f;
		const float RadioMedio = FMath::Lerp(RadioBase, RadioAlto, 0.55f);
		C.Prisma(FVector::ZeroVector, RadioBase, RadioMedio, Medio, LadosTronco, Color);
		C.Prisma(FVector(0.f, 0.f, Medio), RadioMedio, RadioAlto, Alto - Medio, LadosTronco, Color);
	}

	/** Ramas rectas que asoman de la copa: rompen la silueta de "piruleta". */
	void Ramas(AlsasuaMalla::FConstructorMalla& C, float Base, float Largo, int32 Num,
		const FLinearColor& Color)
	{
		for (int32 i = 0; i < Num; ++i)
		{
			const float A = (2.f * PI * i) / Num;
			const float X = FMath::Cos(A) * Largo * 0.5f;
			const float Y = FMath::Sin(A) * Largo * 0.5f;
			C.Caja(FVector(X, Y, Base), FVector(Largo, 12.f, 12.f), Color);
		}
	}

	bool Guardar(AlsasuaMalla::FConstructorMalla& C, const TCHAR* Especie)
	{
		return AlsasuaMalla::Guardar(C, CarpetaArboles, FString(TEXT("SM_")) + Especie, MatArbol);
	}

	// ── Especies ───────────────────────────────────────────────────────────
	// Proporciones de cada una: fracción del alto que ocupa el tronco limpio,
	// radio de copa y grosor. Salen del porte real de cada árbol.

	bool Roble()      // Quercus robur/ilex: 1010 del censo, el más repetido
	{
		AlsasuaMalla::FConstructorMalla C;
		Tronco(C, AlturaRef * 0.40f, 45.f, 32.f, CortezaOscura);
		Ramas(C, AlturaRef * 0.42f, 340.f, 5, CortezaOscura);
		CopaRedonda(C, AlturaRef * 0.34f, AlturaRef * 0.66f, 380.f, Copa);   // copa ancha
		return Guardar(C, TEXT("QuercusRobur"));
	}

	bool Pino()       // Pinus sylvestris/radiata: 858. Tronco limpio y alto.
	{
		AlsasuaMalla::FConstructorMalla C;
		Tronco(C, AlturaRef * 0.62f, 30.f, 18.f, CortezaRojiza);
		CopaConica(C, AlturaRef * 0.55f, AlturaRef * 0.45f, 250.f, Copa);
		return Guardar(C, TEXT("Pinus"));
	}

	bool Haya()       // Fagus sylvatica: 450. Fuste recto y copa alta ovalada.
	{
		AlsasuaMalla::FConstructorMalla C;
		Tronco(C, AlturaRef * 0.50f, 38.f, 26.f, CortezaOscura);
		Ramas(C, AlturaRef * 0.52f, 260.f, 4, CortezaOscura);
		CopaRedonda(C, AlturaRef * 0.44f, AlturaRef * 0.56f, 290.f, Copa);
		return Guardar(C, TEXT("Fagus"));
	}

	bool Abedul()     // Betula: 161. Esbelto, corteza blanca, copa ligera.
	{
		AlsasuaMalla::FConstructorMalla C;
		Tronco(C, AlturaRef * 0.55f, 20.f, 13.f, CortezaClara);
		CopaRedonda(C, AlturaRef * 0.48f, AlturaRef * 0.52f, 190.f, Copa);
		return Guardar(C, TEXT("Betula"));
	}

	bool Chopo()      // Populus: 72. Columnar, casi un cirio.
	{
		AlsasuaMalla::FConstructorMalla C;
		Tronco(C, AlturaRef * 0.70f, 26.f, 16.f, CortezaGris);
		CopaRedonda(C, AlturaRef * 0.22f, AlturaRef * 0.78f, 140.f, Copa);  // estrecha y larga
		return Guardar(C, TEXT("Populus"));
	}

	bool Sauce()      // Salix: 58. Copa ancha y colgante junto al Arakil.
	{
		AlsasuaMalla::FConstructorMalla C;
		Tronco(C, AlturaRef * 0.32f, 40.f, 28.f, CortezaGris);
		Ramas(C, AlturaRef * 0.36f, 420.f, 6, CortezaGris);
		CopaRedonda(C, AlturaRef * 0.28f, AlturaRef * 0.62f, 400.f, Copa);
		// Ramillas colgantes, lo que le da la silueta llorona.
		for (int32 i = 0; i < 8; ++i)
		{
			const float A = (2.f * PI * i) / 8;
			C.Caja(FVector(FMath::Cos(A) * 300.f, FMath::Sin(A) * 300.f, AlturaRef * 0.34f),
				FVector(14.f, 14.f, AlturaRef * 0.34f), Copa);
		}
		return Guardar(C, TEXT("Salix"));
	}

	bool Cerezo()     // Prunus: 28. Porte pequeño y redondo.
	{
		AlsasuaMalla::FConstructorMalla C;
		Tronco(C, AlturaRef * 0.38f, 24.f, 18.f, CortezaOscura);
		CopaRedonda(C, AlturaRef * 0.34f, AlturaRef * 0.60f, 230.f, Copa);
		return Guardar(C, TEXT("Prunus"));
	}

	bool Tilo()       // Tilia: 27. También cubre Fraxinus (fresno), 70.
	{
		AlsasuaMalla::FConstructorMalla C;
		Tronco(C, AlturaRef * 0.45f, 34.f, 24.f, CortezaOscura);
		Ramas(C, AlturaRef * 0.47f, 280.f, 5, CortezaOscura);
		CopaRedonda(C, AlturaRef * 0.40f, AlturaRef * 0.60f, 300.f, Copa);
		return Guardar(C, TEXT("Tilia"));
	}

	bool Platano()    // Platanus: el plátano de sombra de las plazas, muy podado.
	{
		AlsasuaMalla::FConstructorMalla C;
		Tronco(C, AlturaRef * 0.48f, 40.f, 30.f, CortezaGris);
		// La poda en candelabro deja muñones gruesos y horizontales.
		Ramas(C, AlturaRef * 0.50f, 300.f, 4, CortezaGris);
		CopaRedonda(C, AlturaRef * 0.44f, AlturaRef * 0.56f, 330.f, Copa);
		return Guardar(C, TEXT("Platanus"));
	}

	bool Arce()       // Acer.
	{
		AlsasuaMalla::FConstructorMalla C;
		Tronco(C, AlturaRef * 0.42f, 30.f, 22.f, CortezaOscura);
		CopaRedonda(C, AlturaRef * 0.38f, AlturaRef * 0.62f, 270.f, Copa);
		return Guardar(C, TEXT("Acer"));
	}
}

int32 UCreadorMallaArbol::GenerarTodas()
{
	int32 Ok = 0;
	Ok += Roble()   ? 1 : 0;
	Ok += Pino()    ? 1 : 0;
	Ok += Haya()    ? 1 : 0;
	Ok += Abedul()  ? 1 : 0;
	Ok += Chopo()   ? 1 : 0;
	Ok += Sauce()   ? 1 : 0;
	Ok += Cerezo()  ? 1 : 0;
	Ok += Tilo()    ? 1 : 0;
	Ok += Platano() ? 1 : 0;
	Ok += Arce()    ? 1 : 0;

	UE_LOG(LogTemp, Log, TEXT("[Arboles] %d/10 especies generadas en %s"), Ok, CarpetaArboles);
	return Ok;
}
