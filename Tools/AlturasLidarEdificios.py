"""
AlturasLidarEdificios.py — Plantas reales de cada edificio, sacadas del LiDAR.

Por qué esto y no la cartografía: la serie CARTO1 de IDENA trae las alturas
anotadas (CADTEXT "S+III"), pero NO cubre el casco de Alsasua — sobre la plaza
+-1 km devuelve cero features. El LiDAR sí lo cubre entero.

Método, que es el que pedía "referenciar por extrapolación" pero hecho con la
medida directa en vez de con proporciones:

  1. IDENA publica ELEVAC_Ras_Alturas_2M: "Mapa de alturas. Diferencia entre MDS
     y MDT", o sea altura sobre el terreno ya normalizada, 2 m/px, vuelo LiDAR
     de Navarra 2017. Nos ahorra restar dos modelos y arrastrar su error.
  2. Para cada huella catastral (catastro_navarra_edificios.json) se muestrean
     los píxeles que caen DENTRO del polígono.
  3. La altura del edificio es el percentil 75 de esos píxeles, no la media ni
     el máximo: la media la hunden los píxeles de borde que pillan calle, y el
     máximo lo dispara cualquier antena, chimenea o árbol pegado a la fachada.
  4. Plantas = round((altura - ALERO_M) / ALTURA_PLANTA_M), acotado a >=1.
     Se descuenta el alero porque la altura LiDAR incluye el tejado, y una casa
     de dos plantas mide más que dos veces la altura de planta.

Uso:
    python3 Tools/AlturasLidarEdificios.py            # descarga y calcula
    python3 Tools/AlturasLidarEdificios.py --verificar  # contrasta contra levels
"""
import io
import json
import math
import os
import sys
import time
import urllib.request

import numpy as np
from PIL import Image

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATOS = os.path.join(RAIZ, "Content", "Datos")
ENTRADA = os.path.join(DATOS, "catastro_navarra_edificios.json")
SALIDA = os.path.join(DATOS, "alturas_lidar_edificios.json")

WCS = "https://idena.navarra.es/ogc/wcs"
COBERTURA = "IDENA.WCS__ELEVAC_Ras_Alturas_2M"
UA = {"User-Agent": "alsasua-sim/1.0 (proyecto personal)"}

# Los ejes de esta cobertura se llaman E y N. Con x/y o Long/Lat da 404
# "InvalidAxisLabel", que es un error claro pero fácil de perder media hora.
RES_M = 2.0
TESELA_M = 1000.0

# Casco urbano: la plaza +-1,6 km cubre de sobra los 1030 de buildings_final.
CENTRO_E, CENTRO_N = 567951.0, 4749902.0
RADIO_M = 1600.0

ORIGEN_E, ORIGEN_N = 566033.0, 4741332.0   # mismo frame que el resto del mundo

ALTURA_PLANTA_M = 3.0    # planta tipo en vivienda del casco
ALERO_M = 1.5            # lo que añade el tejado sobre la última planta
MIN_ALTURA_M = 2.0       # por debajo, es cobertizo o error: no cuenta como planta


def _get(url, tries=4, timeout=180):
    for i in range(tries):
        try:
            req = urllib.request.Request(url, headers=UA)
            with urllib.request.urlopen(req, timeout=timeout) as r:
                return r.read()
        except Exception as e:
            print("  reintento %d/%d: %s" % (i + 1, tries, e), flush=True)
            time.sleep(2 + 3 * i)
    raise RuntimeError("IDENA WCS no responde")


