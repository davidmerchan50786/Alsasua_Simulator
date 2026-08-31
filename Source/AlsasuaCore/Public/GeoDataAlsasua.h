#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GeoDataAlsasua.generated.h"

USTRUCT(BlueprintType)
struct ALSASUACORE_API FAlsasuaGeoCoords
{
    GENERATED_BODY()
    UPROPERTY() double UTM_E = 567951.0;
    UPROPERTY() double UTM_N = 4749902.0;
    UPROPERTY() double OX = 1918.0;
    UPROPERTY() double OZ = 8570.0;
    UPROPERTY() double ScaleX = 1.0;
    UPROPERTY() double CotaPlaza = 531.94;
    UPROPERTY() double MToUU = 100.0;
};

// ============================================================
//  Coordinate systems
// ============================================================
//
//  LOCAL (JSON data):
//    - buildings_final.json vertices: relative to arbitrary origin
//    - roads_unity.json points: relative (add OX/OZ → absolute)
//    - street_furniture.json: relative (add OX/OZ → absolute)
//    - trees_unity.json: ABSOLUTE (no offset needed)
//    - signage_data.json: ABSOLUTE (no offset needed)
//    - waterways_unity.json: ABSOLUTE (flat float triples)
//
//  ABSOLUTE LOCAL (after OX/OZ applied):
//    - X = east meters, Z = north meters
//    - Origin at some reference point southwest of Alsasua
//    - All data lives in ~2700m × 2200m area
//
//  UTM ZONE 30N (EPSG:32630):
//    - Real-world metric projection
//    - Alsasua center (LIDAR meta): E=567951.0m, N=4749902.0m
//    - Central meridian: -3°, scale factor 0.9996
//
//  UNREAL ENGINE 5:
//    - X = east centimeters, Y = north centimeters, Z = up centimeters
//    - World centered at FVector(191800, 857000, 0) for terrain
//    - 1 local meter = 100 UE5 centimeters
// ============================================================

UCLASS()
class ALSASUACORE_API UAlsasuaGeoData : public UObject
{
    GENERATED_BODY()
public:
    // --- Legacy API (kept for backward compat) ---
    static FVector UnityaUnreal(const FVector& UnityPos);
    static FVector UnrealaUnity(const FVector& UnrealPos);
    static FVector HerrikoPlaza();

    // --- New coordinate conversion API ---

    /** Absolute local meters → UE5 centimeters.
     *  Input: (east, up, north). Output: (east_cm, north_cm, up_cm).
     *  La vertical del vector de entrada va EN MEDIO y se propaga a la Z de
     *  salida; con el segundo componente a cero, la Z sale cero. */
    static FVector AbsLocalToUE5(const FVector& AbsLocalPos);

    /** Relative local meters → UE5 centimeters (adds OX/OZ).
     *  Input: (east, up, north). Output: (east_cm, north_cm, up_cm). */
    static FVector RelLocalToUE5(const FVector& RelLocalPos);

    /**
     * Mobiliario urbano → UE5, decidiendo el marco por la propia coordenada.
     *
     * street_furniture.json no está en un solo marco: lo escribieron dos
     * generadores distintos y quedaron mezclados. 191 de sus 220 piezas están en
     * local RELATIVO (papeleras, bancos, bolardos, bocas de incendio…) y 29 en
     * ABSOLUTO (las 12 paradas de bus, las 5 fuentes, las señales, los cruces).
     *
     * Se distinguen por la coordenada norte y no hay ambigüedad posible: los dos
     * grupos están separados por un hueco de 4115 m —el más alto de los
     * relativos es z=4452,8 y el más bajo de los absolutos z=8568,0—, así que el
     * corte a 6000 tiene kilómetro y medio de margen por abajo y dos y medio por
     * arriba. En este por la X no se puede: los rangos se solapan.
     *
     * Aplicar el marco relativo a todo —que es lo que se hacía— manda esas 29
     * piezas 1918 m al este y 8570 al norte de donde van, o sea a 8,6 km del
     * pueblo, fuera del terreno jugable.
     */
    static FVector MobiliarioAUE5(const FVector& LocalPos);

