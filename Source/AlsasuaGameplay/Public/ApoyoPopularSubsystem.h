// ApoyoPopularSubsystem.h (capa GAMEPLAY)
// Apoyo popular + paranoia, eje de la economía/IA. Puerto de SistemaApoyoPopular.
// Acceso:  GetGameInstance()->GetSubsystem<UApoyoPopularSubsystem>()
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "ApoyoPopularSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnApoyoCambia, float, NuevoApoyo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnParanoiaCambia, float, NuevaParanoia);

UCLASS()
class ALSASUAGAMEPLAY_API UApoyoPopularSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(BlueprintReadOnly, Category="Apoyo") float Apoyo = 50.f;     // 0-100
	UPROPERTY(BlueprintReadOnly, Category="Apoyo") float Paranoia = 0.f;   // 0-100
	UPROPERTY(EditAnywhere, Category="Apoyo")      float DecayApoyo = 0.5f;

	UPROPERTY(BlueprintAssignable, Category="Apoyo") FOnApoyoCambia OnApoyoCambia;
	UPROPERTY(BlueprintAssignable, Category="Paranoia") FOnParanoiaCambia OnParanoiaCambia;

	UFUNCTION(BlueprintCallable, Category="Apoyo") void SumarApoyo(float Cantidad, const FString& Razon = TEXT(""));
	UFUNCTION(BlueprintCallable, Category="Apoyo") void RestarApoyo(float Cantidad, const FString& Razon = TEXT(""));
	UFUNCTION(BlueprintCallable, Category="Apoyo") void SumarParanoia(float Cantidad);
	UFUNCTION(BlueprintCallable, Category="Apoyo") void RestarParanoia(float Cantidad);

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UApoyoPopularSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }
};
