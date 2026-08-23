// AlsasuaTerrainLayersSystem.h (capa MANIFA)
// Firme de cada barrio: qué material le toca al suelo urbano de Herriko, Zelai,
// Intxostia, SanPedro, Errota, Harrobieta, Ferroviario y Monte.
//
// Clasifica, no construye — igual que AlsasuaRoadSurfaceSystem.
//
// Antes construía, y mal. `AplicarMaterialesPorBarrio` soltaba un
// AStaticMeshActor por barrio con un Plane escalado ×1000, o sea un cuadrado de
// **un kilómetro de lado**, y los ocho en (0,0,0): apilados uno dentro de otro,
// a 1,9 km del pueblo y 531 m por debajo, porque el mundo tiene la cota de
// Herriko en 53194 cm y no en cero. Los campos Barrio, BlendDistance y
// HeightOffset de la tabla no los leía nadie.
//
// `GenerarSueloCiudad` hacía lo mismo con un noveno plano, ese sí centrado en el
// pueblo pero también a cota cero: UnityaUnreal(1900, 0, 8570) propaga el
// segundo componente —la vertical— a la Z de salida, y ahí iba un cero.
//
// Y aunque se hubieran colocado bien, nueve planos opacos de un kilómetro tapan
// la ortofoto PNOA y el relieve del terreno, que es justo lo que CLAUDE.md §5
// dice que no se haga: el firme por zona se tiñe en el material, no se cubre con
// geometría nueva.
//
// Quien consume la tabla es AlsasuaSidewalkSystem, que llevaba la regla de qué
// barrio va empedrado copiada dentro. Cero actores.
#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "AlsasuaTerrainLayersSystem.generated.h"

USTRUCT(BlueprintType)
struct FBarrioTerrainLayer
{
    GENERATED_BODY()
    FString Barrio;
    FString MaterialPath;
    float BlendDistance = 2000.0f;
    float HeightOffset = 0.0f;
    /** Casco viejo y cantera van en piedra; el resto en asfalto, grava o tierra. */
    bool bEmpedrado = false;
};

UCLASS()
class GF_CARRETERAS_API UAlsasuaTerrainLayersSystem : public UGameInstanceSubsystem, public IAlsasuaPilarArranque
{
    GENERATED_BODY()
	public:
	virtual int32 EjecutarArranque() override { return PublicarFirmePorBarrio(); }
	virtual FString EtiquetaArranque() const override { return TEXT("firme publicado por barrio"); }
	virtual int32 OrdenArranque() const override { return 340; }

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    /** Publica el firme por barrio. Devuelve cuántos barrios hay en la tabla. */
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Terrain")
    int32 PublicarFirmePorBarrio();

    /** Ruta del material de firme del barrio, o vacío si no está en la tabla. */
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Terrain")
    FString MaterialDeBarrio(const FString& Barrio) const;

    /** ¿El suelo urbano de ese barrio va empedrado? */
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Terrain")
    bool BarrioEmpedrado(const FString& Barrio) const;

    const TArray<FBarrioTerrainLayer>& GetLayers() const { return Layers; }

private:
    TArray<FBarrioTerrainLayer> Layers;
    void CrearLayers();
};
