// AlsasuaPlayerController.cpp
#include "AlsasuaLegacyPlayerController.h"
#include "ArmasComponent.h"
#include "DrogasSubsystem.h"
#include "DisfrazSubsystem.h"
#include "DialogoSubsystem.h"
#include "ManifestacionSubsystem.h"
#include "GuardadoSubsystem.h"
#include "MenuSubsystem.h"
#include "VehiculoJugable.h"
#include "VehiculoAmbiente.h"
#include "WantedSubsystem.h"
#include "ApoyoPopularSubsystem.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "RespawnSubsystem.h"
#include "AlsasuaTypes.h"
#include "ArranqueMundo.h"
#include "Kismet/GameplayStatics.h"

static UDrogasSubsystem* DrogasDe(const APlayerController* PC)
{
	return (PC && PC->GetGameInstance()) ? PC->GetGameInstance()->GetSubsystem<UDrogasSubsystem>() : nullptr;
}

static UDialogoSubsystem* DialogoDe(const APlayerController* PC)
{
	return (PC && PC->GetGameInstance()) ? PC->GetGameInstance()->GetSubsystem<UDialogoSubsystem>() : nullptr;
}

// Si hay conversación con opciones, enruta la tecla N a esa opción y devuelve true.
static bool RutaOpcionDialogo(const APlayerController* PC, int32 Indice)
{
	UDialogoSubsystem* D = DialogoDe(PC);
	if (D && D->EnCurso() && D->OpcionesActuales().Num() > Indice)
	{
		D->Elegir(Indice);
		return true;
	}
	return false;
}

void AAlsasuaLegacyPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (!InPawn) return;

	// Añadir el componente de armas al pawn (glue de la capa Gameplay).
	Armas = NewObject<UArmasComponent>(InPawn, TEXT("Armas"));
	Armas->RegisterComponent();
	Armas->RecogerArma(ETipoArma::Pistola, 30);   // munición de salida para probar
}

void AAlsasuaLegacyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!InputComponent) return;

	// Input clásico por tecla (sin necesidad de Input Actions en el editor).
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AAlsasuaLegacyPlayerController::OnDisparar);
	InputComponent->BindKey(EKeys::One,   IE_Pressed, this, &AAlsasuaLegacyPlayerController::EquiparPunos);
	InputComponent->BindKey(EKeys::Two,   IE_Pressed, this, &AAlsasuaLegacyPlayerController::EquiparPistola);
	InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AAlsasuaLegacyPlayerController::EquiparEscopeta);
	InputComponent->BindKey(EKeys::Four,  IE_Pressed, this, &AAlsasuaLegacyPlayerController::EquiparFusil);
	InputComponent->BindKey(EKeys::Five,  IE_Pressed, this, &AAlsasuaLegacyPlayerController::TomarPorro);
	InputComponent->BindKey(EKeys::Six,   IE_Pressed, this, &AAlsasuaLegacyPlayerController::TomarSpeed);
	InputComponent->BindKey(EKeys::Seven, IE_Pressed, this, &AAlsasuaLegacyPlayerController::TomarChute);
	InputComponent->BindKey(EKeys::Eight, IE_Pressed, this, &AAlsasuaLegacyPlayerController::TomarTripi);
	InputComponent->BindKey(EKeys::H,     IE_Pressed, this, &AAlsasuaLegacyPlayerController::AlternarDisfraz);
	InputComponent->BindKey(EKeys::E,     IE_Pressed, this, &AAlsasuaLegacyPlayerController::OnInteractuar);
	InputComponent->BindKey(EKeys::M,     IE_Pressed, this, &AAlsasuaLegacyPlayerController::OnConvocarManifestacion);
	InputComponent->BindKey(EKeys::F5,    IE_Pressed, this, &AAlsasuaLegacyPlayerController::OnGuardar);
	InputComponent->BindKey(EKeys::F9,    IE_Pressed, this, &AAlsasuaLegacyPlayerController::OnCargar);

	// Menú de pausa: deben ejecutarse aunque el juego esté pausado.
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AAlsasuaLegacyPlayerController::OnMenu).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::Up,     IE_Pressed, this, &AAlsasuaLegacyPlayerController::OnMenuArriba).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::Down,   IE_Pressed, this, &AAlsasuaLegacyPlayerController::OnMenuAbajo).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::Enter,  IE_Pressed, this, &AAlsasuaLegacyPlayerController::OnMenuActivar).bExecuteWhenPaused = true;

	// --- Mando (gamepad) ---
	InputComponent->BindKey(EKeys::Gamepad_RightTrigger,     IE_Pressed, this, &AAlsasuaLegacyPlayerController::OnDisparar);
	InputComponent->BindKey(EKeys::Gamepad_FaceButton_Top,   IE_Pressed, this, &AAlsasuaLegacyPlayerController::OnInteractuar);
	InputComponent->BindKey(EKeys::Gamepad_LeftThumbstick,   IE_Pressed, this, &AAlsasuaLegacyPlayerController::AlternarDisfraz);
	InputComponent->BindKey(EKeys::Gamepad_Special_Right,    IE_Pressed, this, &AAlsasuaLegacyPlayerController::OnMenu).bExecuteWhenPaused = true;   // Start
	InputComponent->BindKey(EKeys::Gamepad_DPad_Up,          IE_Pressed, this, &AAlsasuaLegacyPlayerController::OnMenuArriba).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::Gamepad_DPad_Down,        IE_Pressed, this, &AAlsasuaLegacyPlayerController::OnMenuAbajo).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::Gamepad_FaceButton_Bottom,IE_Pressed, this, &AAlsasuaLegacyPlayerController::OnMenuActivar).bExecuteWhenPaused = true;
}

