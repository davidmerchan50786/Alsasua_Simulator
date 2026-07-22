// MenuPrincipalController.cpp
#include "MenuPrincipalController.h"
#include "GuardadoSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/GameInstance.h"

void AMenuPrincipalController::BeginPlay()
{
	Super::BeginPlay();
	bShowMouseCursor = true;
	FInputModeGameAndUI Modo;
	SetInputMode(Modo);
}

TArray<FString> AMenuPrincipalController::Opciones() const
{
	const bool bHay = GetGameInstance() && GetGameInstance()->GetSubsystem<UGuardadoSubsystem>()
		&& GetGameInstance()->GetSubsystem<UGuardadoSubsystem>()->ExisteGuardado(0);
	return { TEXT("Nueva partida"), bHay ? TEXT("Continuar") : TEXT("Continuar  (sin partida)"), TEXT("Salir") };
}

void AMenuPrincipalController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!InputComponent) return;
	InputComponent->BindKey(EKeys::Up,    IE_Pressed, this, &AMenuPrincipalController::Arriba);
	InputComponent->BindKey(EKeys::Down,  IE_Pressed, this, &AMenuPrincipalController::Abajo);
	InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &AMenuPrincipalController::Activar);
}

void AMenuPrincipalController::Arriba() { const int32 n = Opciones().Num(); Seleccion = ((Seleccion - 1) % n + n) % n; }
void AMenuPrincipalController::Abajo()  { const int32 n = Opciones().Num(); Seleccion = (Seleccion + 1) % n; }

void AMenuPrincipalController::Activar()
{
	UGuardadoSubsystem* G = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGuardadoSubsystem>() : nullptr;
	switch (Seleccion)
	{
	case 0:   // Nueva partida
		if (G) G->bCargarAlArrancar = false;
		UGameplayStatics::OpenLevel(this, NivelJuego);
		break;
	case 1:   // Continuar
		if (G && G->ExisteGuardado(0)) { G->bCargarAlArrancar = true; UGameplayStatics::OpenLevel(this, NivelJuego); }
		break;
	case 2:   // Salir
		UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
		break;
	}
}
