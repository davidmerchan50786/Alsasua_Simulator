// AlsasuaOverheadCableSystem.h (capa MANIFA)
// Tendido eléctrico aéreo sobre las calles del casco: postes y vanos.
//
// Tenía cuatro fallos que se tapaban entre ellos:
//
//  - Los tres postes de cada vano se colocaban a la cota del **primer** punto
//    del tramo (`PostPos.Z = Loc0.Z`), así que en cuanto la calle tenía
//    pendiente el segundo y el tercero salían flotando o enterrados. Cada poste
//    muestrea ahora su propio suelo.
//  - Los vanos consecutivos compartían extremo y cada uno plantaba su poste
//    ahí: postes duplicados, uno dentro de otro, en cada junta.
//  - El cable era un cubo girado sólo en yaw. Entre dos postes a cotas
//    distintas salía horizontal, sin tocar ninguno de los dos. Ahora lleva
//    también el pitch del vano.
//  - Se tendía sobre cualquier vía con nombre, incluidas la A-10 y la N-1. El
//    tendido aéreo de un pueblo va por las calles, no por la autovía.
//
// Y los cuatro `LoadObject` estaban dentro del bucle. Postes y cables van a dos
// capas instanciadas.
#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "AlsasuaOverheadCableSystem.generated.h"

USTRUCT(BlueprintType)
struct FOverheadCable
{
    GENERATED_BODY()
    FVector Inicio = FVector::ZeroVector;
    FVector Fin = FVector::ZeroVector;
    float Caida = 30.0f;
    FString Tipo;
    FString Calle;
};

UCLASS()
class GF_CARRETERAS_API UAlsasuaOverheadCableSystem : public UGameInstanceSubsystem, public IAlsasuaPilarArranque
{
    GENERATED_BODY()
	public:
	virtual int32 EjecutarArranque() override { return ColocarCables(); }
	virtual FString EtiquetaArranque() const override { return TEXT("cables aereos"); }
	virtual int32 OrdenArranque() const override { return 500; }

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Cables")
    int32 ColocarCables();

    /** Tope de vanos. Las 205 calles con nombre del casco dan 890 vanos sobre
     *  1095 postes con el vano de 35 m, así que el tope no recorta: está por
     *  encima a propósito, porque cortar por él trunca en orden de fichero y
     *  roads_unity.json no viene ordenado por nada geográfico. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Cables")
    int32 MaxCables = 1200;

    /** Altura del cable sobre el suelo, en cm. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Cables")
    float AlturaCables = 500.0f;

    /** Separación entre postes, en cm. Un vano de calle son 30-40 m. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Cables")
    float VanoCm = 3500.0f;

    const TArray<FOverheadCable>& GetCables() const { return Cables; }

private:
    TArray<FOverheadCable> Cables;

    /** Actor que aloja las dos capas instanciadas. */
    UPROPERTY() TObjectPtr<AActor> Host = nullptr;
};