static UMenuSubsystem* MenuDe(const APlayerController* PC)
{
	return (PC && PC->GetGameInstance()) ? PC->GetGameInstance()->GetSubsystem<UMenuSubsystem>() : nullptr;
}

void AAlsasuaLegacyPlayerController::OnMenu()        { if (UMenuSubsystem* M = MenuDe(this)) M->AlternarPausa(this); }
void AAlsasuaLegacyPlayerController::OnMenuArriba()  { if (UMenuSubsystem* M = MenuDe(this)) M->Mover(-1); }
void AAlsasuaLegacyPlayerController::OnMenuAbajo()   { if (UMenuSubsystem* M = MenuDe(this)) M->Mover(+1); }
void AAlsasuaLegacyPlayerController::OnMenuActivar() { if (UMenuSubsystem* M = MenuDe(this)) M->Activar(this); }

void AAlsasuaLegacyPlayerController::OnGuardar()
{
	if (UGuardadoSubsystem* G = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGuardadoSubsystem>() : nullptr)
		G->GuardarEnSlot(0);
}

void AAlsasuaLegacyPlayerController::OnCargar()
{
	if (UGuardadoSubsystem* G = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGuardadoSubsystem>() : nullptr)
		G->CargarDeSlot(0);
}

void AAlsasuaLegacyPlayerController::OnConvocarManifestacion()
{
	UManifestacionSubsystem* Mf = GetGameInstance() ? GetGameInstance()->GetSubsystem<UManifestacionSubsystem>() : nullptr;
	if (!Mf || Mf->Activa()) return;
	if (const APawn* P = GetPawn())
		Mf->Convocar(P->GetActorLocation(), TArray<FVector>());   // mitin estático donde estás
}

void AAlsasuaLegacyPlayerController::OnInteractuar()
{
	// Si voy conduciendo: bajarme.
	if (AVehiculoJugable* V = Cast<AVehiculoJugable>(GetPawn())) { SalirVehiculo(V); return; }

	// En diálogo: E avanza la línea automática (o cierra si no hay opciones).
	if (UDialogoSubsystem* D = DialogoDe(this))
		if (D->EnCurso()) { D->Elegir(-1); return; }

	// A pie: subirme a un coche cercano.
	EntrarVehiculoCercano();
}

void AAlsasuaLegacyPlayerController::EntrarVehiculoCercano()
{
	APawn* Yo = GetPawn();
	UWorld* W = GetWorld();
	if (!Yo || !W) return;
	const FVector P = Yo->GetActorLocation();
	const float Alcance = 450.f;   // 4.5 m

	// Coche conducible ya colocado más cercano.
	AVehiculoJugable* Conducible = nullptr; float dConducible = Alcance;
	for (TActorIterator<AVehiculoJugable> It(W); It; ++It)
	{ const float d = FVector::Dist(It->GetActorLocation(), P); if (d < dConducible) { dConducible = d; Conducible = *It; } }

	// Coche del tráfico más cercano (para robarlo).
	AVehiculoAmbiente* Ambiente = nullptr; float dAmbiente = Alcance;
	for (TActorIterator<AVehiculoAmbiente> It(W); It; ++It)
	{ const float d = FVector::Dist(It->GetActorLocation(), P); if (d < dAmbiente) { dAmbiente = d; Ambiente = *It; } }

	if (Ambiente && dAmbiente <= dConducible)
	{
		// Carjacking: sustituye el coche de tráfico por uno conducible y delito.
		FActorSpawnParameters SP; SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AVehiculoJugable* Nuevo = W->SpawnActor<AVehiculoJugable>(AVehiculoJugable::StaticClass(),
			Ambiente->GetActorLocation(), Ambiente->GetActorRotation(), SP);
		Ambiente->Destroy();
		if (Nuevo)
		{
			if (UWantedSubsystem* Wn = GetGameInstance()->GetSubsystem<UWantedSubsystem>()) Wn->AumentarBusqueda(2);
			if (UApoyoPopularSubsystem* Ap = GetGameInstance()->GetSubsystem<UApoyoPopularSubsystem>()) Ap->RestarApoyo(2.f, TEXT("robo coche"));
			EntrarEn(Nuevo);
		}
		return;
	}
	if (Conducible) EntrarEn(Conducible);
}

void AAlsasuaLegacyPlayerController::EntrarEn(AVehiculoJugable* V)
{
	if (!V) return;
	PersonajePawn = GetPawn();
	if (PersonajePawn) { PersonajePawn->SetActorHiddenInGame(true); PersonajePawn->SetActorEnableCollision(false); }
	Possess(V);
}

