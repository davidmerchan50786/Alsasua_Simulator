#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "FestivalSubsystem.generated.h"

// Tipos de tradiciones locales
UENUM(BlueprintType)
enum class ETraditionType : uint8 {
    Momotxorros,
    SanPedro,
    Romeria,
    Carnaval,
    Other
};

USTRUCT(BlueprintType)
struct FFestivalSchedule {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Month;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Day;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ETraditionType Festival;
};

UCLASS()
class ALSASUAMANIFA_API UFestivalSubsystem : public UWorldSubsystem {
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // Actualiza el reloj del juego y checkea si hay festival
    UFUNCTION(BlueprintCallable, Category="AAA|Calendar")
    void UpdateCalendar(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category="AAA|Calendar")
    void StartFestival(ETraditionType Festival);

    UPROPERTY(BlueprintReadOnly, Category="AAA|Calendar")
    int32 CurrentDay = 1;

    UPROPERTY(BlueprintReadOnly, Category="AAA|Calendar")
    int32 CurrentMonth = 1;

    UPROPERTY(BlueprintReadOnly, Category="AAA|Calendar")
    float GameTime = 0.f;

private:
    TArray<FFestivalSchedule> Calendar;
    void CheckForFestivals();

    // Configuración de fechas reales de Altsasu
    void SetupRealDates();
};