    /**
     * Cota del terreno bajo un punto del mundo (cm), por trazo vertical.
     *
     * Si no encuentra suelo devuelve CotaPlazaCm: es mejor dejar la pieza a la
     * altura del pueblo que a la del nivel del mar.
     */
    /**
     * Cota del terreno bajo un punto del mundo (cm).
     *
     * Si hay muestreador registrado —lo hace ATerrenoGenerado con su propio
     * heightmap— se lee de ahí. Si no, se cae al trazo vertical de siempre.
     *
     * El cambio no es sólo de velocidad, aunque de eso va: el arranque hacía del
     * orden de 25 000 de estos, 12 000 sólo del foliage, y cada uno es una
     * consulta de física contra la escena entera. Leer el heightmap es una
     * interpolación bilineal sobre un array que ya está en memoria.
     *
     * Y además es MÁS correcto para quien pide "el suelo". El trazo devuelve lo
     * más alto que haya debajo, sea lo que sea: por eso el tren y las bocas de
     * túnel están en las fases 52 y 51b, para que nada se suba a un vagón o a un
     * dintel. Muestreando el terreno ese peligro de orden desaparece.
     *
     * Quien de verdad quiera "lo que haya encima, sea lo que sea" —drapear sobre
     * un puente, por ejemplo— tiene AlturaSuperficieUE5, que sigue trazando.
     */
    static float AlturaSueloUE5(const class UWorld* World, double XCm, double YCm);

    /**
     * Cota de la superficie más alta bajo un punto, por trazo vertical.
     *
     * Lo que hacía AlturaSueloUE5 siempre. Se conserva aparte porque hay casos
     * en que es lo que se quiere: apoyarse sobre un puente, sobre una plaza
     * drapeada o sobre cualquier cosa ya colocada. Si usas esto, el orden de
     * fases importa — ver el aviso de CLAUDE.md §5.
     */
    static float AlturaSuperficieUE5(const class UWorld* World, double XCm, double YCm);

    /**
     * Muestreador de cota del terreno. Devuelve false si el punto cae fuera de
     * lo que sabe, y entonces se usa el trazo.
     *
     * Vive como puntero registrable porque AlsasuaCore no puede ver a
     * ATerrenoGenerado: la dependencia va AlsasuaCore → AlsasuaWorld. El terreno
     * se registra al construirse y se borra al destruirse.
     */
    using FMuestreadorCota = TFunction<bool(double XCm, double YCm, float& OutZCm)>;

    static void RegistrarMuestreadorTerreno(FMuestreadorCota Fn);
    static void OlvidarMuestreadorTerreno();
    static bool HayMuestreadorTerreno();

    /**
     * Relative local meters → UE5 centimeters, con la Z apoyada en el terreno.
     *
     * Existe porque RelLocalToUE5 devuelve Z = 0 y veintiún sistemas hacían
     * "Pos.Z += 300" sobre eso: Alsasua está a 531 m, así que el mobiliario,
     * los toldos, las aceras, los semáforos y los peatones acababan medio
     * kilómetro por debajo del terreno. La Y del vector de entrada se ignora:
     * la altura la manda SobreSueloCm.
     */
    static FVector RelLocalASueloUE5(const class UWorld* World, const FVector& RelLocalPos,
                                     float SobreSueloCm = 0.f);

    /**
     * Igual, pero apoyando la Z en la superficie trazada en vez de en el terreno.
     *
     * Para lo que va pintado SOBRE algo ya colocado, no sobre el suelo desnudo:
     * las marcas viales y las plazas de aparcamiento van encima del firme, y el
     * firme —ACalleGenerada— se levanta EpsilonCm = 12 sobre el terreno para no
     * hacer z-fighting con él. Muestreando el terreno esas marcas se pintan 12 cm
     * por debajo del asfalto, o sea invisibles; los +2 cm que se dan sobre el
     * firme dan por supuesto que la cota de partida es la del firme.
     */
    static FVector RelLocalASuperficieUE5(const class UWorld* World, const FVector& RelLocalPos,
                                          float SobreSuperficieCm = 0.f);

    /** Lat/Lon (WGS84) + altitude(m) → UE5 centimeters via UTM Zone 30N. */
    static FVector LatLonToUE5(double LatDeg, double LonDeg, double AltM = 0.0);

    /** UE5 centimeters → Lat/Lon (WGS84) via inverse UTM. */
    static FVector2D UE5ToLatLon(const FVector& UE5Pos);

