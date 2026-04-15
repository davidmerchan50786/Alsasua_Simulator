#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
DescargadorIDENA.py
═══════════════════════════════════════════════════════════════════════════════
Convierte datos LiDAR/DEM del Valle de Burunda a un heightmap RAW de 16 bits
listo para importar en Unity Terrain.

FUENTES SOPORTADAS:
  --archivo ruta.asc   ← MDT05 descargado de filescartografia.navarra.es
  --archivo ruta.tif   ← GeoTIFF (requiere gdal)
  --analitico          ← Modelo analítico (sin archivos externos)
  (sin flags)          ← Descarga automática via WCS de IDENA

PARA DESCARGAR EL MDT05:
  1. Abre https://filescartografia.navarra.es/5_LIDAR/
  2. Entra en la carpeta MDT05/ (o PNOA_2020/MDT05/)
  3. Descarga el .zip de la hoja 30TWN84  (cubre el Valle de Burunda)
  4. Extrae el .asc y ejecútalo con:
       python DescargadorIDENA.py --archivo PNOA_MDT05_30TWN84.asc --res 513

SALIDA:
  Assets/Terrain/alsasua_heightmap.raw   ← Unity Terrain Heightmap (16-bit)
  Assets/Terrain/alsasua_heightmap.json  ← Metadatos

IMPORTAR EN UNITY:
  1. Terrain component → Import Raw
  2. Depth: 16 bit  |  Byte Order: Windows  |  Flip: Y-axis = ON
  3. Width/Height: el valor del JSON (513 por defecto)
  4. Terrain Height Inspector: 600 m

REQUISITOS:
  pip install numpy
  pip install requests   (solo para descarga WCS automática)
  pip install gdal       (solo para leer .tif directamente)
