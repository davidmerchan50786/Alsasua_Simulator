#!/usr/bin/env python3
# PrepararLandscape.py — convierte el heightmap clipmap V3 de Unity (4097^2, r16)
# en un heightmap listo para Unreal Landscape (4033^2, tamaño válido) y calcula
# la transformada exacta del actor para que la Z del mundo coincida con
# alturaUnity_m * 100 (cm).
#
#   Codificacion Unity (no se toca el significado): altitudReal = 495 + q/64
#   Mundo Unreal (cm):  WorldZ = (altitudReal - Z_MIN) * 100,  Z_MIN = 511.33
#
#   Unreal Landscape:   WorldZ = Loc.Z + (v - 32768) * (1/128) * ScaleZ
#   Igualando con v = q  (mismos valores):
#       ScaleZ = 1.5625 * 128 = 200
#       Loc.Z  = -1633 + 256*200 = 49567 cm  (495.67 m)
#   XY: metrosPorPixel * 100. Para 4033 verts sobre 7200 m -> 7200/4032 m/quad.
#
# Uso:  python3 PrepararLandscape.py <r16_entrada> <carpeta_salida>
import sys, os, json, numpy as np

SRC_RES   = 4097
DST_RES   = 4033          # tamaño de Landscape valido (63 quads * 64 comp + 1)
LADO_M    = 7200.0
DATUM     = 495.0
CUANTO    = 1.0/64.0
Z_MIN     = 511.33

def resample_bilineal(a, n):
    """Remuestrea a (HxW) -> (n x n) bilineal, separable."""
    h, w = a.shape
    yi = np.linspace(0, h-1, n)
    xi = np.linspace(0, w-1, n)
    # interp por filas y luego por columnas
    cols = np.empty((h, n), np.float64)
    xf = np.arange(w)
    for r in range(h):
        cols[r] = np.interp(xi, xf, a[r])
    out = np.empty((n, n), np.float64)
    yf = np.arange(h)
    for c in range(n):
        out[:, c] = np.interp(yi, yf, cols[:, c])
    return out

def main():
    src = sys.argv[1] if len(sys.argv) > 1 else \
        "/sessions/amazing-elegant-keller/mnt/Altsasu_Manifa/Assets/AlsasuaData/terrain_clipmap_v3/heightmap_unificado.r16"
    out_dir = sys.argv[2] if len(sys.argv) > 2 else \
        "/sessions/amazing-elegant-keller/mnt/coperenea--Alsasua_Simulator/UnrealProject/Content/Terreno"
    os.makedirs(out_dir, exist_ok=True)

    q = np.fromfile(src, dtype="<u2").astype(np.float64)
    assert q.size == SRC_RES*SRC_RES, f"esperaba {SRC_RES}^2, vi {q.size}"
    q = q.reshape(SRC_RES, SRC_RES)

    real = DATUM + q*CUANTO                 # altitud real (m)
    real = resample_bilineal(real, DST_RES) # 4097 -> 4033
    real = np.flipud(real)                  # fila 0 sur -> norte (Unreal). Si sale espejado N-S, quitar.

    q2 = np.clip(np.rint((real - DATUM)/CUANTO), 0, 65535).astype("<u2")
    out_r16 = os.path.join(out_dir, "alsasua_landscape_4033.r16")
    q2.tofile(out_r16)

    m_por_px = LADO_M/(DST_RES-1)
    info = {
        "archivo": os.path.basename(out_r16),
        "resolucion": DST_RES,
        "lado_m": LADO_M,
        "metrosPorPixel": round(m_por_px, 6),
        "import_landscape": {
            "ScaleX_cm": round(m_por_px*100, 4),
            "ScaleY_cm": round(m_por_px*100, 4),
            "ScaleZ":    200.0,
            "LocationZ_cm": 49567.0,
            "nota_XY": "centrar el Landscape para que su centro caiga en Herriko Plaza (UGeoDataAlsasua::HerrikoPlaza X/Y)"
        },
        "componentes": "63 quads/seccion x 1 seccion x 64x64 componentes = 4032 quads (4033 verts)",
        "verif": {}
    }
    # verificacion round-trip
    real_rt = DATUM + q2.astype(np.float64)*CUANTO
    info["verif"] = {
        "hMinReal": round(float(real_rt.min()), 3),
        "hMaxReal": round(float(real_rt.max()), 3),
        "spanZ_m":  round(float(real_rt.max()-real_rt.min()), 3),
        "worldZ_min_cm": round((float(real_rt.min())-Z_MIN)*100, 1),
        "worldZ_max_cm": round((float(real_rt.max())-Z_MIN)*100, 1),
    }
    with open(os.path.join(out_dir, "landscape_import.json"), "w") as f:
        json.dump(info, f, indent=1, ensure_ascii=False)

    print("OK ->", out_r16, f"({os.path.getsize(out_r16)} bytes)")
    print(json.dumps(info, indent=1, ensure_ascii=False))

if __name__ == "__main__":
    main()
