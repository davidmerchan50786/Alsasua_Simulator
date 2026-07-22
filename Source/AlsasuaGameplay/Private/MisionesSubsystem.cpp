// MisionesSubsystem.cpp
#include "MisionesSubsystem.h"
#include "DialogoSubsystem.h"
#include "DialogoTipos.h"
#include "EconomiaSubsystem.h"
#include "ApoyoPopularSubsystem.h"
#include "ArranqueMundo.h"
#include "GeoDataAlsasua.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

void UMisionesSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ConstruirMisionesDemo();   // M00 por defecto si nadie registra misiones
}

void UMisionesSubsystem::Tick(float DeltaTime)
{
	// Auto-arranque al estar el mundo mínimo listo (como SistemaMisiones en Unity).
	if (!bArrancada && ArranqueMundo::BaselineListo)
	{
		bArrancada = true;
		if (!bSaltarIntro) IniciarMision(PrimeraMision);
	}

	if (HayMision() && Actual->Id == TEXT("M00")) EngancheDemoM00();
}

// Completa los objetivos de M00 por movimiento y por llegar a la plaza.
void UMisionesSubsystem::EngancheDemoM00()
{
	UGameInstance* GI = GetGameInstance();
	UWorld* W = GI ? GI->GetWorld() : nullptr;
	if (!W) return;
	APawn* Jug = UGameplayStatics::GetPlayerPawn(W, 0);
	if (!Jug) return;

	const FVector P = Jug->GetActorLocation();
	if (!bPosInicial) { PosInicial = P; bPosInicial = true; }

	// "mover": haberse alejado >5 m del punto inicial.
	if (FVector::Dist(P, PosInicial) > 500.f) CompletarObjetivo(TEXT("mover"));

	// "plaza": estar a <30 m de Herriko Plaza (XY).
	const FVector Plaza = UAlsasuaGeoData::HerrikoPlaza();
	if (FVector2D::Distance(FVector2D(P), FVector2D(Plaza)) < 3000.f) CompletarObjetivo(TEXT("plaza"));
}

void UMisionesSubsystem::RegistrarMision(UMisionDef* Def)
{
	if (Def && !Def->Id.IsNone()) Registro.Add(Def->Id, Def);
}

bool UMisionesSubsystem::IniciarMision(FName Id)
{
	UMisionDef** F = Registro.Find(Id);
	if (!F || !*F) { UE_LOG(LogTemp, Warning, TEXT("[Misiones] no existe %s"), *Id.ToString()); return false; }

	Actual = *F;
	Estado = EEstadoMision::Activa;
	ObjetivosActivos = Actual->Objetivos;          // copia con Progreso=0
	for (FObjetivoMision& O : ObjetivosActivos) O.Progreso = 0;

	OnMisionIniciada.Broadcast(Actual->Id, Actual->Titulo);
	OnObjetivosCambian.Broadcast();

	if (Actual->DialogoInicio)
		if (UGameInstance* GI = GetGameInstance())
			if (UDialogoSubsystem* Di = GI->GetSubsystem<UDialogoSubsystem>())
				Di->Iniciar(Actual->DialogoInicio);

	// Convoca manifestación si la misión lo pide.
	if (Actual->bConvocaManifestacion)
		if (UGameInstance* GI = GetGameInstance())
			if (UManifestacionSubsystem* Mf = GI->GetSubsystem<UManifestacionSubsystem>())
			{
				if (!bSuscritoManifa) { Mf->OnEstado.AddDynamic(this, &UMisionesSubsystem::OnManifestacionEstado); bSuscritoManifa = true; }
				const TArray<FVector>& R = Actual->RutaManifestacion;
				const FVector Punto = R.Num() > 0 ? R[0] : (GI->GetWorld() && GI->GetWorld()->GetFirstPlayerController() && GI->GetWorld()->GetFirstPlayerController()->GetPawn()
					? GI->GetWorld()->GetFirstPlayerController()->GetPawn()->GetActorLocation() : FVector::ZeroVector);
				Mf->Convocar(Punto, R);
			}

	// Waypoint del objetivo: plaza en M00, cabeza de marcha en las que convocan.
	if (Actual->Id == TEXT("M00")) { MarcadorMundo = UAlsasuaGeoData::HerrikoPlaza(); bMarcadorActivo = true; }
	else if (Actual->RutaManifestacion.Num() > 0) { MarcadorMundo = Actual->RutaManifestacion[0]; bMarcadorActivo = true; }
	else bMarcadorActivo = false;

	UE_LOG(LogTemp, Log, TEXT("[Misiones] iniciada %s: %s"), *Actual->Id.ToString(), *Actual->Titulo);
	return true;
}

void UMisionesSubsystem::OnManifestacionEstado(EEstadoManifestacion E)
{
	// La protesta terminó (volvió a Inactiva): completa el objetivo "manifestacion".
	if (E == EEstadoManifestacion::Inactiva && Estado == EEstadoMision::Activa)
		CompletarObjetivo(TEXT("manifestacion"));
}

void UMisionesSubsystem::AvanzarObjetivo(FName ObjetivoId, int32 Cantidad)
{
	if (Estado != EEstadoMision::Activa) return;
	for (FObjetivoMision& O : ObjetivosActivos)
		if (O.Id == ObjetivoId)
		{
			O.Progreso = FMath::Clamp(O.Progreso + Cantidad, 0, O.Meta);
			OnObjetivosCambian.Broadcast();
			ComprobarFin();
			return;
		}
}

