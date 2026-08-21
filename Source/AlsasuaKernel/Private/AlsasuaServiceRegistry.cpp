#include "AlsasuaServiceRegistry.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

void UAlsasuaServiceRegistry::Publicar(FName Servicio, UObject* Implementacion)
{
	if (Servicio.IsNone() || !Implementacion)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Kernel] Publicar rechazado: servicio=%s implementacion=%s"),
			*Servicio.ToString(), Implementacion ? *Implementacion->GetName() : TEXT("null"));
		return;
	}

	// Reemplazar en vez de duplicar: en el editor un pilar se recarga varias veces
	// por sesión y la segunda publicación es la buena.
	Servicios.Add(Servicio, Implementacion);
	UE_LOG(LogTemp, Log, TEXT("[Kernel] Publicado %s -> %s"), *Servicio.ToString(), *Implementacion->GetName());
}

void UAlsasuaServiceRegistry::Retirar(FName Servicio)
{
	if (Servicios.Remove(Servicio) > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[Kernel] Retirado %s"), *Servicio.ToString());
	}
}

UObject* UAlsasuaServiceRegistry::Pedir(FName Servicio) const
{
	const TObjectPtr<UObject>* Encontrado = Servicios.Find(Servicio);
	// El pilar puede haberse descargado dejando la entrada colgando.
	return (Encontrado && IsValid(*Encontrado)) ? Encontrado->Get() : nullptr;
}

TArray<FName> UAlsasuaServiceRegistry::Listar() const
{
	TArray<FName> Nombres;
	Servicios.GetKeys(Nombres);
	Nombres.Sort(FNameLexicalLess());
	return Nombres;
}

UAlsasuaServiceRegistry* UAlsasuaServiceRegistry::Get(const UObject* ContextoMundo)
{
	const UWorld* Mundo = GEngine ? GEngine->GetWorldFromContextObject(ContextoMundo, EGetWorldErrorMode::ReturnNull) : nullptr;
	UGameInstance* Instancia = Mundo ? Mundo->GetGameInstance() : nullptr;
	return Instancia ? Instancia->GetSubsystem<UAlsasuaServiceRegistry>() : nullptr;
}

void UAlsasuaServiceRegistry::Deinitialize()
{
	Servicios.Empty();
	Super::Deinitialize();
}

static FAutoConsoleCommandWithWorld ListarPilaresCommand(
	TEXT("Alsasua.Pilar.Listar"),
	TEXT("Lista los servicios publicados en el registro del kernel"),
	FConsoleCommandWithWorldDelegate::CreateStatic([](UWorld* Mundo)
	{
		UGameInstance* Instancia = Mundo ? Mundo->GetGameInstance() : nullptr;
		UAlsasuaServiceRegistry* Registro = Instancia ? Instancia->GetSubsystem<UAlsasuaServiceRegistry>() : nullptr;
		if (!Registro)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Kernel] sin registro"));
			return;
		}

		const TArray<FName> Nombres = Registro->Listar();
		UE_LOG(LogTemp, Log, TEXT("[Kernel] %d servicios publicados"), Nombres.Num());
		for (const FName& Nombre : Nombres)
		{
			UE_LOG(LogTemp, Log, TEXT("[Kernel]   %s"), *Nombre.ToString());
		}
	}));
