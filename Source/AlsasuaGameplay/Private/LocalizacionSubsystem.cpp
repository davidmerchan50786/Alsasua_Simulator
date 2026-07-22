// LocalizacionSubsystem.cpp
#include "LocalizacionSubsystem.h"

void ULocalizacionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Sembrar();
}

FString ULocalizacionSubsystem::Texto(FName Clave) const
{
	if (const FTextoLoc* T = Tabla.Find(Clave))
		return (Idioma == EIdioma::Euskera && !T->EU.IsEmpty()) ? T->EU : T->ES;
	return Clave.ToString();   // fallback: la propia clave
}

void ULocalizacionSubsystem::Sembrar()
{
	auto Add = [&](const TCHAR* K, const TCHAR* ES, const TCHAR* EU)
	{ FTextoLoc T; T.ES = ES; T.EU = EU; Tabla.Add(FName(K), T); };

	// Menú
	Add(TEXT("ui.reanudar"),       TEXT("Reanudar"),         TEXT("Jarraitu"));
	Add(TEXT("ui.guardar"),        TEXT("Guardar partida"),  TEXT("Partida gorde"));
	Add(TEXT("ui.cargar"),         TEXT("Cargar partida"),   TEXT("Partida kargatu"));
	Add(TEXT("ui.opciones"),       TEXT("Opciones"),         TEXT("Aukerak"));
	Add(TEXT("ui.menu_principal"), TEXT("Menu principal"),   TEXT("Menu nagusia"));
	Add(TEXT("ui.salir"),          TEXT("Salir del juego"),  TEXT("Jokotik irten"));
	Add(TEXT("ui.volver"),         TEXT("Volver"),           TEXT("Itzuli"));
	Add(TEXT("ui.calidad"),        TEXT("Calidad grafica"),  TEXT("Kalitate grafikoa"));
	Add(TEXT("ui.idioma"),         TEXT("Idioma"),           TEXT("Hizkuntza"));
	Add(TEXT("ui.volumen"),        TEXT("Volumen"),          TEXT("Bolumena"));
	Add(TEXT("ui.nueva"),          TEXT("Nueva partida"),    TEXT("Partida berria"));
	Add(TEXT("ui.continuar"),      TEXT("Continuar"),        TEXT("Jarraitu"));
	Add(TEXT("ui.pausa"),          TEXT("PAUSA"),            TEXT("ETENALDIA"));

	// HUD
	Add(TEXT("hud.salud"),    TEXT("Salud"),          TEXT("Osasuna"));
	Add(TEXT("hud.dinero"),   TEXT("Dinero"),         TEXT("Dirua"));
	Add(TEXT("hud.apoyo"),    TEXT("Apoyo popular"),  TEXT("Herri babesa"));
	Add(TEXT("hud.busqueda"), TEXT("Busqueda"),       TEXT("Bilaketa"));
	Add(TEXT("hud.manifa"),   TEXT("Manifa"),         TEXT("Manifa"));
}