═══════════════════════════════════════════════════════════════════════════════
"""

import argparse
import json
import math
import os
import struct
import sys
import time

try:
    import requests
    HAS_REQUESTS = True
except ImportError:
    HAS_REQUESTS = False

try:
    import numpy as np
    HAS_NUMPY = True
except ImportError:
    HAS_NUMPY = False
    sys.exit("ERROR: instala numpy →  pip install numpy")

# ═══════════════════════════════════════════════════════════════════════════
#  CONSTANTES (igual que GeoDataAlsasua.cs)
# ═══════════════════════════════════════════════════════════════════════════

LAT_CENTRO  =  42.9016
LON_CENTRO  =  -2.1668
ALT_CENTRO  = 536.0        # altitud real del valle en metros s.n.m.

M_POR_GRADO_LAT = 111_300.0
M_POR_GRADO_LON =  81_490.0

IDENA_WCS = "https://idena.navarra.es/ogc/wcs"

# ═══════════════════════════════════════════════════════════════════════════
#  CONVERSIÓN COORDENADAS  UTM Zone 30N (ETRS89/EPSG:25830) ↔ lat/lon
#  Los archivos MDT05 de IDENA usan UTM 30N sin reproyección.
# ═══════════════════════════════════════════════════════════════════════════

def latlon_a_utm30n(lat_deg: float, lon_deg: float) -> tuple[float, float]:
    """Convierte lat/lon (WGS84/ETRS89) a UTM Zone 30N (EPSG:25830)."""
    a  = 6_378_137.0
    f  = 1 / 298.257_223_563
    b  = a * (1 - f)
    e2 = 1 - (b / a) ** 2
    k0 = 0.9996
    lon0 = math.radians(-3.0)   # meridiano central zona 30

    lat = math.radians(lat_deg)
    lon = math.radians(lon_deg)

    N   = a / math.sqrt(1 - e2 * math.sin(lat) ** 2)
    T   = math.tan(lat) ** 2
    C   = e2 / (1 - e2) * math.cos(lat) ** 2
    A   = math.cos(lat) * (lon - lon0)
    e2p = e2 / (1 - e2)

    M = a * (
        (1 - e2/4 - 3*e2**2/64  - 5*e2**3/256)   * lat
      - (3*e2/8 + 3*e2**2/32    + 45*e2**3/1024)  * math.sin(2*lat)
      + (15*e2**2/256 + 45*e2**3/1024)             * math.sin(4*lat)
      - 35*e2**3/3072                               * math.sin(6*lat)
    )

    x = k0 * N * (A + (1-T+C)*A**3/6 +
                  (5-18*T+T**2+72*C-58*e2p)*A**5/120) + 500_000
    y = k0 * (M + N * math.tan(lat) * (
                  A**2/2 +
                  (5-T+9*C+4*C**2)*A**4/24 +
                  (61-58*T+T**2+600*C-330*e2p)*A**6/720))
    if lat_deg < 0:
        y += 10_000_000
    return x, y


def utm30n_a_latlon(x: float, y: float) -> tuple[float, float]:
    """Convierte UTM Zone 30N (EPSG:25830) a lat/lon (WGS84/ETRS89)."""
    a  = 6_378_137.0
    f  = 1 / 298.257_223_563
    b  = a * (1 - f)
    e2 = 1 - (b / a) ** 2
    k0 = 0.9996
    e1 = (1 - math.sqrt(1 - e2)) / (1 + math.sqrt(1 - e2))
    lon0 = math.radians(-3.0)

    x -= 500_000
    M  = y / k0
    mu = M / (a * (1 - e2/4 - 3*e2**2/64 - 5*e2**3/256))

    phi1 = (mu
            + (3*e1/2 - 27*e1**3/32)  * math.sin(2*mu)
            + (21*e1**2/16 - 55*e1**4/32) * math.sin(4*mu)
            + 151*e1**3/96             * math.sin(6*mu)
            + 1097*e1**4/512           * math.sin(8*mu))

    N1  = a / math.sqrt(1 - e2 * math.sin(phi1)**2)
    T1  = math.tan(phi1)**2
    C1  = e2/(1-e2) * math.cos(phi1)**2
    R1  = a*(1-e2) / (1 - e2*math.sin(phi1)**2)**1.5
    D   = x / (N1 * k0)
    e2p = e2 / (1 - e2)

    lat = phi1 - (N1*math.tan(phi1)/R1) * (
              D**2/2 -
              (5+3*T1+10*C1-4*C1**2-9*e2p)*D**4/24 +
              (61+90*T1+298*C1+45*T1**2-252*e2p-3*C1**2)*D**6/720)
    lon = lon0 + (D - (1+2*T1+C1)*D**3/6 +
                  (5-2*C1+(28*T1)-(3*C1**2)+(8*e2p)+(24*T1**2))*D**5/120) / math.cos(phi1)

    return math.degrees(lat), math.degrees(lon)


# ═══════════════════════════════════════════════════════════════════════════
#  LECTOR DE ARCHIVOS ASC (Arc/Info ASCII Grid)
#  Formato de los MDT05 de IDENA:
#    ncols / nrows / xllcorner / yllcorner / cellsize / NODATA_value
#    seguido de filas de valores de elevación (fila 0 = norte)
# ═══════════════════════════════════════════════════════════════════════════

def leer_asc(ruta: str) -> tuple[np.ndarray, dict]:
    """
    Lee un archivo ASC de MDT5 de IDENA.
    Devuelve (array_alturas, metadatos).
    metadatos contiene: ncols, nrows, xllcorner, yllcorner, cellsize, nodata, crs
    Las coordenadas están en UTM Zone 30N (ETRS89, EPSG:25830).
    """
    print(f"  Leyendo {os.path.basename(ruta)}...")
    meta = {}
    header_lines = 0

    with open(ruta, "r", encoding="utf-8", errors="replace") as f:
        for _ in range(10):
            line = f.readline().strip()
            if not line:
                break
            parts = line.split()
            if len(parts) == 2:
                key, val = parts[0].lower(), parts[1]
                try:
                    meta[key] = float(val)
                    header_lines += 1
                except ValueError:
                    break
            else:
                break

    required = {"ncols", "nrows", "cellsize"}
    missing  = required - set(meta.keys())
    if missing:
        raise ValueError(f"Header ASC incompleto — faltan: {missing}")

    ncols  = int(meta["ncols"])
    nrows  = int(meta["nrows"])
    nodata = meta.get("nodata_value", meta.get("nodata", -9999.0))

    # xllcorner/yllcorner (esquina inferior izquierda)
    # xllcenter/yllcenter (centro de la celda inferior izquierda)
    if "xllcorner" in meta:
        xll = meta["xllcorner"]
        yll = meta["yllcorner"]
    elif "xllcenter" in meta:
        xll = meta["xllcenter"] - meta["cellsize"] / 2
        yll = meta["yllcenter"] - meta["cellsize"] / 2
    else:
        raise ValueError("Header ASC sin xllcorner ni xllcenter")

    print(f"    Dimensiones: {ncols}×{nrows}  cellsize={meta['cellsize']:.1f}m")
    lat_sw, lon_sw = utm30n_a_latlon(xll, yll)
    lat_ne, lon_ne = utm30n_a_latlon(xll + ncols*meta["cellsize"],
                                      yll + nrows*meta["cellsize"])
    print(f"    Cobertura: lat [{lat_sw:.4f}°, {lat_ne:.4f}°]  "
          f"lon [{lon_sw:.4f}°, {lon_ne:.4f}°]")

    # Verificar que Alsasua está dentro
    if not (lat_sw <= LAT_CENTRO <= lat_ne and lon_sw <= LON_CENTRO <= lon_ne):
        print(f"  ⚠ Alsasua (lat={LAT_CENTRO}, lon={LON_CENTRO}) "
              f"no está dentro de este archivo.")
        print(f"    Puede que necesites otra hoja. Continuando con el centro del archivo.")

    # Leer los datos (ignorar cabecera ya procesada)
    data = np.loadtxt(ruta, skiprows=header_lines, dtype=np.float32)

    if data.shape != (nrows, ncols):
        # Intentar reshape si loadtxt aplanó todo
        data = data.reshape(nrows, ncols)

    # NODATA → 0
    data[data == nodata] = 0.0
    data[data < 0]       = 0.0

    # El ASC tiene fila 0 = norte (igual que la convención Unity RAW)
    # No hay que invertir aquí; la inversión la hace exportar_raw_unity.

    meta_out = {
        "ncols":    ncols,
        "nrows":    nrows,
        "xllcorner": xll,
        "yllcorner": yll,
        "cellsize": meta["cellsize"],
        "nodata":   nodata,
        "crs":      "EPSG:25830 (UTM Zone 30N / ETRS89)",
    }
    print(f"    Elevaciones: min={data.min():.0f}m  max={data.max():.0f}m  "
          f"({data.nbytes//1024//1024} MB en RAM)")
    return data, meta_out


def leer_tif(ruta: str) -> tuple[np.ndarray, dict]:
    """Lee un GeoTIFF usando gdal (requiere pip install gdal)."""
    try:
        from osgeo import gdal, osr
    except ImportError:
        raise ImportError(
            "gdal no instalado. Para leer .tif instala:\n"
            "  pip install gdal\n"
            "O convierte el .tif a .asc con QGIS:\n"
            "  Raster → Convertir → Traducir (raster to raster) → formato AAIGrid")

    print(f"  Leyendo {os.path.basename(ruta)} con gdal...")
    ds = gdal.Open(ruta)
    if ds is None:
        raise IOError(f"gdal no pudo abrir '{ruta}'")

    band    = ds.GetRasterBand(1)
    nodata  = band.GetNoDataValue() or -9999.0
    data    = band.ReadAsArray().astype(np.float32)
    data[data == nodata] = 0.0
    data[np.isnan(data)] = 0.0
    data[data < 0]       = 0.0

    gt = ds.GetGeoTransform()   # (x_min, cellsize_x, 0, y_max, 0, -cellsize_y)
    cellsize = abs(gt[1])
    xll = gt[0]
    yll = gt[3] + data.shape[0] * gt[5]   # gt[5] es negativo

    # Detectar si está en UTM o en lat/lon
    srs = osr.SpatialReference()
    srs.ImportFromWkt(ds.GetProjection())
    is_geographic = bool(srs.IsGeographic())

    if is_geographic:
        # Ya en lat/lon — adaptar meta
        meta_out = {
            "ncols": data.shape[1], "nrows": data.shape[0],
            "xllcorner": xll, "yllcorner": yll,
            "cellsize": cellsize, "nodata": nodata,
            "crs": "Geographic (lat/lon)",
        }
    else:
        meta_out = {
            "ncols": data.shape[1], "nrows": data.shape[0],
            "xllcorner": xll, "yllcorner": yll,
            "cellsize": cellsize, "nodata": nodata,
            "crs": "EPSG:25830 (UTM Zone 30N / ETRS89)",
        }

    print(f"    Dimensiones: {data.shape[1]}×{data.shape[0]}  cellsize={cellsize:.1f}m")
    print(f"    Elevaciones: min={data.min():.0f}m  max={data.max():.0f}m")
    return data, meta_out


# ═══════════════════════════════════════════════════════════════════════════
#  RECORTAR Y REMUESTREAR al área de Alsasua
# ═══════════════════════════════════════════════════════════════════════════

def recortar_a_alsasua(data: np.ndarray, meta: dict,
                        radio_m: float, res: int) -> np.ndarray:
    """
    Recorta el raster al área de Alsasua (radio_m desde el centro)
    y lo remuestrea bilinealmente a res×res píxeles.

    Soporta rasters en UTM Zone 30N o en lat/lon.
    """
    cellsize = meta["cellsize"]
    xll      = meta["xllcorner"]
    yll      = meta["yllcorner"]
    nrows    = meta["nrows"]
    ncols    = meta["ncols"]
    crs      = meta.get("crs", "")

    # Calcular el centro de Alsasua en coordenadas del raster
    if "Geographic" in crs or "lat" in crs.lower():
        # Coordenadas en grados
        cx_world = LON_CENTRO
        cy_world = LAT_CENTRO
    else:
        # UTM Zone 30N
        cx_world, cy_world = latlon_a_utm30n(LAT_CENTRO, LON_CENTRO)

    print(f"  Centro Alsasua en coordenadas raster: ({cx_world:.1f}, {cy_world:.1f})")

    # Radio en píxeles
    radio_px = radio_m / cellsize

    # Columna y fila del centro (fila 0 = norte en ASC)
    col_c = (cx_world - xll) / cellsize
    # yll es el borde SUR, fila 0 es el borde NORTE del ASC
    row_c = nrows - (cy_world - yll) / cellsize

    print(f"  Posición en raster: col={col_c:.1f}, row={row_c:.1f}  "
          f"(de {ncols}×{nrows})")

    # BBox en píxeles (con margen)
    col0 = max(0, int(col_c - radio_px))
    col1 = min(ncols, int(col_c + radio_px))
    row0 = max(0, int(row_c - radio_px))
    row1 = min(nrows, int(row_c + radio_px))

    if col1 <= col0 or row1 <= row0:
        print("  ⚠ El recorte está fuera del raster. Usando el raster completo.")
        recorte = data
    else:
        recorte = data[row0:row1, col0:col1]
        print(f"  Recorte: {recorte.shape[1]}×{recorte.shape[0]} px "
              f"({recorte.shape[1]*cellsize:.0f}m × {recorte.shape[0]*cellsize:.0f}m)")

    # Remuestrear a res×res con interpolación bilineal (via zoom de numpy)
    if recorte.shape[0] != res or recorte.shape[1] != res:
        src_rows, src_cols = recorte.shape
        # Crear grid de coordenadas destino
        zi = np.linspace(0, src_rows - 1, res)
        xi = np.linspace(0, src_cols - 1, res)
        # Bilineal manual (evita dependencia de scipy)
        zi0 = np.floor(zi).astype(int).clip(0, src_rows - 2)
        xi0 = np.floor(xi).astype(int).clip(0, src_cols - 2)
        tz  = (zi - zi0)[:, None]   # (res, 1)
        tx  = (xi - xi0)[None, :]   # (1, res)

        v00 = recorte[zi0[:, None], xi0[None, :]]
        v10 = recorte[zi0[:, None], (xi0+1)[None, :]]
        v01 = recorte[(zi0+1)[:, None], xi0[None, :]]
        v11 = recorte[(zi0+1)[:, None], (xi0+1)[None, :]]

        resultado = (v00*(1-tz)*(1-tx) + v10*(1-tz)*tx +
                     v01*tz*(1-tx)     + v11*tz*tx).astype(np.float32)
        print(f"  Remuestreado: {src_cols}×{src_rows} → {res}×{res}")
    else:
        resultado = recorte.astype(np.float32)

    print(f"  Elevaciones finales: min={resultado.min():.0f}m  "
          f"max={resultado.max():.0f}m  (AMSL)")

    # Restar la altitud del valle para que el suelo quede a 0
    resultado -= ALT_CENTRO
    resultado  = np.maximum(resultado, 0.0)
    print(f"  Sobre el valle:     min={resultado.min():.0f}m  "
          f"max={resultado.max():.0f}m")
    return resultado


# ═══════════════════════════════════════════════════════════════════════════
#  MODELO ANALÍTICO (fallback sin archivos externos)
# ═══════════════════════════════════════════════════════════════════════════

CRESTAS = [
    (-700,   200,  220, 550, 700, -15),
    (-900,  -200,  180, 400, 550,  10),
    ( 750,   650,  260, 500, 700,  20),
    (  50,  1400,  130, 900, 600,   0),
    (-600,   900,  200, 500, 600, -25),
    (   0, -1600,  120, 800, 500,   0),
    (-1100,-1400,  200, 600, 700,  30),
    ( 900,  -400,  160, 450, 600,  10),
    (1200,  1200,  190, 700, 900,  35),
]

def altura_analitica(wx, wz, radio_valle_flat=300.0, suavizado=1.4,
                     seed_x=42.0, seed_z=137.0):
    alt_monte = 0.0
    for (cx, cz, alt, rx, rz, ang) in CRESTAS:
        dx, dz = wx - cx, wz - cz
        if ang != 0:
            rad = math.radians(ang)
            cosA, sinA = math.cos(rad), math.sin(rad)
            dx, dz = dx*cosA - dz*sinA, dx*sinA + dz*cosA
        alt_monte += alt * math.exp(-((dx/rx)**2 + (dz/rz)**2))
    dist = math.sqrt(wx*wx + wz*wz)
    if dist < radio_valle_flat:
        factor_flat = 0.0
    else:
        t = max(0.0, min(1.0, (dist - radio_valle_flat) / (radio_valle_flat * suavizado)))
        factor_flat = t*t*(3 - 2*t)
    alt_monte *= factor_flat
    ruido = (math.sin(wx*0.00035*6.28 + seed_x) * math.cos(wz*0.00035*6.28 + seed_z)
             * 18.0
           + math.sin(wx*0.0018*6.28 + seed_x+3) * math.cos(wz*0.0018*6.28 + seed_z+2)
             * 4.5)
    ruido *= min(1.0, dist / (radio_valle_flat * 2.0))
    return max(0.0, alt_monte + ruido)

def generar_heightmap_analitico(res, radio_m):
    half = radio_m
    print(f"  Generando heightmap analítico {res}×{res} ({half*2:.0f}m × {half*2:.0f}m)...")
    h = np.zeros((res, res), dtype=np.float32)
    for zi in range(res):
        wz = (zi / (res-1) - 0.5) * half * 2
        for xi in range(res):
            wx = (xi / (res-1) - 0.5) * half * 2
            h[zi, xi] = altura_analitica(wx, wz)
    return h


# ═══════════════════════════════════════════════════════════════════════════
#  DESCARGA WCS IDENA (automática)
# ═══════════════════════════════════════════════════════════════════════════

def generar_heightmap_idena(res, radio_m):
    if not HAS_REQUESTS:
        print("  ✗ 'requests' no instalado. Usa: pip install requests")
        return None
    bbox = (LAT_CENTRO - radio_m/M_POR_GRADO_LAT,
            LON_CENTRO - radio_m/M_POR_GRADO_LON,
            LAT_CENTRO + radio_m/M_POR_GRADO_LAT,
            LON_CENTRO + radio_m/M_POR_GRADO_LON)
    params = {
        "SERVICE":  "WCS", "VERSION": "1.0.0", "REQUEST": "GetCoverage",
        "COVERAGE": "MDT5", "CRS": "EPSG:4326",
        "BBOX":     f"{bbox[1]},{bbox[0]},{bbox[3]},{bbox[2]}",
        "WIDTH":    str(res), "HEIGHT": str(res), "FORMAT": "image/tiff",
    }
    print("  → Descargando DEM desde IDENA WCS (MDT 5m)...")
    try:
        resp = requests.get(IDENA_WCS, params=params, timeout=120,
                            headers={"User-Agent": "AlsasuaSimulator/1.0"})
        resp.raise_for_status()
        try:
            from osgeo import gdal
            import tempfile
            with tempfile.NamedTemporaryFile(suffix=".tif", delete=False) as f:
                f.write(resp.content)
                tmp = f.name
            ds   = gdal.Open(tmp)
            band = ds.GetRasterBand(1)
            arr  = band.ReadAsArray().astype(np.float32)
            nd   = band.GetNoDataValue()
            if nd is not None:
                arr[arr == nd] = 0.0
            os.unlink(tmp)
            print(f"  ✓ DEM IDENA: {arr.shape[1]}×{arr.shape[0]}px  "
                  f"min={arr.min():.0f}m  max={arr.max():.0f}m AMSL")
            arr -= ALT_CENTRO
            return np.maximum(arr, 0.0)
        except ImportError:
            os.makedirs("Assets/Terrain", exist_ok=True)
            tif_out = "Assets/Terrain/alsasua_dem_idena.tif"
            with open(tif_out, "wb") as f:
                f.write(resp.content)
            print(f"  ⚠ gdal no instalado. GeoTIFF guardado en {tif_out}")
            print("    Úsalo con:  python DescargadorIDENA.py --archivo Assets/Terrain/alsasua_dem_idena.tif")
            return None
    except Exception as e:
        print(f"  ✗ IDENA no accesible: {e}")
        return None


# ═══════════════════════════════════════════════════════════════════════════
#  EXPORTAR RAW 16-BIT (formato Unity Terrain)
# ═══════════════════════════════════════════════════════════════════════════

def exportar_raw_unity(heightmap: np.ndarray, ruta_raw: str,
                       alt_max: float = 600.0) -> dict:
    """
    Exporta el heightmap como RAW 16-bit little-endian.
    El array de entrada tiene fila 0 = norte (convención ASC).
    Unity espera fila 0 = sur → invertimos el eje Y aquí.
    """
    res         = heightmap.shape[0]
    alt_min_val = float(heightmap.min())
    alt_max_val = float(heightmap.max())

    h_norm = np.clip(heightmap / alt_max, 0.0, 1.0)
    # Invertir eje Y: fila 0 norte → fila 0 sur (Unity)
    h_norm = h_norm[::-1, :]
    h_u16  = (h_norm * 65535).astype(np.uint16)

    os.makedirs(os.path.dirname(ruta_raw) or ".", exist_ok=True)
    h_u16.tofile(ruta_raw)

    size_kb = os.path.getsize(ruta_raw) / 1024
    print(f"  ✓ RAW guardado: {ruta_raw}  ({size_kb:.0f} KB)")

    return {
        "generado":          time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "resolucion_px":     res,
        "alt_min_m":         round(alt_min_val, 1),
        "alt_max_real_m":    round(alt_max_val, 1),
        "alt_max_unity_m":   alt_max,
        "lat_centro":        LAT_CENTRO,
        "lon_centro":        LON_CENTRO,
        "instrucciones_unity": {
            "paso_1": "Terrain component → Import Raw",
            "paso_2": f"Seleccionar {os.path.basename(ruta_raw)}",
            "paso_3": "Depth: 16 bit  |  Byte Order: Windows  |  Flip: Y-axis = OFF  (ya invertido)",
            "paso_4": f"Width: {res}  Height: {res}",
            "paso_5": f"Terrain Height (Inspector): {alt_max} m",
        }
    }


# ═══════════════════════════════════════════════════════════════════════════
#  MAIN
# ═══════════════════════════════════════════════════════════════════════════

def main():
    parser = argparse.ArgumentParser(
        description="Convierte MDT05 IDENA Navarra a heightmap RAW para Unity",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
EJEMPLOS:
  # Desde archivo ASC descargado de filescartografia.navarra.es:
  python DescargadorIDENA.py --archivo PNOA_MDT05_30TWN84.asc --res 513

  # Desde GeoTIFF (requiere gdal):
  python DescargadorIDENA.py --archivo dem_alsasua.tif --res 1025

  # Sin archivos (modelo analítico):
  python DescargadorIDENA.py --analitico --res 513

  # Descarga automática via WCS de IDENA:
  python DescargadorIDENA.py --res 513 --radio 2048
        """)

    parser.add_argument("--archivo",  type=str,   default=None,
        metavar="RUTA",
        help="Ruta al archivo MDT local (.asc o .tif descargado de IDENA)")
    parser.add_argument("--radio",    type=float, default=2048.0,
        help="Radio en metros desde el centro de Alsasua (default: 2048)")
    parser.add_argument("--res",      type=int,   default=513,
        choices=[257, 513, 1025, 2049],
        help="Resolución del heightmap de salida (default: 513)")
    parser.add_argument("--output",   type=str,   default="Assets/Terrain",
        help="Carpeta de salida (default: Assets/Terrain)")
    parser.add_argument("--analitico", action="store_true",
        help="Forzar modelo analítico (sin descargar ni leer archivos)")
    args = parser.parse_args()

    alt_max = 600.0

    print("═══════════════════════════════════════════════════════════")
    print("  DescargadorIDENA — DEM Valle de Burunda / Alsasua")
    print(f"  Radio: {args.radio:.0f}m  |  Res: {args.res}²  |  Salida: {args.output}")
    if args.archivo:
        print(f"  Archivo: {args.archivo}")
    print("═══════════════════════════════════════════════════════════\n")

    heightmap = None

    # ── Modo 1: archivo local (.asc o .tif) ──────────────────────────────
    if args.archivo and not args.analitico:
        ruta = args.archivo
        if not os.path.exists(ruta):
            sys.exit(f"ERROR: No se encuentra '{ruta}'")
        ext = os.path.splitext(ruta)[1].lower()
        try:
            if ext == ".asc":
                data, meta = leer_asc(ruta)
            elif ext in (".tif", ".tiff"):
                data, meta = leer_tif(ruta)
            else:
                sys.exit(f"ERROR: formato no soportado '{ext}'. Usa .asc o .tif")

            print(f"\n  Recortando al área de Alsasua (radio={args.radio:.0f}m)...")
            heightmap = recortar_a_alsasua(data, meta, args.radio, args.res)
            del data   # liberar memoria
        except Exception as e:
            print(f"  ✗ Error leyendo archivo: {e}")
            print("  Usando modelo analítico como fallback.\n")

    # ── Modo 2: descarga WCS IDENA ────────────────────────────────────────
    elif not args.analitico:
        heightmap = generar_heightmap_idena(args.res, args.radio)

    # ── Modo 3: modelo analítico ──────────────────────────────────────────
    if heightmap is None:
        print("\n  Generando modelo analítico del Valle de Burunda...")
        heightmap = generar_heightmap_analitico(args.res, args.radio)
        print(f"  ✓ Analítico: min={heightmap.min():.0f}m  max={heightmap.max():.0f}m")

    # ── Exportar ──────────────────────────────────────────────────────────
    print("\n  Exportando RAW 16-bit para Unity...")
    ruta_raw  = os.path.join(args.output, "alsasua_heightmap.raw")
    ruta_json = os.path.join(args.output, "alsasua_heightmap.json")

    meta_out = exportar_raw_unity(heightmap, ruta_raw, alt_max)
    meta_out["radio_m"]       = args.radio
    meta_out["terreno_size_m"] = args.radio * 2
    meta_out["fuente"]        = (args.archivo or
                                  ("analitico" if args.analitico else "IDENA WCS"))

    with open(ruta_json, "w", encoding="utf-8") as f:
        json.dump(meta_out, f, ensure_ascii=False, indent=2)
    print(f"  ✓ Metadatos: {ruta_json}")

    print("\n  INSTRUCCIONES PARA UNITY:")
    for k, v in meta_out["instrucciones_unity"].items():
        print(f"    {k}: {v}")
    print()


if __name__ == "__main__":
    main()
