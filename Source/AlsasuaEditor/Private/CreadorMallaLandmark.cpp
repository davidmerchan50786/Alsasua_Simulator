// CreadorMallaLandmark.cpp (sólo editor)
#include "CreadorMallaLandmark.h"
#include "ConstructorMallaComun.h"

namespace
{
	const TCHAR* CarpetaLandmarks = TEXT("/Game/Landmarks");

	// Materiales del pueblo. El color por vértice hace de tinte encima.
	const TCHAR* MatPiedra = TEXT("/Game/Materiales/M_Muro_Piedra.M_Muro_Piedra");
	const TCHAR* MatTejas  = TEXT("/Game/Materiales/M_Techo_Tejas.M_Techo_Tejas");
	const TCHAR* MatEdif   = TEXT("/Game/Materiales/M_Edificio.M_Edificio");

	const FLinearColor Caliza    (0.74f, 0.71f, 0.64f);   // sillería clara
	const FLinearColor Mamposteo (0.58f, 0.55f, 0.49f);
	const FLinearColor Teja      (0.52f, 0.26f, 0.17f);
	const FLinearColor Revoco    (0.80f, 0.78f, 0.73f);
	const FLinearColor Hormigon  (0.66f, 0.66f, 0.64f);
	const FLinearColor Zinc      (0.42f, 0.44f, 0.46f);
	const FLinearColor Madera    (0.40f, 0.25f, 0.13f);
	const FLinearColor Cancha    (0.55f, 0.42f, 0.30f);