void AAlsasuaLegacyPlayerController::SalirVehiculo(AVehiculoJugable* V)
{
	if (!PersonajePawn) return;
	// Coloca al personaje al lado del coche.
	const FVector Lado = V->GetActorRightVector() * 220.f + FVector(0, 0, 30.f);
	PersonajePawn->SetActorLocation(V->GetActorLocation() + Lado);
	PersonajePawn->SetActorHiddenInGame(false);
	PersonajePawn->SetActorEnableCollision(true);
	Possess(PersonajePawn);
	PersonajePawn = nullptr;
}

void AAlsasuaLegacyPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	GestionarCongelado();
	ComprobarMuerte(DeltaTime);
}

void AAlsasuaLegacyPlayerController::GestionarCongelado()
{
	// Congela el movimiento hasta que el director marque el baseline listo.
	const bool QuiereCongelar = !ArranqueMundo::BaselineListo;
	if (QuiereCongelar && !bCongeladoArranque)      { SetIgnoreMoveInput(true);  bCongeladoArranque = true; }
	else if (!QuiereCongelar && bCongeladoArranque)
	{
		SetIgnoreMoveInput(false); bCongeladoArranque = false;
		// "Continuar" del menú principal: cargar el slot al estar el mundo listo.
		if (UGuardadoSubsystem* G = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGuardadoSubsystem>() : nullptr)
			if (G->bCargarAlArrancar) { G->bCargarAlArrancar = false; G->CargarDeSlot(0); }
	}
}

void AAlsasuaLegacyPlayerController::ComprobarMuerte(float DeltaTime)
{
	IDamageable* D = Cast<IDamageable>(GetPawn());
	if (!bEsperandoRespawn)
	{
		if (D && D->EstaMuerto()) { bEsperandoRespawn = true; TimerRespawn = RetardoRespawn; }
		return;
	}

	TimerRespawn -= DeltaTime;
	if (TimerRespawn > 0.f) return;

	// reaparecer en el último piso franco (o donde esté si no hay)
	if (UGameInstance* GI = GetGameInstance())
	{
		if (URespawnSubsystem* R = GI->GetSubsystem<URespawnSubsystem>())
			R->Reaparecer(GetPawn());
		if (UWantedSubsystem* Wn = GI->GetSubsystem<UWantedSubsystem>())
			Wn->AumentarBusqueda(-5);   // al reaparecer se pierde el calor
	}
	if (IDamageable* D2 = Cast<IDamageable>(GetPawn()))
		D2->Curar(D2->GetVidaMax());   // por si no había punto de respawn
	bEsperandoRespawn = false;
}

void AAlsasuaLegacyPlayerController::OnDisparar()      { if (Armas) Armas->UsarArma(); }
// En diálogo, las teclas 1-4 eligen opción; si no, equipan arma.
void AAlsasuaLegacyPlayerController::EquiparPunos()    { if (RutaOpcionDialogo(this, 0)) return; if (Armas) Armas->CambiarArma(ETipoArma::Punos); }
void AAlsasuaLegacyPlayerController::EquiparPistola()  { if (RutaOpcionDialogo(this, 1)) return; if (Armas) Armas->CambiarArma(ETipoArma::Pistola); }
void AAlsasuaLegacyPlayerController::EquiparEscopeta() { if (RutaOpcionDialogo(this, 2)) return; if (Armas) Armas->CambiarArma(ETipoArma::Escopeta); }
void AAlsasuaLegacyPlayerController::EquiparFusil()    { if (RutaOpcionDialogo(this, 3)) return; if (Armas) Armas->CambiarArma(ETipoArma::Fusil); }

void AAlsasuaLegacyPlayerController::TomarPorro() { if (UDrogasSubsystem* D = DrogasDe(this)) D->Tomar(ESustancia::Porro); }
void AAlsasuaLegacyPlayerController::TomarSpeed() { if (UDrogasSubsystem* D = DrogasDe(this)) D->Tomar(ESustancia::Speed); }
void AAlsasuaLegacyPlayerController::TomarChute() { if (UDrogasSubsystem* D = DrogasDe(this)) D->Tomar(ESustancia::Chute); }
void AAlsasuaLegacyPlayerController::TomarTripi() { if (UDrogasSubsystem* D = DrogasDe(this)) D->Tomar(ESustancia::Tripi); }

void AAlsasuaLegacyPlayerController::AlternarDisfraz()
{
	if (GetGameInstance())
		if (UDisfrazSubsystem* Dis = GetGameInstance()->GetSubsystem<UDisfrazSubsystem>())
			Dis->Alternar();
}

void AAlsasuaLegacyPlayerController::SetMouseSensitivity(float X, float Y)
{
	MouseSensitivityX = FMath::Max(0.05f, X);
	MouseSensitivityY = FMath::Max(0.05f, Y);
}

void AAlsasuaLegacyPlayerController::TogglePause()
{
	bJuegoEnPausa = !bJuegoEnPausa;
	UGameplayStatics::SetGamePaused(this, bJuegoEnPausa);
}
