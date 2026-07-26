#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaBarrioStyleSystem.generated.h"

/**
 * Sistema que aplica el estilo visual REAL de cada barrio de Alsasua
 * basándose en los datos de nighborhoods.json.
 * 
 * SOLO usa datos reales: materiales de fachada, colores de tejado,
 * alturas medias, y tipos de barrio documentados.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UAlsasuaBarrioStyleSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaBarrioStyleSystem();

	virtual void BeginPlay() override;

	// --- Datos reales por barrio (de nighborhoods.json) ---

	// Herriko Aldea (Casco Viejo)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrio|Herriko")
	FLinearColor HerrikoFachadaPiedra = FLinearColor(0.75f, 0.72f, 0.65f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrio|Herriko")
	FLinearColor HerrikoTejadoTerracota = FLinearColor(0.7f, 0.35f, 0.15f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrio|Herriko")
	float HerrikoAlturaMedia = 1200.f;

	// Zelai (Residencial)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrio|Zelai")
	FLinearColor ZelaiFachadaHormigon = FLinearColor(0.85f, 0.83f, 0.8f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrio|Zelai")
	FLinearColor ZelaiTejadoGris = FLinearColor(0.5f, 0.5f, 0.5f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrio|Zelai")
	float ZelaiAlturaMedia = 800.f;

	// Intxostia (Ensanche)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrio|Intxostia")
	FLinearColor IntxostiaFachadaHormigon = FLinearColor(0.8f, 0.8f, 0.78f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrio|Intxostia")
	FLinearColor IntxostiaTejadoPlano = FLinearColor(0.55f, 0.55f, 0.55f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrio|Intxostia")
	float IntxostiaAlturaMedia = 1500.f;

	// Errota (Industrial, junto al río)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrio|Errota")
	FLinearColor ErrotaFachadaLadrillo = FLinearColor(0.65f, 0.35f, 0.2f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrio|Errota")
	FLinearColor ErrotaTejadoTerracota = FLinearColor(0.7f, 0.35f, 0.15f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrio|Errota")
	float ErrotaAlturaMedia = 600.f;

	// San Pedro (Estación + comercios)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrio|SanPedro")
	FLinearColor SanPedroFachadaPiedra = FLinearColor(0.7f, 0.68f, 0.62f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrio|SanPedro")
	FLinearColor SanPedroFachadaHormigon = FLinearColor(0.8f, 0.78f, 0.75f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrio|SanPedro")
	float SanPedroAlturaMedia = 1000.f;

	// Harrobieta (Mercado + nightlife)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrio|Harrobieta")
	FLinearColor HarrobietaFachadaPiedra = FLinearColor(0.68f, 0.65f, 0.58f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrio|Harrobieta")
	FLinearColor HarrobietaTejadoTerracota = FLinearColor(0.7f, 0.35f, 0.15f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrio|Harrobieta")
	float HarrobietaAlturaMedia = 900.f;

	// Ferroviario (Zona industrial vías)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrio|Ferroviario")
	FLinearColor FerroviarioFachadaLadrillo = FLinearColor(0.6f, 0.3f, 0.15f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrio|Ferroviario")
	FLinearColor FerroviarioTejadoOxidado = FLinearColor(0.5f, 0.3f, 0.15f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrio|Ferroviario")
	float FerroviarioAlturaMedia = 500.f;

	// Monte (Ladera, caseríos dispersos)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrio|Monte")
	FLinearColor MonteFachadaPiedraRustica = FLinearColor(0.6f, 0.55f, 0.48f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrio|Monte")
	FLinearColor MonteTejadoPizarra = FLinearColor(0.3f, 0.3f, 0.32f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrio|Monte")
	float MonteAlturaMedia = 400.f;

private:
	void ApplyBarrioStyles();

	UPROPERTY()
	TObjectPtr<UDataTable> BarrioDataTable;
};
