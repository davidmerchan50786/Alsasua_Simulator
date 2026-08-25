// MisionTipos.h (capa GAMEPLAY)
// Datos de misión. Puerto de SistemaMisiones (cadena M00->M12). UMisionDef es
// un DataAsset para autorar misiones en editor o en C++.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MisionTipos.generated.h"

class UConversacionDialogo;

UENUM(BlueprintType)
enum class EEstadoMision : uint8 { Inactiva, Activa, Completada, Fallada };

USTRUCT(BlueprintType)
struct FObjetivoMision
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Descripcion;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Meta = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bOpcional = false;
	UPROPERTY(BlueprintReadOnly) int32 Progreso = 0;   // runtime

	bool Completado() const { return Progreso >= Meta; }
};

UCLASS(BlueprintType)
class ALSASUAGAMEPLAY_API UMisionDef : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Titulo;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(MultiLine=true)) FString Descripcion;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FObjetivoMision> Objetivos;

	// Conversación que se lanza al iniciar la misión (opcional).
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UConversacionDialogo* DialogoInicio = nullptr;

	// Misión que se inicia al completar ésta (encadena M00->M12). NAME_None = fin.
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Siguiente;

	// Recompensas al completar.
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 RecompensaDinero = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float RecompensaApoyo  = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float RecompensaNivelBusqueda = 0.f;

	// Requisitos para aceptar la misión.
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName RequiereFaccion;   // NAME_None = ninguna
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> MisionesRequeridas;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float LimiteTiempo = 0.f;  // 0 = sin límite
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Dificultad = 1;

	// Si true, al iniciar convoca una manifestación. La ruta (mundo, cm) define la
	// marcha; el objetivo "manifestacion" se completa cuando la protesta termina.
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bConvocaManifestacion = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FVector> RutaManifestacion;
};
