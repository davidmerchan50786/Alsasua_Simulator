// CreadorMallaMobiliario.cpp (sólo editor)
#include "CreadorMallaMobiliario.h"
#include "ConstructorMallaComun.h"
#include "Misc/Paths.h"

namespace
{
	const TCHAR* CarpetaMobiliario = TEXT("/Game/Mobiliario");

	// Paleta del mobiliario de Alsasua. Va a color por vértice, y los
	// materiales la usan de tinte: M_Mobiliario sobre la veta de madera y
	// M_Metal sobre la chapa.
	const FLinearColor MaderaMobiliario(0.42f, 0.26f, 0.14f);
	const FLinearColor HierroVerde (0.10f, 0.18f, 0.13f);   // verde Navarra
	const FLinearColor HierroNegro (0.06f, 0.06f, 0.07f);
	const FLinearColor Fundicion   (0.20f, 0.20f, 0.22f);
	const FLinearColor RojoCorreos (0.55f, 0.06f, 0.06f);
	const FLinearColor Piedra      (0.62f, 0.60f, 0.55f);
	const FLinearColor Terracota   (0.55f, 0.28f, 0.18f);

	const TCHAR* MatMadera = TEXT("/Game/Materiales/M_Mobiliario.M_Mobiliario");
	const TCHAR* MatMetal  = TEXT("/Game/Materiales/M_Metal.M_Metal");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Banco: 24 en street_furniture.json, "madera_piedra".
//  Listones de madera sobre dos pies de fundición, 180 x 45, asiento a 45 cm.
// ─────────────────────────────────────────────────────────────────────────────
bool UCreadorMallaMobiliario::GenerarBanco()
{
	AlsasuaMalla::FConstructorMalla C;

	// Cinco listones de asiento con hueco entre ellos.
	for (int32 i = 0; i < 5; ++i)
	{
		const float Y = -18.f + i * 9.f;
		C.Caja(FVector(0.f, Y, 45.f), FVector(180.f, 7.f, 4.f), MaderaMobiliario);
	}
	// Tres listones de respaldo, inclinados de forma sencilla por altura.
	for (int32 i = 0; i < 3; ++i)
	{
		C.Caja(FVector(0.f, 20.f + i * 1.5f, 60.f + i * 9.f), FVector(180.f, 6.f, 4.f), MaderaMobiliario);
	}
	// Pies: pata y zapata a cada lado.
	for (int32 s = -1; s <= 1; s += 2)
	{
		const float X = s * 75.f;
		C.Caja(FVector(X, 0.f, 22.f), FVector(8.f, 42.f, 44.f), Fundicion);
		C.Caja(FVector(X, 0.f, 2.f),  FVector(14.f, 48.f, 4.f), Fundicion);
		C.Caja(FVector(X, 24.f, 62.f), FVector(6.f, 6.f, 40.f), Fundicion);
	}

	return AlsasuaMalla::Guardar(C, CarpetaMobiliario, TEXT("SM_Banco"), MatMadera);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Papelera: 97 piezas, la más repetida del pueblo.
//  Cubo troncocónico de 12 lados sobre poste, con aro superior.
// ─────────────────────────────────────────────────────────────────────────────
bool UCreadorMallaMobiliario::GenerarPapelera()
{
	AlsasuaMalla::FConstructorMalla C;

	C.Prisma(FVector(0.f, 0.f, 40.f), 16.f, 19.f, 45.f, 12, HierroVerde);   // cubo
	C.Prisma(FVector(0.f, 0.f, 85.f), 20.f, 20.f, 3.f,  12, HierroNegro);   // aro
	C.Caja(FVector(0.f, 0.f, 20.f), FVector(8.f, 8.f, 40.f), HierroNegro);  // poste
	C.Caja(FVector(0.f, 0.f, 1.5f), FVector(20.f, 20.f, 3.f), Fundicion);   // base

	return AlsasuaMalla::Guardar(C, CarpetaMobiliario, TEXT("SM_Papelera"), MatMetal);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Bolardo: 19 piezas. Pilona de fundición de 90 cm con remate.
// ─────────────────────────────────────────────────────────────────────────────
bool UCreadorMallaMobiliario::GenerarBolardo()
{
	AlsasuaMalla::FConstructorMalla C;

	C.Prisma(FVector(0.f, 0.f, 3.f),  9.f, 7.f, 80.f, 10, HierroNegro);
	C.Prisma(FVector(0.f, 0.f, 83.f), 8.f, 4.f, 7.f,  10, HierroNegro);     // remate
	C.Prisma(FVector(0.f, 0.f, 0.f),  12.f, 11.f, 3.f, 10, Fundicion);      // zócalo

	return AlsasuaMalla::Guardar(C, CarpetaMobiliario, TEXT("SM_Bolardo"), MatMetal);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Maceta: 6 piezas. Jardinera troncocónica de terracota.
// ─────────────────────────────────────────────────────────────────────────────
bool UCreadorMallaMobiliario::GenerarMaceta()
{
	AlsasuaMalla::FConstructorMalla C;

	C.Prisma(FVector(0.f, 0.f, 0.f),  22.f, 28.f, 40.f, 12, Terracota);
	C.Prisma(FVector(0.f, 0.f, 40.f), 29.f, 29.f, 4.f,  12, Terracota);     // borde
	// La tierra, un poco hundida respecto al borde.
	C.Prisma(FVector(0.f, 0.f, 36.f), 26.f, 26.f, 2.f,  12, FLinearColor(0.16f, 0.11f, 0.07f));

	return AlsasuaMalla::Guardar(C, CarpetaMobiliario, TEXT("SM_Maceta"), MatMadera);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Boca de incendio: 8 piezas. Hidrante bajo con dos salidas.
// ─────────────────────────────────────────────────────────────────────────────
bool UCreadorMallaMobiliario::GenerarBocaIncendio()
{
	AlsasuaMalla::FConstructorMalla C;
	const FLinearColor Rojo(0.5f, 0.05f, 0.04f);

	C.Prisma(FVector(0.f, 0.f, 0.f),  14.f, 13.f, 4.f, 10, Fundicion);      // brida
	C.Prisma(FVector(0.f, 0.f, 4.f),  11.f, 10.f, 55.f, 10, Rojo);          // cuerpo
	C.Prisma(FVector(0.f, 0.f, 59.f), 12.f, 8.f, 8.f, 10, Rojo);            // casquete
	// Salidas laterales.
	C.Caja(FVector( 13.f, 0.f, 35.f), FVector(10.f, 9.f, 9.f), Fundicion);
	C.Caja(FVector(-13.f, 0.f, 35.f), FVector(10.f, 9.f, 9.f), Fundicion);

	return AlsasuaMalla::Guardar(C, CarpetaMobiliario, TEXT("SM_BocaIncendio"), MatMetal);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tapa de alcantarilla: 6 piezas. Disco de fundición casi a ras de suelo.
// ─────────────────────────────────────────────────────────────────────────────
bool UCreadorMallaMobiliario::GenerarTapaAlcantarilla()
{
	AlsasuaMalla::FConstructorMalla C;

	C.Prisma(FVector(0.f, 0.f, 0.f), 32.f, 32.f, 2.f, 20, Fundicion);       // cerco
	C.Prisma(FVector(0.f, 0.f, 2.f), 29.f, 29.f, 1.5f, 20, Fundicion);      // tapa

	return AlsasuaMalla::Guardar(C, CarpetaMobiliario, TEXT("SM_TapaAlcantarilla"), MatMetal);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Buzón de Correos: 4 piezas. El amarillo de Correos va en la instancia;
//  aquí el color por vértice deja el rojo oscuro de los antiguos.
// ─────────────────────────────────────────────────────────────────────────────
bool UCreadorMallaMobiliario::GenerarBuzonCorreos()
{
	AlsasuaMalla::FConstructorMalla C;

	C.Caja(FVector(0.f, 0.f, 65.f), FVector(45.f, 30.f, 60.f), RojoCorreos);   // cuerpo
	C.Caja(FVector(0.f, 0.f, 96.f), FVector(48.f, 33.f, 4.f),  HierroNegro);   // tapa
	C.Caja(FVector(0.f, -15.f, 88.f), FVector(30.f, 2.f, 3.f), HierroNegro);   // ranura
	C.Caja(FVector(0.f, 0.f, 17.f), FVector(10.f, 10.f, 35.f), HierroNegro);   // poste
	C.Caja(FVector(0.f, 0.f, 1.5f), FVector(24.f, 24.f, 3.f),  Fundicion);     // base

	return AlsasuaMalla::Guardar(C, CarpetaMobiliario, TEXT("SM_BuzonCorreos"), MatMetal);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Parada de bus: 12 piezas. Marquesina de 4 postes, techo y banco corrido.
// ─────────────────────────────────────────────────────────────────────────────
bool UCreadorMallaMobiliario::GenerarParadaBus()
{
	AlsasuaMalla::FConstructorMalla C;

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
	C.Caja(FVector(0.f, 20.f, 45.f), FVector(340.f, 40.f, 5.f), MaderaMobiliario); // asiento
	for (int32 s = -1; s <= 1; s += 2)
	{
		C.Caja(FVector(s * 150.f, 20.f, 22.f), FVector(8.f, 36.f, 44.f), HierroVerde);
	}

	return AlsasuaMalla::Guardar(C, CarpetaMobiliario, TEXT("SM_ParadaBus"), MatMadera);
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
