#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SecurityRadarComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnThreatDetected, int32, Level, FString, Description);

/**
 * SecurityRadar: escanea cada ScanInterval segundos una esfera de
 * DetectionRadius alrededor del propietario y cuenta actores cuyas etiquetas
 * estén en ThreatTags (Police / Military / DeepState por defecto).
 * El conteo mapea a ThreatLevel 0-3 y el delegado solo dispara al cambiar de
 * nivel. GetClosestThreat devuelve la amenaza más próxima del último scan.
 */
UCLASS(ClassGroup = (Alsasua), meta = (BlueprintSpawnableComponent))
class GF_SYSTEMS_API USecurityRadarComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	USecurityRadarComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|SecurityRadar")
	float DetectionRadius = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|SecurityRadar", meta = (ClampMin = "0.1"))
	float ScanInterval = 2.f;

	/** 0=calma, 1=sospechoso, 2=alerta, 3=peligro. */
	UPROPERTY(BlueprintReadOnly, Category = "Alsasua|SecurityRadar")
	int32 ThreatLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|SecurityRadar")
	TArray<FName> ThreatTags = { TEXT("Police"), TEXT("Military"), TEXT("DeepState") };

	UPROPERTY(BlueprintAssignable, Category = "Alsasua|SecurityRadar")
	FOnThreatDetected OnThreatDetected;

	/** Solapa una esfera y recalcula amenazas + nivel (también callable desde BP). */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|SecurityRadar")
	void ScanForThreats();

	/** Amenaza más cercana del último scan; nullptr si no hay. */
	UFUNCTION(BlueprintPure, Category = "Alsasua|SecurityRadar")
	AActor* GetClosestThreat() const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	FString DescribirNivel(int32 Nivel) const;

	float TiempoDesdeScan = 0.f;

	/** Resultado del último scan; TObjectPtr para no retener basura destruida. */
	UPROPERTY()
	TArray<TObjectPtr<AActor>> Amenazas;
};