	/**
	 * Tejado a dos aguas: dos faldones inclinados y los dos hastiales.
	 * Ancho en X, largo en Y, cumbrera paralela a Y.
	 */
	void DosAguas(AlsasuaMalla::FConstructorMalla& C, const FVector& Centro,
		float Ancho, float Largo, float Alto, const FLinearColor& Color)
	{
		const float HX = Ancho * 0.5f, HY = Largo * 0.5f;
		const float Z0 = Centro.Z, Z1 = Centro.Z + Alto;
		const float X0 = Centro.X - HX, X1 = Centro.X + HX;
		const float Y0 = Centro.Y - HY, Y1 = Centro.Y + HY;

		// Faldón oeste y faldón este, desde el alero hasta la cumbrera.
		C.Cara(FVector(X0, Y0, Z0), FVector(X0, Y1, Z0), FVector(Centro.X, Y1, Z1), FVector(Centro.X, Y0, Z1),
			FVector(-Alto, 0.f, HX).GetSafeNormal(), Color, 0.01f);
		C.Cara(FVector(Centro.X, Y0, Z1), FVector(Centro.X, Y1, Z1), FVector(X1, Y1, Z0), FVector(X1, Y0, Z0),
			FVector(Alto, 0.f, HX).GetSafeNormal(), Color, 0.01f);

		// Hastiales, el triángulo de cada extremo.
		C.Triangulo(FVector(X0, Y0, Z0), FVector(X1, Y0, Z0), FVector(Centro.X, Y0, Z1), FVector(0.f, -1.f, 0.f), Color, 0.01f);
		C.Triangulo(FVector(X1, Y1, Z0), FVector(X0, Y1, Z0), FVector(Centro.X, Y1, Z1), FVector(0.f,  1.f, 0.f), Color, 0.01f);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  Iglesia (2 en el JSON: Jasokundeko Andre Maria y San Martin Toursko).
//  Nave de 14 x 34 m con torre campanario de 22 m a los pies.
// ─────────────────────────────────────────────────────────────────────────────
bool UCreadorMallaLandmark::GenerarIglesia()
{
	AlsasuaMalla::FConstructorMalla C;

	// Nave: 14 m de ancho, 34 de largo, 12 de alto hasta el alero.
	C.Caja(FVector(0.f, 0.f, 600.f), FVector(1400.f, 3400.f, 1200.f), Caliza);
	DosAguas(C, FVector(0.f, 0.f, 1200.f), 1500.f, 3500.f, 450.f, Teja);

	// Ábside semicircular al fondo.
	C.Prisma(FVector(0.f, 1700.f, 0.f), 700.f, 700.f, 1100.f, 12, Caliza);

	// Torre campanario a los pies, 6 x 6 y 22 m.
	C.Caja(FVector(0.f, -1900.f, 1100.f), FVector(600.f, 600.f, 2200.f), Mamposteo);
	// Vanos del campanario en las cuatro caras.
	for (int32 s = -1; s <= 1; s += 2)
	{
		C.Caja(FVector(s * 300.f, -1900.f, 1850.f), FVector(30.f, 200.f, 350.f), FLinearColor::Black);
		C.Caja(FVector(0.f, -1900.f + s * 300.f, 1850.f), FVector(200.f, 30.f, 350.f), FLinearColor::Black);
	}
	// Chapitel piramidal.
	C.Prisma(FVector(0.f, -1900.f, 2200.f), 460.f, 0.f, 500.f, 4, Zinc);

	// Portada y pórtico.
	C.Caja(FVector(0.f, -1710.f, 300.f), FVector(320.f, 40.f, 600.f), Madera);

	return AlsasuaMalla::Guardar(C, CarpetaLandmarks, TEXT("SM_Iglesia"), MatPiedra);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Frontón (2 en el JSON, uno el Burunda). Es el edificio más
//  característico: frontis alto, pared izquierda larga y cancha abierta.
//  Medidas de frontón corto: 30 m de cancha, frontis de 10 m.
// ─────────────────────────────────────────────────────────────────────────────
bool UCreadorMallaLandmark::GenerarFronton()
{
	AlsasuaMalla::FConstructorMalla C;

	// Frontis: el paredón contra el que se juega, 11 x 10 m.
	C.Caja(FVector(0.f, -1500.f, 500.f), FVector(1100.f, 80.f, 1000.f), Hormigon);
	// Franja roja de falta y chapa inferior.
	C.Caja(FVector(0.f, -1455.f, 90.f), FVector(1100.f, 10.f, 180.f), FLinearColor(0.35f, 0.06f, 0.05f));

	// Pared izquierda a lo largo de la cancha, bajando de 10 a 6 m.
	C.Caja(FVector(-550.f, 0.f, 400.f), FVector(80.f, 3000.f, 800.f), Hormigon);

	// Cancha.
	C.Caja(FVector(0.f, 0.f, 2.f), FVector(1100.f, 3000.f, 4.f), Cancha);

	// Rebote al fondo, más bajo.
	C.Caja(FVector(0.f, 1500.f, 250.f), FVector(1100.f, 80.f, 500.f), Hormigon);

	// Graderío corrido en el lado abierto.
	for (int32 i = 0; i < 4; ++i)
	{
		C.Caja(FVector(620.f + i * 80.f, 0.f, 40.f + i * 40.f),
			FVector(80.f, 3000.f, 80.f + i * 80.f), Hormigon);
	}

	// Cubierta sobre la cancha, apoyada en la pared izquierda.
	C.Caja(FVector(0.f, 0.f, 1050.f), FVector(1300.f, 3100.f, 30.f), Zinc);
	for (int32 i = -1; i <= 1; ++i)
	{
		C.Caja(FVector(600.f, i * 1200.f, 520.f), FVector(50.f, 50.f, 1040.f), Zinc);
	}

	return AlsasuaMalla::Guardar(C, CarpetaLandmarks, TEXT("SM_Fronton"), MatEdif);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Ayuntamiento. La descripción de poi_data.json dice "arkupe de tres ojos",
//  el pórtico de tres arcos de la casa municipal de 1760.
// ─────────────────────────────────────────────────────────────────────────────
bool UCreadorMallaLandmark::GenerarAyuntamiento()
{
	AlsasuaMalla::FConstructorMalla C;

	// Cuerpo de 18 x 12 m, tres plantas.
	C.Caja(FVector(0.f, 0.f, 600.f), FVector(1800.f, 1200.f, 1200.f), Caliza);
	DosAguas(C, FVector(0.f, 0.f, 1200.f), 1900.f, 1300.f, 380.f, Teja);

	// Arkupe: tres ojos en planta baja. Los pilares dejan los tres huecos.
	for (int32 i = 0; i < 4; ++i)
	{
		const float X = -750.f + i * 500.f;
		C.Caja(FVector(X, -640.f, 200.f), FVector(120.f, 120.f, 400.f), Mamposteo);
	}
	// Dintel corrido sobre los arcos.
	C.Caja(FVector(0.f, -640.f, 430.f), FVector(1800.f, 140.f, 60.f), Mamposteo);

	// Balcón corrido de la planta noble.
	C.Caja(FVector(0.f, -620.f, 800.f), FVector(1500.f, 60.f, 20.f), Madera);
	C.Caja(FVector(0.f, -650.f, 850.f), FVector(1500.f, 10.f, 90.f), Madera);

	// Escudo de la villa sobre el balcón.
	C.Caja(FVector(0.f, -610.f, 1050.f), FVector(160.f, 20.f, 200.f), Mamposteo);

	// Reloj en el hastial.
	C.Caja(FVector(0.f, -660.f, 1330.f), FVector(180.f, 15.f, 180.f), FLinearColor::White);

	return AlsasuaMalla::Guardar(C, CarpetaLandmarks, TEXT("SM_Ayuntamiento"), MatPiedra);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Escuela: ikastola, colegio, escuela pública, casa de cultura, gaztetxe.
//  Bloque en L de tres plantas con ventanas corridas.
// ─────────────────────────────────────────────────────────────────────────────
bool UCreadorMallaLandmark::GenerarEscuela()
{
	AlsasuaMalla::FConstructorMalla C;

	// Ala principal 32 x 14, tres plantas de 3,2 m.
	C.Caja(FVector(0.f, 0.f, 480.f), FVector(3200.f, 1400.f, 960.f), Revoco);
	// Ala corta perpendicular.
	C.Caja(FVector(-1600.f, 1200.f, 480.f), FVector(1400.f, 1000.f, 960.f), Revoco);

	// Ventanas corridas por planta: bandas oscuras en la fachada larga.
	for (int32 p = 0; p < 3; ++p)
	{
		const float Z = 180.f + p * 320.f;
		C.Caja(FVector(0.f, -710.f, Z), FVector(2900.f, 20.f, 140.f), FLinearColor(0.10f, 0.13f, 0.16f));
		C.Caja(FVector(0.f,  710.f, Z), FVector(2900.f, 20.f, 140.f), FLinearColor(0.10f, 0.13f, 0.16f));
	}

	// Cubierta plana con peto.
	C.Caja(FVector(0.f, 0.f, 975.f), FVector(3260.f, 1460.f, 30.f), Hormigon);
	C.Caja(FVector(0.f, 0.f, 1010.f), FVector(3260.f, 1460.f, 40.f), Hormigon);

	// Zócalo de piedra.
	C.Caja(FVector(0.f, 0.f, 40.f), FVector(3240.f, 1440.f, 80.f), Mamposteo);

	// Porche de entrada.
	C.Caja(FVector(0.f, -800.f, 330.f), FVector(600.f, 200.f, 30.f), Hormigon);

	return AlsasuaMalla::Guardar(C, CarpetaLandmarks, TEXT("SM_Escuela"), MatEdif);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Nave: polideportivo y mercado. Cubierta a dos aguas de poca pendiente
//  sobre nave diáfana de 40 x 24 m.
// ─────────────────────────────────────────────────────────────────────────────
bool UCreadorMallaLandmark::GenerarNave()
{
	AlsasuaMalla::FConstructorMalla C;

	C.Caja(FVector(0.f, 0.f, 450.f), FVector(2400.f, 4000.f, 900.f), Hormigon);
	DosAguas(C, FVector(0.f, 0.f, 900.f), 2500.f, 4100.f, 350.f, Zinc);

	// Lucernario corrido en la cumbrera.
	C.Caja(FVector(0.f, 0.f, 1180.f), FVector(200.f, 3600.f, 60.f), FLinearColor(0.75f, 0.82f, 0.88f));

	// Franja alta de ventanas en los laterales largos.
	for (int32 s = -1; s <= 1; s += 2)
	{
		C.Caja(FVector(s * 1210.f, 0.f, 700.f), FVector(20.f, 3600.f, 200.f), FLinearColor(0.12f, 0.15f, 0.18f));
	}

	// Pilastras estructurales.
	for (int32 i = -2; i <= 2; ++i)
	{
		for (int32 s = -1; s <= 1; s += 2)
		{
			C.Caja(FVector(s * 1220.f, i * 800.f, 450.f), FVector(60.f, 60.f, 900.f), Hormigon);
		}
	}

	// Puerta grande en el hastial.
	C.Caja(FVector(0.f, -2010.f, 250.f), FVector(700.f, 20.f, 500.f), Zinc);

	return AlsasuaMalla::Guardar(C, CarpetaLandmarks, TEXT("SM_Nave"), MatEdif);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Estación de tren. Alsasua es nudo ferroviario: edificio de viajeros
//  alargado con marquesina de andén.
// ─────────────────────────────────────────────────────────────────────────────
bool UCreadorMallaLandmark::GenerarEstacion()
{
	AlsasuaMalla::FConstructorMalla C;

	// Edificio de viajeros, 36 x 12, dos plantas, cuerpo central más alto.
	C.Caja(FVector(0.f, 0.f, 400.f), FVector(3600.f, 1200.f, 800.f), Caliza);
	DosAguas(C, FVector(0.f, 0.f, 800.f), 3700.f, 1300.f, 300.f, Teja);
	C.Caja(FVector(0.f, 0.f, 1100.f), FVector(1000.f, 1300.f, 400.f), Caliza);
	DosAguas(C, FVector(0.f, 0.f, 1500.f), 1100.f, 1400.f, 250.f, Teja);

	// Arcos de ventana en la planta baja del andén.
	for (int32 i = -5; i <= 5; ++i)
	{
		C.Caja(FVector(i * 300.f, 610.f, 300.f), FVector(140.f, 20.f, 400.f), FLinearColor(0.10f, 0.12f, 0.15f));
	}

	// Marquesina del andén, en voladizo.
	C.Caja(FVector(0.f, 1400.f, 700.f), FVector(3600.f, 1600.f, 25.f), Zinc);
	for (int32 i = -3; i <= 3; ++i)
	{
		C.Caja(FVector(i * 550.f, 2100.f, 350.f), FVector(40.f, 40.f, 700.f), Zinc);
	}

	// Andén.
	C.Caja(FVector(0.f, 1400.f, 25.f), FVector(3600.f, 1600.f, 50.f), Hormigon);

	// Reloj de estación en el cuerpo central.
	C.Caja(FVector(0.f, -620.f, 1150.f), FVector(160.f, 15.f, 160.f), FLinearColor::White);

	return AlsasuaMalla::Guardar(C, CarpetaLandmarks, TEXT("SM_Estacion"), MatPiedra);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Bloque cívico: juzgado, centro de salud, biblioteca. Dos plantas, 20 x 14.
// ─────────────────────────────────────────────────────────────────────────────
bool UCreadorMallaLandmark::GenerarBloqueCivico()
{
	AlsasuaMalla::FConstructorMalla C;

	C.Caja(FVector(0.f, 0.f, 350.f), FVector(2000.f, 1400.f, 700.f), Revoco);
	C.Caja(FVector(0.f, 0.f, 40.f), FVector(2040.f, 1440.f, 80.f), Mamposteo);   // zócalo

	// Ventanas en dos plantas.
	for (int32 p = 0; p < 2; ++p)
	{
		const float Z = 200.f + p * 320.f;
		for (int32 i = -3; i <= 3; ++i)
		{
			C.Caja(FVector(i * 280.f, -710.f, Z), FVector(160.f, 20.f, 160.f), FLinearColor(0.10f, 0.13f, 0.16f));
			C.Caja(FVector(i * 280.f,  710.f, Z), FVector(160.f, 20.f, 160.f), FLinearColor(0.10f, 0.13f, 0.16f));
		}
	}

	DosAguas(C, FVector(0.f, 0.f, 700.f), 2100.f, 1500.f, 320.f, Teja);

	// Entrada con marquesina.
	C.Caja(FVector(0.f, -730.f, 220.f), FVector(400.f, 60.f, 440.f), Madera);
	C.Caja(FVector(0.f, -820.f, 460.f), FVector(500.f, 240.f, 25.f), Hormigon);

	return AlsasuaMalla::Guardar(C, CarpetaLandmarks, TEXT("SM_BloqueCivico"), MatEdif);
}

int32 UCreadorMallaLandmark::GenerarTodos()
{
	int32 Ok = 0;
	Ok += GenerarIglesia()       ? 1 : 0;
	Ok += GenerarFronton()       ? 1 : 0;
	Ok += GenerarAyuntamiento()  ? 1 : 0;
	Ok += GenerarEscuela()       ? 1 : 0;
	Ok += GenerarNave()          ? 1 : 0;
	Ok += GenerarEstacion()      ? 1 : 0;
	Ok += GenerarBloqueCivico()  ? 1 : 0;

	UE_LOG(LogTemp, Log, TEXT("[Landmarks] %d/7 arquetipos generados en %s"), Ok, CarpetaLandmarks);
	return Ok;
}