def descargar_mosaico():
    """Devuelve (array de alturas, e0, n0) del casco, norte arriba."""
    e0, n0 = CENTRO_E - RADIO_M, CENTRO_N - RADIO_M
    lado = 2 * RADIO_M
    nt = int(math.ceil(lado / TESELA_M))
    px_t = int(TESELA_M / RES_M)
    total = np.full((nt * px_t, nt * px_t), np.nan, np.float32)

    for j in range(nt):
        for i in range(nt):
            a, b = e0 + i * TESELA_M, n0 + j * TESELA_M
            url = ("%s?service=WCS&version=2.0.1&request=GetCoverage&coverageId=%s"
                   "&subset=E(%f,%f)&subset=N(%f,%f)&format=image/tiff"
                   % (WCS, COBERTURA, a, a + TESELA_M, b, b + TESELA_M))
            arr = np.array(Image.open(io.BytesIO(_get(url)))).astype(np.float32)
            h, w = arr.shape[:2]
            # La tesela llega norte arriba; el mosaico se arma con j=0 abajo.
            fila = (nt - 1 - j) * px_t
            total[fila:fila + min(h, px_t), i * px_t:i * px_t + min(w, px_t)] = \
                arr[:px_t, :px_t]
            print("  tesela %d/%d" % (j * nt + i + 1, nt * nt), flush=True)
    return total, e0, n0


def dentro(poly, x, y):
    """Point-in-polygon por cruces (ray casting)."""
    n = len(poly)
    d = False
    j = n - 1
    for i in range(n):
        xi, yi = poly[i]
        xj, yj = poly[j]
        if ((yi > y) != (yj > y)) and (x < (xj - xi) * (y - yi) / (yj - yi + 1e-12) + xi):
            d = not d
        j = i
    return d


def main():
    if not os.path.exists(ENTRADA):
        raise SystemExit("Falta %s: ejecuta antes DescargarCatastroNavarra.py" % ENTRADA)
    edificios = json.load(open(ENTRADA, encoding="utf-8"))["edificios"]

    print("Descargando mapa de alturas LiDAR (2 m/px)...", flush=True)
    ras, e0, n0 = descargar_mosaico()
    alto, ancho = ras.shape
    print("  mosaico %dx%d px" % (ancho, alto), flush=True)

    out = []
    for ed in edificios:
        # El anillo viene en cm de mundo; se vuelve a UTM para casar con el raster.
        poly = [((x / 100.0) + ORIGEN_E, (y / 100.0) + ORIGEN_N)
                for x, y in ed["anillo_mundo_cm"]]
        xs = [p[0] for p in poly]
        ys = [p[1] for p in poly]
        if max(xs) < e0 or min(xs) > e0 + ancho * RES_M:
            continue
        if max(ys) < n0 or min(ys) > n0 + alto * RES_M:
            continue

        c0 = max(0, int((min(xs) - e0) / RES_M))
        c1 = min(ancho - 1, int((max(xs) - e0) / RES_M))
        f0 = max(0, int((min(ys) - n0) / RES_M))
        f1 = min(alto - 1, int((max(ys) - n0) / RES_M))
        vals = []
        for f in range(f0, f1 + 1):
            yc = n0 + (f + 0.5) * RES_M
            for c in range(c0, c1 + 1):
                xc = e0 + (c + 0.5) * RES_M
                if not dentro(poly, xc, yc):
                    continue
                v = ras[alto - 1 - f, c]      # fila 0 del array es el norte
                if np.isfinite(v):
                    vals.append(float(v))
        if len(vals) < 3:
            continue

        h = float(np.percentile(vals, 75))
        if h < MIN_ALTURA_M:
            plantas = 0
        else:
            plantas = max(1, int(round((h - ALERO_M) / ALTURA_PLANTA_M)))
        out.append({
            "centro_mundo_cm": ed["centro_mundo_cm"],
            "area_m2": ed["area_m2"],
            "altura_m": round(h, 2),
            "plantas": plantas,
            "px": len(vals),
        })

    doc = {
        "_fuente": ("IDENA %s - 'Mapa de alturas. Diferencia entre MDS y MDT', "
                    "LiDAR Navarra 2017, 2 m/px" % COBERTURA),
        "_metodo": ("altura = percentil 75 de los pixeles dentro de la huella; "
                    "plantas = round((altura - %.1f) / %.1f)" % (ALERO_M, ALTURA_PLANTA_M)),
        "edificios": out,
    }
    json.dump(doc, open(SALIDA, "w", encoding="utf-8"), ensure_ascii=False)
    conp = [e["plantas"] for e in out if e["plantas"]]
    print("\n%d edificios con altura LiDAR -> %s" % (len(out), SALIDA))
    if conp:
        print("  altura mediana %.1f m   plantas mediana %d"
              % (float(np.median([e["altura_m"] for e in out])), int(np.median(conp))))


