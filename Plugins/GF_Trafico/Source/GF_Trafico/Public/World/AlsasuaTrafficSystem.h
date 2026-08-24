// AlsasuaTrafficSystem.h (capa MANIFA)
// Coches aparcados y señales verticales de tráfico.
//
// El coche va en la plaza que calcula AlsasuaParkingSystem, así que esta fase
// tiene que ir DESPUÉS de la de aparcamiento. Antes se sacaba la posición de
// roads_unity.json aquí mismo y salía mal por tres sitios: el coche al primer
// punto del trazado —que en OSM es el nudo del cruce— con ±2 m de ruido, el giro
// por FRandRange(0, 360) o sea atravesado en la calzada, y el filtro
// `AnchoVia < 5` que, con los anchos que hay, dejaba fuera las 194 calles
// residenciales y aparcaba en la A-10.
//
// La señal iba igual: al primer punto, con giro al azar, y el "ceda el paso"
// sólo donde el ancho pasa de 8 m, que es únicamente la autovía.
#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "Arranque/AlsasuaPilarArranque.h"
#include "AlsasuaTrafficSystem.generated.h"

USTRUCT(BlueprintType)
struct FParkedCar
{
    GENERATED_BODY()
    FString Calle;
    /** Posición en coordenadas de mundo (cm), con la cota ya apoyada. */
    FVector Mundo = FVector::ZeroVector;
    float Rotacion = 0.0f;
    FString Color;
    FString Tipo;
};

USTRUCT(BlueprintType)
struct FTrafficSign
{
    GENERATED_BODY()
    FString Tipo;
    float X = 0.0f;
    float Z = 0.0f;
    float Rotacion = 0.0f;
    FString Texto;
};

UCLASS()
class GF_TRAFICO_API UAlsasuaTrafficSystem : public UGameInstanceSubsystem, public IAlsasuaPilarArranque
{
    GENERATED_BODY()

public:
	virtual int32 EjecutarArranque() override;
	virtual FString EtiquetaArranque() const override;
	virtual int32 OrdenArranque() const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Traffic")
    int32 ColocarCocheAparcado();

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Traffic")
    int32 ColocarSenalesTrafico();

    const TArray<FParkedCar>& GetCoches() const { return Coches; }

private:
    TArray<FParkedCar> Coches;
    TArray<FTrafficSign> SenalesTrafico;

    /** Anfitrión propio para cada uno: con uno compartido, la segunda llamada
     *  lo recrearía y se llevaría por delante las capas de la primera. */
    UPROPERTY() TObjectPtr<AActor> HostCoches = nullptr;
    UPROPERTY() TObjectPtr<AActor> HostSenales = nullptr;

    void GenerarCochesDesdeCalles();
    void GenerarSenalesDesdeCalles();

    bool PrepararHost(class UWorld* World, const TCHAR* Etiqueta, TObjectPtr<AActor>& Slot);
    class UHierarchicalInstancedStaticMeshComponent* CrearCapa(
        AActor* Anfitrion, const TCHAR* Nombre, class UStaticMesh* Malla);
};
