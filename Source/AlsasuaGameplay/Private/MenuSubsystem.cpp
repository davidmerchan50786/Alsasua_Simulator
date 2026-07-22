// MenuSubsystem.cpp
#include "MenuSubsystem.h"
#include "GuardadoSubsystem.h"
#include "LocalizacionSubsystem.h"
#include "AudioAmbienteSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/Engine.h"

FString UMenuSubsystem::NombreCalidad() const
{
	static const TCHAR* N[] = { TEXT("Bajo"), TEXT("Medio"), TEXT("Alto"), TEXT("Épico"), TEXT("Cine") };
	return N[FMath::Clamp(CalidadActual, 0, 4)];
}

TArray<FString> UMenuSubsystem::Opciones() const
{
	const ULocalizacionSubsystem* L = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULocalizacionSubsystem>() : nullptr;
	auto Tx = [&](const TCHAR* K, const TCHAR* Def) { return L ? L->Texto(FName(K)) : FString(Def); };

	if (Pantalla == EPantallaMenu::Opciones)
	{
		const UAudioAmbienteSubsystem* Au = GetGameInstance() ? GetGameInstance()->GetSubsystem<UAudioAmbienteSubsystem>() : nullptr;
		const int32 VolPct = Au ? FMath::RoundToInt(Au->VolumenMaestro * 100.f) : 100;
		return {
			Tx(TEXT("ui.calidad"), TEXT("Calidad grafica")) + TEXT(":  ") + NombreCalidad(),
			Tx(TEXT("ui.idioma"), TEXT("Idioma")) + TEXT(":  ") + (L ? L->NombreIdioma() : TEXT("Castellano")),
			Tx(TEXT("ui.volumen"), TEXT("Volumen")) + FString::Printf(TEXT(":  %d%%"), VolPct),
			Tx(TEXT("ui.volver"), TEXT("Volver"))
		};
	}
	return {
		Tx(TEXT("ui.reanudar"), TEXT("Reanudar")),
		Tx(TEXT("ui.guardar"), TEXT("Guardar partida")),
		Tx(TEXT("ui.cargar"), TEXT("Cargar partida")),
		Tx(TEXT("ui.opciones"), TEXT("Opciones")),
		Tx(TEXT("ui.menu_principal"), TEXT("Menu principal")),
		Tx(TEXT("ui.salir"), TEXT("Salir del juego"))
	};
}

void UMenuSubsystem::AlternarPausa(APlayerController* PC)
{
	if (Pantalla == EPantallaMenu::Ninguno)
	{
		Pantalla = EPantallaMenu::Pausa; Seleccion = 0;
		if (PC) { UGameplayStatics::SetGamePaused(PC, true); PC->SetShowMouseCursor(true); }
	}
	else Cerrar(PC);
}

void UMenuSubsystem::Cerrar(APlayerController* PC)
{
	Pantalla = EPantallaMenu::Ninguno; Seleccion = 0;
	if (PC) { UGameplayStatics::SetGamePaused(PC, false); PC->SetShowMouseCursor(false); }
}

void UMenuSubsystem::Mover(int32 Dir)
{
	if (!Abierto()) return;
	const int32 n = Opciones().Num();
	Seleccion = ((Seleccion + Dir) % n + n) % n;
}

void UMenuSubsystem::AplicarCalidad()
{
	if (UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		S->SetOverallScalabilityLevel(FMath::Clamp(CalidadActual, 0, 4));
		S->ApplySettings(false);
	}
}

void UMenuSubsystem::Activar(APlayerController* PC)
{
	if (!Abierto()) return;

	if (Pantalla == EPantallaMenu::Opciones)
	{
		switch (Seleccion)
		{
		case 0: CalidadActual = (CalidadActual + 1) % 5; AplicarCalidad(); break;   // cicla calidad
		case 1: if (ULocalizacionSubsystem* L = GetGameInstance()->GetSubsystem<ULocalizacionSubsystem>()) L->CiclarIdioma(); break;  // idioma
		case 2: if (UAudioAmbienteSubsystem* Au = GetGameInstance()->GetSubsystem<UAudioAmbienteSubsystem>())   // volumen +25% (0..100)
					Au->VolumenMaestro = (Au->VolumenMaestro >= 0.99f) ? 0.f : FMath::Min(1.f, Au->VolumenMaestro + 0.25f); break;
		case 3: Pantalla = EPantallaMenu::Pausa; Seleccion = 0; break;              // volver
		}
		return;
	}

	// Pantalla de pausa
	UGuardadoSubsystem* G = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGuardadoSubsystem>() : nullptr;
	switch (Seleccion)
	{
	case 0: Cerrar(PC); break;                                   // Reanudar
	case 1: if (G) G->GuardarEnSlot(0); break;                   // Guardar
	case 2: if (G) G->CargarDeSlot(0); Cerrar(PC); break;        // Cargar
	case 3: Pantalla = EPantallaMenu::Opciones; Seleccion = 0; break;  // Opciones
	case 4: Cerrar(PC); UGameplayStatics::OpenLevel(PC, NivelMenuPrincipal); break;   // Menú principal
	case 5: if (PC) UKismetSystemLibrary::QuitGame(PC, PC, EQuitPreference::Quit, false); break;  // Salir
	}
}
