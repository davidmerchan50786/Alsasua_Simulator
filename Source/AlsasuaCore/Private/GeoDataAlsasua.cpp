#include "GeoDataAlsasua.h"
#include <cmath>

// ============================================================
//  WGS84 / UTM Zone 30N precomputed constants
// ============================================================
static constexpr double kE2  = 2.0 * UAlsasuaGeoData::UTM_F - UAlsasuaGeoData::UTM_F * UAlsasuaGeoData::UTM_F;
static constexpr double kEP2 = kE2 / (1.0 - kE2);
static constexpr double kN   = UAlsasuaGeoData::UTM_F / (2.0 - UAlsasuaGeoData::UTM_F);
static constexpr double kA0  = 1.0 - kN + 5.0/4.0*(kN*kN - kN*kN*kN);
static constexpr double kA2  = 3.0/4.0*(kN*kN - kN*kN*kN);
static constexpr double kA4  = 15.0/16.0*(kN*kN*kN*kN - kN*kN*kN*kN*kN);
static constexpr double kL0Rad = UAlsasuaGeoData::UTM_LON0 * UE_PI / 180.0;

// ============================================================
//  Legacy API
// ============================================================

FVector UAlsasuaGeoData::UnityaUnreal(const FVector& UnityPos)
{
    // Input: (east, 0, north) in local meters.
    // Output: (east_cm, north_cm, 0) in UE5 world.
    // UE5: X=east, Y=north, Z=up.
    return FVector(UnityPos.X * 100.0, UnityPos.Z * 100.0, UnityPos.Y * 100.0);
}

FVector UAlsasuaGeoData::UnrealaUnity(const FVector& UnrealPos)
{
    // Inverse of UnityaUnreal: UE5 (X=east, Y=north, Z=up) → local (east, 0, north).
    return FVector(UnrealPos.X / 100.0, UnrealPos.Z / 100.0, UnrealPos.Y / 100.0);
}

FVector UAlsasuaGeoData::HerrikoPlaza()
{
    return FVector(0.0, 0.0, CotaPlazaCm);
}

// ============================================================
//  Absolute local meters → UE5 centimeters
//  Local: X=east, Z=north. UE5: X=east, Y=north, Z=up.
// ============================================================

FVector UAlsasuaGeoData::AbsLocalToUE5(const FVector& AbsLocalPos)
{
    // Input: (east_m, 0, north_m). Output: (east_cm, north_cm, 0).
    return FVector(AbsLocalPos.X * 100.0, AbsLocalPos.Z * 100.0, AbsLocalPos.Y * 100.0);
}

// ============================================================
//  Relative local meters → UE5 centimeters (adds OX/OZ)
// ============================================================

FVector UAlsasuaGeoData::RelLocalToUE5(const FVector& RelLocalPos)
{
    const double Ax = RelLocalPos.X + OX;
    const double Az = RelLocalPos.Z + OZ;
    return FVector(Ax * 100.0, Az * 100.0, RelLocalPos.Y * 100.0);
}

// ============================================================
//  Lat/Lon (WGS84) → UTM Zone 30N → UE5 centimeters
// ============================================================

FVector UAlsasuaGeoData::LatLonToUE5(double LatDeg, double LonDeg, double AltM)
{
    const double LatR = LatDeg * UE_PI / 180.0;
    const double LonR = LonDeg * UE_PI / 180.0;

    const double SinLat = std::sin(LatR);
    const double CosLat = std::cos(LatR);
    const double TanLat = std::tan(LatR);
    const double N_val  = UTM_A / std::sqrt(1.0 - kE2 * SinLat * SinLat);
    const double T = TanLat * TanLat;
    const double C = kEP2 * CosLat * CosLat;

    const double M = UTM_A * (kA0 * LatR - kA2 * std::sin(2.0 * LatR)
                             + kA4 * std::sin(4.0 * LatR));

    const double D  = (LonR - kL0Rad) * CosLat;
    const double D2 = D * D;
    const double D4 = D2 * D2;

    const double E = UTM_FE + UTM_K0 * N_val * (D
        + (1.0 - T + C) * D2 * D / 6.0
        + (5.0 - 18.0*T + T*T + 14.0*C - 58.0*kEP2) * D4 * D / 120.0);

    const double N = UTM_FN + UTM_K0 * M + UTM_K0 * N_val * TanLat
        * (D2 / 2.0
        + (5.0 - T + 9.0*C + 4.0*C*C) * D4 / 24.0
        + (61.0 - 58.0*T + T*T + 600.0*C - 330.0*kEP2) * D4 * D2 / 720.0);

    return UTMToUE5(E, N, AltM);
}

// ============================================================
//  UE5 centimeters → Lat/Lon (WGS84) via inverse UTM
// ============================================================

FVector2D UAlsasuaGeoData::UE5ToLatLon(const FVector& UE5Pos)
{
    const double LocalX = UE5Pos.X / 100.0;
    const double LocalZ = UE5Pos.Y / 100.0;

    const double ApproxE = (LocalX - OriginLocalX) + 568043.3;
    const double ApproxN = (LocalZ - OriginLocalZ) + 4764786.9;

    const double M  = (ApproxN - UTM_FN) / UTM_K0;
    const double Mu = M / (UTM_A * kA0);

    const double Phi1 = Mu
        + (3.0*kN/2.0 - 27.0*kN*kN*kN/32.0) * std::sin(2.0*Mu)
        + (21.0*kN*kN/16.0 - 55.0*kN*kN*kN*kN/32.0) * std::sin(4.0*Mu)
        + (151.0*kN*kN*kN/96.0) * std::sin(6.0*Mu);

    const double SinP = std::sin(Phi1);
    const double CosP = std::cos(Phi1);
    const double TanP = std::tan(Phi1);
    const double N1   = UTM_A / std::sqrt(1.0 - kE2 * SinP * SinP);
    const double T1   = TanP * TanP;
    const double C1   = kEP2 * CosP * CosP;
    const double R1   = UTM_A * (1.0 - kE2) / std::pow(1.0 - kE2 * SinP * SinP, 1.5);
    const double D    = (ApproxE - UTM_FE) / (N1 * UTM_K0);
    const double D2   = D * D;

    const double Lat = Phi1 - (N1 * TanP / R1) * (D2/2.0
        - (5.0 + 3.0*T1 + 10.0*C1 - 4.0*C1*C1 - 9.0*kEP2) * D2*D2/24.0
        + (61.0 + 90.0*T1 + 298.0*C1 + 45.0*T1*T1 - 252.0*kEP2 - 3.0*C1*C1) * D2*D2*D2/720.0);

    const double Lon = kL0Rad + (D
        - (1.0 + 2.0*T1 + C1) * D*D2/6.0
        + (5.0 - 2.0*C1 + 28.0*T1 - 3.0*C1*C1 + 8.0*kEP2 + 24.0*T1*T1) * D*D2*D2/120.0) / CosP;

    return FVector2D(Lat * 180.0 / UE_PI, Lon * 180.0 / UE_PI);
}

// ============================================================
//  UTM meters → UE5 centimeters (centered on Alsasua)
// ============================================================

FVector UAlsasuaGeoData::UTMToUE5(double UtmE, double UtmN, double AltM)
{
    const double LocalE = UtmE - 568043.3;
    const double LocalN = UtmN - 4764786.9;
    return FVector(LocalE * 100.0, LocalN * 100.0, AltM * 100.0);
}
