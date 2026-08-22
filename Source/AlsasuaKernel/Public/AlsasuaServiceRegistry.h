#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaServiceRegistry.generated.h"

/**
 * Tablón de anuncios entre pilares. Un pilar publica lo que ofrece con un nombre
 * y los demás lo piden por ese nombre, sin incluir su cabecera ni depender de su
 * módulo. Así se corta el acoplamiento: si el pilar no está cargado, el que
 * pregunta recibe nullptr y sigue, en vez de no compilar.
 */
UCLASS()
class ALSASUAKERNEL_API UAlsasuaServiceRegistry : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Kernel")
	void Publicar(FName Servicio, UObject* Implementacion);

	UFUNCTION(BlueprintCallable, Category = "Alsasua|Kernel")
	void Retirar(FName Servicio);

	UFUNCTION(BlueprintCallable, Category = "Alsasua|Kernel")
	UObject* Pedir(FName Servicio) const;

	UFUNCTION(BlueprintCallable, Category = "Alsasua|Kernel")
	TArray<FName> Listar() const;

	template <typename T>
	T* PedirComo(FName Servicio) const
	{
		return Cast<T>(Pedir(Servicio));
	}

	static UAlsasuaServiceRegistry* Get(const UObject* ContextoMundo);

	virtual void Deinitialize() override;

private:
	UPROPERTY()
	TMap<FName, TObjectPtr<UObject>> Servicios;
};