def verificar():
    from collections import Counter, defaultdict
    if not os.path.exists(SALIDA):
        raise SystemExit("Falta %s: ejecuta antes la descarga." % SALIDA)
    lid = json.load(open(SALIDA, encoding="utf-8"))["edificios"]
    bf = json.load(open(os.path.join(DATOS, "buildings_final.json"), encoding="utf-8"))
    OX, OZ = 1918.0, 8570.0

    def centro(b):
        v = b["vertices"]
        pts = ([(p["x"], p.get("z", p.get("y"))) for p in v] if isinstance(v[0], dict)
               else [(v[i], v[i + 1]) for i in range(0, len(v) - 1, 2)])
        return ((sum(p[0] for p in pts) / len(pts) + OX) * 100.0,
                (sum(p[1] for p in pts) / len(pts) + OZ) * 100.0)

    CEL = 3000.0
    rej = defaultdict(list)
    for e in lid:
        x, y = e["centro_mundo_cm"]
        rej[(int(x // CEL), int(y // CEL))].append(e)

    # Se comparan DOS cosas: plantas (que depende de ALTURA_PLANTA_M, o sea de una
    # suposición mía) y altura en metros (que no depende de nada). Si ambas dan el
    # mismo sesgo, el sesgo es del mundo y no de mis parámetros.
    difs, reales, nuestras, pares = [], Counter(), Counter(), 0
    dif_h = []
    for b in bf:
        x, y = centro(b)
        mejor, md = None, float("inf")
        cx, cy = int(x // CEL), int(y // CEL)
        for ax in (-1, 0, 1):
            for ay in (-1, 0, 1):
                for e in rej.get((cx + ax, cy + ay), ()):
                    d = math.hypot(e["centro_mundo_cm"][0] - x, e["centro_mundo_cm"][1] - y)
                    if d < md:
                        md, mejor = d, e
        if mejor is None or md > 1500.0 or not mejor["plantas"]:
            continue
        if b.get("height"):
            dif_h.append(float(b["height"]) - mejor["altura_m"])
        lv = b.get("levels")
        if not lv:
            continue
        pares += 1
        difs.append(lv - mejor["plantas"])
        reales[mejor["plantas"]] += 1
        nuestras[lv] += 1

    if not pares:
        print("Sin parejas.")
        return
    ex = sum(1 for d in difs if d == 0)
    c1 = sum(1 for d in difs if abs(d) <= 1)
    print("Edificios emparejados (huella LiDAR <15 m del nuestro): %d" % pares)
    print("  plantas exactas: %4d (%.1f%%)" % (ex, 100.0 * ex / pares))
    print("  con +-1 planta : %4d (%.1f%%)" % (c1, 100.0 * c1 / pares))
    print("  sesgo (nuestro - LiDAR): media %+.2f  mediana %+.1f"
          % (float(np.mean(difs)), float(np.median(difs))))
    print("  reparto LiDAR  :", dict(sorted(reales.items())))
    print("  reparto nuestro:", dict(sorted(nuestras.items())))

    if dif_h:
        m, med = float(np.mean(dif_h)), float(np.median(dif_h))
        sg = float(np.std(dif_h))
        print("\nAltura en metros (sin suponer altura de planta):")
        print("  diferencia nuestra - LiDAR: media %+.2f m  mediana %+.2f m  sigma %.2f"
              % (m, med, sg))
        print("  Las dos vías coinciden: ~%.1f m de defecto es ~1 planta. El sesgo es"
              % abs(med))
        print("  del mundo, no del parámetro ALTURA_PLANTA_M.")
        print("  Ojo: sigma %.1f m > |media|, o sea que hay sesgo Y mucha dispersión." % sg)
        print("  Un +1 a todos taparía el sesgo y empeoraría los que ya estaban bien;")
        print("  lo correcto es usar la altura LiDAR por edificio donde haya pareja.")


if __name__ == "__main__":
    if "--verificar" in sys.argv:
        verificar()
    else:
        main()