void UMisionesSubsystem::CompletarObjetivo(FName ObjetivoId)
{
	if (Estado != EEstadoMision::Activa) return;
	for (FObjetivoMision& O : ObjetivosActivos)
		if (O.Id == ObjetivoId) { O.Progreso = O.Meta; OnObjetivosCambian.Broadcast(); ComprobarFin(); return; }
}

void UMisionesSubsystem::ComprobarFin()
{
	for (const FObjetivoMision& O : ObjetivosActivos)
		if (!O.bOpcional && !O.Completado()) return;   // queda algo obligatorio
	CompletarMision();
}

void UMisionesSubsystem::CompletarMision()
{
	if (!Actual) return;
	Estado = EEstadoMision::Completada;
	bMarcadorActivo = false;   // el siguiente IniciarMision pondrá el suyo

	if (UGameInstance* GI = GetGameInstance())
	{
		if (Actual->RecompensaDinero != 0)
			if (UEconomiaSubsystem* Eco = GI->GetSubsystem<UEconomiaSubsystem>())
				Eco->GanarDinero(Actual->RecompensaDinero);
		if (!FMath::IsNearlyZero(Actual->RecompensaApoyo))
			if (UApoyoPopularSubsystem* Ap = GI->GetSubsystem<UApoyoPopularSubsystem>())
				Ap->SumarApoyo(Actual->RecompensaApoyo, TEXT("mision"));
	}

	const FName Sig = Actual->Siguiente;
	const FName Hecha = Actual->Id;
	OnMisionCompletada.Broadcast(Hecha);
	UE_LOG(LogTemp, Log, TEXT("[Misiones] completada %s"), *Hecha.ToString());

	Actual = nullptr;
	Estado = EEstadoMision::Inactiva;
	if (!Sig.IsNone() && Registro.Contains(Sig)) IniciarMision(Sig);   // encadena M00->M12
}

TArray<FString> UMisionesSubsystem::ObjetivosTexto() const
{
	TArray<FString> R;
	for (const FObjetivoMision& O : ObjetivosActivos)
	{
		const TCHAR* Marca = O.Completado() ? TEXT("[x]") : TEXT("[ ]");
		if (O.Meta > 1) R.Add(FString::Printf(TEXT("%s %s (%d/%d)"), Marca, *O.Descripcion, O.Progreso, O.Meta));
		else            R.Add(FString::Printf(TEXT("%s %s"), Marca, *O.Descripcion));
	}
	return R;
}

// --- Contenido por defecto: M00 "Esnatu, Altsasu" (tutorial) ---
void UMisionesSubsystem::ConstruirMisionesDemo()
{
	if (Registro.Num() > 0) return;

	// Conversación de inicio (ficción neutra, tono tutorial).
	UConversacionDialogo* Conv = NewObject<UConversacionDialogo>(this);
	Conv->Inicio = TEXT("n0");
	{
		FNodoDialogo n0; n0.Id = TEXT("n0"); n0.Hablante = TEXT("Kepa");
		n0.Texto = TEXT("Esnatu, Altsasu. Despierta. Hoy empieza algo grande para el pueblo.");
		n0.Auto = TEXT("n1");
		FNodoDialogo n1; n1.Id = TEXT("n1"); n1.Hablante = TEXT("Kepa");
		n1.Texto = TEXT("Date una vuelta para soltar las piernas y nos vemos en la Herriko Plaza.");
		FOpcionDialogo o; o.Texto = TEXT("Voy para alla"); o.Destino = NAME_None; o.DeltaApoyo = 2.f;
		n1.Opciones.Add(o);
		Conv->Nodos.Add(n0); Conv->Nodos.Add(n1);
	}

	UMisionDef* M00 = NewObject<UMisionDef>(this);
	M00->Id = TEXT("M00");
	M00->Titulo = TEXT("Esnatu, Altsasu");
	M00->Descripcion = TEXT("Tutorial: aprende a moverte por Altsasu y reúnete con tu contacto.");
	M00->DialogoInicio = Conv;
	M00->RecompensaApoyo = 3.f;
	M00->RecompensaDinero = 50;
	M00->Siguiente = TEXT("M01");   // encadena con la primera manifa
	{
		FObjetivoMision o1; o1.Id = TEXT("mover"); o1.Descripcion = TEXT("Muevete por Altsasu"); o1.Meta = 1;
		FObjetivoMision o2; o2.Id = TEXT("plaza"); o2.Descripcion = TEXT("Llega a la Herriko Plaza"); o2.Meta = 1;
		M00->Objetivos.Add(o1); M00->Objetivos.Add(o2);
	}
	RegistrarMision(M00);

	// --- M01 "La primera manifa": convoca una marcha desde la plaza ---
	UMisionDef* M01 = NewObject<UMisionDef>(this);
	M01->Id = TEXT("M01");
	M01->Titulo = TEXT("La primera manifa");
	M01->Descripcion = TEXT("Convoca una marcha en la Herriko Plaza y acompanala hasta el final.");
	M01->bConvocaManifestacion = true;
	M01->RecompensaApoyo = 8.f;
	M01->Siguiente = NAME_None;
	{
		const FVector Plaza = UAlsasuaGeoData::HerrikoPlaza();
		M01->RutaManifestacion = {
			Plaza,
			Plaza + FVector(12000.f, 0.f, 0.f),    // 120 m
			Plaza + FVector(20000.f, 8000.f, 0.f)  // gira y termina
		};
		FObjetivoMision o; o.Id = TEXT("manifestacion"); o.Descripcion = TEXT("Acompana la marcha hasta el final"); o.Meta = 1;
		M01->Objetivos.Add(o);
	}
	RegistrarMision(M01);
}
