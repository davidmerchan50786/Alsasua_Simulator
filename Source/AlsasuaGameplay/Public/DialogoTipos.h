// DialogoTipos.h (capa GAMEPLAY)
// Datos de diálogo ramificado. Puerto de SistemaDialogo (NodoDialogo /
// OpcionDialogo / ConversacionDialogo) de Unity. UConversacionDialogo es un
// DataAsset para poder autorar conversaciones en el editor o en C++.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DialogoTipos.generated.h"

USTRUCT(BlueprintType)
struct FOpcionDialogo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Texto;
	// Nodo destino al elegir esta opción. NAME_None = termina la conversación.
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Destino;
	// Efectos opcionales sobre apoyo popular al elegir (puede ser negativo).
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float DeltaApoyo = 0.f;
};

USTRUCT(BlueprintType)
struct FNodoDialogo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Hablante;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(MultiLine=true)) FString Texto;
	// Si vacío -> continúa por Auto (o termina). Si hay opciones, el jugador elige.
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FOpcionDialogo> Opciones;
	// Nodo siguiente cuando no hay opciones (línea automática). NAME_None = fin.
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Auto;
};

UCLASS(BlueprintType)
class ALSASUAGAMEPLAY_API UConversacionDialogo : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Inicio;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FNodoDialogo> Nodos;

	const FNodoDialogo* BuscarNodo(FName Id) const
	{
		for (const FNodoDialogo& N : Nodos) if (N.Id == Id) return &N;
		return nullptr;
	}
};
