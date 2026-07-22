// LocalizacionSubsystem.h (capa GAMEPLAY)
// Textos por idioma (castellano / euskera). Puerto de SistemaLocalizacion.
// Texto(clave) devuelve la cadena en el idioma actual. Otros sistemas (menú, HUD)
// pueden pedir sus textos por clave.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LocalizacionSubsystem.generated.h"

UENUM(BlueprintType)
enum class EIdioma : uint8 { Castellano, Euskera };

USTRUCT()
struct FTextoLoc { GENERATED_BODY() FString ES; FString EU; };

UCLASS()
class ALSASUAGAMEPLAY_API ULocalizacionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category="Idioma") EIdioma Idioma = EIdioma::Castellano;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category="Idioma") FString Texto(FName Clave) const;
	UFUNCTION(BlueprintCallable, Category="Idioma") void SetIdioma(EIdioma I) { Idioma = I; }
	UFUNCTION(BlueprintCallable, Category="Idioma") void CiclarIdioma() { Idioma = (Idioma == EIdioma::Castellano) ? EIdioma::Euskera : EIdioma::Castellano; }
	UFUNCTION(BlueprintCallable, Category="Idioma") FString NombreIdioma() const { return Idioma == EIdioma::Euskera ? TEXT("Euskera") : TEXT("Castellano"); }

private:
	TMap<FName, FTextoLoc> Tabla;
	void Sembrar();
};
