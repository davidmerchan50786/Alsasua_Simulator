// NegocioActor.cpp
#include "NegocioActor.h"
#include "EconomiaCriminalSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

ANegocioActor::ANegocioActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

int32 ANegocioActor::IngresoMin() const
{
	switch (Tipo)
	{
	case ETipoNegocio::Bar:       return 30;
	case ETipoNegocio::Comercio:  return 50;
	case ETipoNegocio::Empresa:   return 120;
	case ETipoNegocio::Industria: return 300;
	default:                      return 30;
	}
}

static UEconomiaCriminalSubsystem* CrimenDe(const AActor* A)
{
	if (const UWorld* W = A ? A->GetWorld() : nullptr)
		if (UGameInstance* GI = W->GetGameInstance())
			return GI->GetSubsystem<UEconomiaCriminalSubsystem>();
	return nullptr;
}

void ANegocioActor::BeginPlay()
{
	Super::BeginPlay();
	if (UEconomiaCriminalSubsystem* C = CrimenDe(this)) C->Registrar(this);
}

void ANegocioActor::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UEconomiaCriminalSubsystem* C = CrimenDe(this)) C->Quitar(this);
	Super::EndPlay(Reason);
}

void ANegocioActor::Interactuar()
{
	if (Estado != EEstadoNegocio::Libre) return;
	if (UEconomiaCriminalSubsystem* C = CrimenDe(this)) C->Extorsionar(this);
}
