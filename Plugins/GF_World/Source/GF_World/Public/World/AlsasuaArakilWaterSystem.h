// AlsasuaArakilWaterSystem.h (capa MANIFA)
// Caracteriza el agua del Arakil y de los 73 regatas de waterways_unity.json:
// color, espuma, turbidez y velocidad de corriente por tramo.
//
// Caracteriza, no construye — como AlsasuaRoadSurfaceSystem y
// AlsasuaTerrainLayersSystem. El cauce ya está: lo drapea `UCargadorVias` sobre
// el terreno desde este mismo JSON (fase 3, "río un poco hundido"), y los
// puentes salen de `UCargadorPuentes`. `AuditarSistemas.py` lo tenía marcado
// como riesgo de duplicado contra los dos.
//
// `GenerarMallaAgua` construía, y de la peor manera posible: un
// AStaticMeshActor por tramo —2392— con un Plane escalado y **sin rotar**, con
// FRotator::ZeroRotator. O sea 2392 rectángulos alineados a los ejes del mundo,
// sin seguir el río, encima de la cinta que ya estaba puesta, y 2392 draw calls
// sobre los ~819 de referencia del RESUMEN_TECNICO. Nadie lo llamaba, así que
// no se veía; pero enchufarlo habría sido un desastre silencioso.
//
// Lo que sí aporta es el dato: el Arakil trae 8 m de ancho y los regatas 2, y
// un regato de montaña no baja ni del mismo color ni a la misma velocidad que
// el río del valle. Eso se publica por tramo para quien pinte el agua.
#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaArakilWaterSystem.generated.h"

USTRUCT(BlueprintType)
struct FWaterSegment
{
    GENERATED_BODY()
    FVector Centro = FVector::ZeroVector;
    /** Nombre del cauce en OSM; vacío en el tramo que no lo trae. */
    FString Nombre;
    /** true si es cauce principal (8 m): el Arakil y el Altzania. */
    bool bCauceMayor = false;
    float Ancho = 15.0f;
    float Largo = 50.0f;
    float Profundidad = 2.0f;
    float VelocidadFlujo = 0.5f;
    FLinearColor ColorSuperficie = FLinearColor(0.02f, 0.12f, 0.18f, 0.92f);
    FLinearColor ColorProfundo = FLinearColor(0.01f, 0.04f, 0.08f, 0.95f);
    FLinearColor ColorEspuma = FLinearColor(0.8f, 0.85f, 0.9f, 0.6f);
    float Turbidez = 0.3f;
};

UCLASS()
class GF_WORLD_API UAlsasuaArakilWaterSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Water")
    bool CargarTramosRio();

    /**
     * Publica la caracterización del agua. Devuelve cuántos tramos hay.
     *
     * No crea geometría: el cauce lo drapea UCargadorVias. Ver la cabecera.
     */
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Water")
    int32 PublicarAgua();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Water|Visual")
    float WaterSpeed = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Water|Visual")
    float WaveAmplitude = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Water|Visual")
    float WaveFrequency = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Water|Visual")
    FLinearColor RiverColor = FLinearColor(0.03f, 0.15f, 0.22f, 0.9f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Water|Visual")
    float FoamIntensity = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Water|Physics")
    float CurrentStrength = 200.0f;

    const TArray<FWaterSegment>& GetTramos() const { return Tramos; }

private:
    TArray<FWaterSegment> Tramos;
    bool bCargado = false;
};
