// MisionesSubsystem.h (capa GAMEPLAY)
// Gestiona la cadena de misiones M00->M12: registro, misión activa, progreso de
// objetivos, recompensas y encadenado. Auto-lanza la misión inicial al estar el
// mundo listo (flag bSaltarIntro). Puerto de SistemaMisiones.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "MisionTipos.h"
#include "ManifestacionSubsystem.h"   // EEstadoManifestacion
#include "MisionesSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMisionIniciada, FName, Id, const FString&, Titulo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnObjetivosCambian);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMisionCompletada, FName, Id);

UCLASS()
class ALSASUAGAMEPLAY_API UMisionesSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable) FOnMisionIniciada   OnMisionIniciada;
	UPROPERTY(BlueprintAssignable) FOnObjetivosCambian OnObjetivosCambian;
	UPROPERTY(BlueprintAssignable) FOnMisionCompletada OnMisionCompletada;

	UPROPERTY(EditAnywhere, Category="Misiones") bool bSaltarIntro = false;
	UPROPERTY(EditAnywhere, Category="Misiones") FName PrimeraMision = TEXT("M00");

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category="Misiones") void RegistrarMision(UMisionDef* Def);
	UFUNCTION(BlueprintCallable, Category="Misiones") bool IniciarMision(FName Id);

	// Reacciona al fin de una manifestación convocada por la misión.
	UFUNCTION() void OnManifestacionEstado(EEstadoManifestacion E);
	UFUNCTION(BlueprintCallable, Category="Misiones") void AvanzarObjetivo(FName ObjetivoId, int32 Cantidad = 1);
	UFUNCTION(BlueprintCallable, Category="Misiones") void CompletarObjetivo(FName ObjetivoId);

	// Lectura para el HUD.
	UFUNCTION(BlueprintCallable, Category="Misiones") bool HayMision() const { return Estado == EEstadoMision::Activa && Actual != nullptr; }
	UFUNCTION(BlueprintCallable, Category="Misiones") FString TituloActual() const { return Actual ? Actual->Titulo : FString(); }
	UFUNCTION(BlueprintCallable, Category="Misiones") FName MisionActualId() const { return Actual ? Actual->Id : NAME_None; }
	UFUNCTION(BlueprintCallable, Category="Misiones") TArray<FString> ObjetivosTexto() const;

	// Marcador de objetivo (waypoint en el HUD).
	UFUNCTION(BlueprintCallable, Category="Misiones") bool HayMarcador() const { return bMarcadorActivo; }
	UFUNCTION(BlueprintCallable, Category="Misiones") FVector PosMarcador() const { return MarcadorMundo; }

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMisionesSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }

private:
	UPROPERTY() TMap<FName, UMisionDef*> Registro;
	UPROPERTY() UMisionDef* Actual = nullptr;
	TArray<FObjetivoMision> ObjetivosActivos;
	EEstadoMision Estado = EEstadoMision::Inactiva;
	bool bArrancada = false;
	bool bSuscritoManifa = false;

	// Enganche de objetivos de M00 por proximidad (demo).
	FVector PosInicial = FVector::ZeroVector;
	bool bPosInicial = false;
	void EngancheDemoM00();

	// Waypoint del objetivo activo.
	FVector MarcadorMundo = FVector::ZeroVector;
	bool bMarcadorActivo = false;

	void ComprobarFin();
	void CompletarMision();
	void ConstruirMisionesDemo();   // M00 por defecto si no hay nada registrado
};
