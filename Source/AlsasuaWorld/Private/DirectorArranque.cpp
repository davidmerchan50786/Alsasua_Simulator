#include "DirectorArranque.h"
#include "ArranqueMundo.h"
#include "TerrenoGenerado.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

void ADirectorArranque::BeginPlay()
{
    Super::BeginPlay();
    IniciarConstruccion();
}

void ADirectorArranque::IniciarConstruccion()
{
    ArranqueMundo::HayDirector = true;
    ArranqueMundo::BaselineListo = false;
    ArranqueMundo::Progreso = 0.f;

    UE_LOG(LogTemp, Log, TEXT("DirectorArranque: Iniciando construccion de Alsasua..."));

    UWorld* World = GetWorld();
    if (!World) return;

    ATerrenoGenerado* Terreno = World->SpawnActor<ATerrenoGenerado>(
        ATerrenoGenerado::StaticClass(),
        FVector::ZeroVector, FRotator::ZeroRotator);
    if (Terreno)
    {
        Terreno->SetActorLabel(TEXT("Alsasua_TerrenoProcedural"));
        UE_LOG(LogTemp, Log, TEXT("DirectorArranque: Terreno procedural spawneado."));
    }

    ArranqueMundo::Progreso = 0.3f;
    ArranqueMundo::BaselineListo = true;
    ArranqueMundo::HayDirector = false;
    UE_LOG(LogTemp, Log, TEXT("DirectorArranque: BaselineListo = true (terreno listo)."));
}