    /** UTM meters → UE5 centimeters (centers on Alsasua origin). */
    static FVector UTMToUE5(double UtmE, double UtmN, double AltM = 0.0);

    /** Get absolute local coordinates of a barrio center (meters).
     *  Coordinates derived from neighborhoods.json relative offsets + Herriko center. */
    static FVector BarrioCenter(const FString& BarrioName);

    /** Get all barrio names and their centers. */
    static void GetAllBarrioCenters(TMap<FString, FVector>& OutCenters);

    // --- Constants ---

    // OX/OZ: offset from relative to absolute local coords.
    static constexpr double OX = 1918.0;
    static constexpr double OZ = 8570.0;

    /**
     * Z DE MUNDO de Herriko Plaza, en cm. No es su altitud.
     *
     * Valía 53194, que son los 531,94 m de altitud multiplicados por 100. Pero
     * esto no se usa como altitud: se usa como Z de mundo —es el respaldo de
     * AlturaSueloUE5 cuando el raycast no encuentra suelo, y HerrikoPlazaGenerator
     * coloca su actor justo ahí—. Y el mundo no cuenta metros sobre el mar:
     * worldZ_cm = 100*alt_m - 51133 (CLAUDE.md §5b, y es lo que aplica
     * ATerrenoLejano). Con 531,94 m eso da 2061 cm.
     *
     * Con el valor viejo, todo lo que caía en el respaldo aparecía 511 m por
     * encima del pueblo. Se notaba poco porque el terreno tenía a su vez el
     * doble de escala vertical (EscalaZ=200 en vez de 100), que empujaba el
     * suelo hacia arriba y disimulaba parte del error; corregido eso, este
     * quedaba al descubierto.
     */
    static constexpr float CotaPlazaCm = 2061.0f;

    // Límites para raycasts de suelo (trace vertical desde/to).
    static constexpr float TraceUp = 500000.0f;
    static constexpr float TraceDown = -500000.0f;

    /**
     * Caja del terreno jugable (cm): 7200×7200 m centrados aquí.
     *
     * Hace falta porque hay datos que caen fuera y colocarlos no falla: 31 de
     * las 126 señales de signage_data.json llegan a 216 km, y 280 de los 2783
     * árboles del LiDAR quedan fuera. El actor se crea igual, su trazo de suelo
     * no encuentra nada y acaba a cota cero o a la de la plaza. Quien lea esos
     * datos tiene que filtrarlos diciendo cuántos.
     *
     * Está aquí y no en cada sistema porque son los límites del mundo, no una
     * decisión de quien coloca: los llevaban copiados a mano AlsasuaSignPlacer y
     * Tools/VerificarDatasets.py, y tres copias de una constante son tres
     * oportunidades de que una se quede atrás.
     */
    static constexpr double CentroTerrenoXCm = 191800.0;
    static constexpr double CentroTerrenoYCm = 857000.0;
    static constexpr double SemiTerrenoCm    = 360000.0;

    /** ¿Cae este punto del mundo dentro del terreno jugable? */
    static bool DentroDelTerreno(const FVector& UE5Pos)
    {
        return FMath::Abs(UE5Pos.X - CentroTerrenoXCm) <= SemiTerrenoCm
            && FMath::Abs(UE5Pos.Y - CentroTerrenoYCm) <= SemiTerrenoCm;
    }

    // --- UTM Zone 30N constants (WGS84) ---
    static constexpr double UTM_A = 6378137.0;           // WGS84 semi-major axis
    static constexpr double UTM_F = 1.0 / 298.257223563; // WGS84 flattening
    static constexpr double UTM_K0 = 0.9996;             // UTM scale factor
    static constexpr double UTM_FE = 500000.0;            // False easting
    static constexpr double UTM_FN = 0.0;                 // False northing (N hemisphere)
    static constexpr double UTM_LON0 = -3.0;              // Central meridian (degrees)

    // Alsasua origin in absolute local coords (for centering).
    // Trees data center: ~1891.5, 8572.0
    // (UTM↔UE5 usa el frame LIDAR: UE5 m = UTM - (566033, 4741332); no usar estos para UTM.)
    static constexpr double OriginLocalX = 1891.5;
    static constexpr double OriginLocalZ = 8572.0;
};
