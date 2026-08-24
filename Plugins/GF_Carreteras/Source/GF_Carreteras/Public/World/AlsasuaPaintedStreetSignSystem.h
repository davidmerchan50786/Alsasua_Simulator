// AlsasuaPaintedStreetSignSystem.h (capa MANIFA)
// Placas de calle bilingües (es/eu) pintadas en el muro de la esquina.
//
// Iban clavadas en el primer punto del eje de la calzada, a 2,5 m de altura y
// con un giro sorteado: una placa flotando en mitad de la calle, mirando a
// donde cayera. Y la malla era /Engine/BasicShapes/Plane, un plano en XY que
// mira hacia arriba, escalado en Z: tumbado en el suelo. Ahora van sobre el
// muro más cercano al arranque de la calle, que es donde está una placa.
//
// El tope se gastaba además repitiendo calle: los 489 trazados de
// roads_unity.json son 231 calles, y el fichero no viene en orden geográfico.
#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "Arranque/AlsasuaPilarArranque.h"
#include "AlsasuaPaintedStreetSignSystem.generated.h"

USTRUCT(BlueprintType)
struct FPaintedSignEntry
{
    GENERATED_BODY()
    FString NombreES;
    FString NombreEU;
    FVector Posicion = FVector::ZeroVector;
    float Rotacion = 0.0f;
    float Ancho = 300.0f;
    float Altura = 80.0f;
    FString Barrio;
    FString Color;
};

UCLASS()
class GF_CARRETERAS_API UAlsasuaPaintedStreetSignSystem : public UGameInstanceSubsystem, public IAlsasuaPilarArranque
{
    GENERATED_BODY()
	public:
	virtual int32 EjecutarArranque() override { return ColocarRotulosPintados(); }
	virtual FString EtiquetaArranque() const override { return TEXT("rotulos pintados bilingues"); }
	virtual int32 OrdenArranque() const override { return 510; }

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|PaintedSigns")
    int32 ColocarRotulosPintados();

    /** Tope de placas. Ahora es una por calle, así que 231 es el techo real. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|PaintedSigns")
    int32 MaxRotulos = 120;

    const TArray<FPaintedSignEntry>& GetRotulos() const { return Rotulos; }

private:
    TArray<FPaintedSignEntry> Rotulos;

    /** Actor que aloja la capa instanciada. */
    UPROPERTY() TObjectPtr<AActor> Host = nullptr;
};
