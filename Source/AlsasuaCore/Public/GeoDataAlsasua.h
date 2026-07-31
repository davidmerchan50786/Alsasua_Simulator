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
     *  Input: (east, 0, north). Output: (east_cm, north_cm, 0). */
    static FVector AbsLocalToUE5(const FVector& AbsLocalPos);

    /** Relative local meters → UE5 centimeters (adds OX/OZ).
     *  Input: (east, 0, north). Output: (east_cm, north_cm, 0). */
    static FVector RelLocalToUE5(const FVector& RelLocalPos);

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

    // Altura de Herriko Plaza sobre el nivel del mar (cm).
    static constexpr float CotaPlazaCm = 53194.0f;

    // Límites para raycasts de suelo (trace vertical desde/to).
    static constexpr float TraceUp = 500000.0f;
    static constexpr float TraceDown = -500000.0f;

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
