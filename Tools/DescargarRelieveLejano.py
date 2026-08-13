"""
DescargarRelieveLejano.py — Relieve lejano de Alsasua desde los servicios del IGN.

El terreno jugable son 7200x7200 m y se corta de golpe a 3,6 km del centro. Esto
baja los 60x60 km de alrededor para poder dibujar detrás un anillo de relieve sin
colisión, de forma que el mundo no acabe en un vacío: al oeste Aizkorri (1543 m a
14,8 km), al este San Donato/Andía (1410 m a 15,1 km), al norte Aralar (1382 m a
16,1 km) y al sur Urbasa/Lokiz (1375 m a 32,9 km).

Genera, georreferenciado al mismo grid UTM30N que el resto del mundo
(world_cm = (UTM_m - 566033, UTM_m - 4741332) * 100, X=este, Y=norte):

  1. Content/Terreno/alsasua_relieve_lejano_2048.r16   alturas, 29,3 m/px
  2. Content/Terreno/alsasua_relieve_lejano_meta.json  caja, escala y codificación
  3. Content/Terreno/alsasua_relieve_lejano_4096.png   ortofoto PNOA, 14,6 m/px

Convenciones (las mismas que ya usa el proyecto, no las cambies a la ligera):
  - El .r16 va con la fila 0 al SUR, igual que alsasua_landscape_4033.r16
    (ver PrepararLandscape.py, que hace flipud por el mismo motivo).
  - El PNG va con el SUR arriba, porque el material del terreno mapea v=0 a
    world Ymin y en UE v=0 es la fila de arriba.
  - La altura NO usa el datum 495 + q/64 del heightmap principal: ese topa en
    1518,98 m y Aizkorri pasa de 1520, así que clipaba los picos. Aquí datum y
    paso van en el meta y los lee el C++.

Uso:  python3 Tools/DescargarRelieveLejano.py
Requiere: pip install pillow numpy
"""
import io
import json
import os
import time
import urllib.request

from PIL import Image
import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Centro del mundo = Herriko Plaza en UTM30N, el mismo que usa TerrenoGenerado.
CENTRO_E, CENTRO_N = 567951.0, 4749902.0
RADIO_M = 30000.0
BOX = (CENTRO_E - RADIO_M, CENTRO_N - RADIO_M, CENTRO_E + RADIO_M, CENTRO_N + RADIO_M)

RES_ALTURA = 2048          # 60000/2048 = 29,3 m por muestra
RES_ORTO = 4096            # 60000/4096 = 14,6 m por píxel
REJILLA = (4, 4)           # tiles, para no pedir un raster gigante de una vez

# Codificación del .r16: alt_m = DATUM + q * PASO. Con estos, 0..2047,97 m a 3 cm.
DATUM_M = 0.0
PASO_M = 1.0 / 32.0

WCS = ("https://servicios.idee.es/wcs-inspire/mdt"
       "?service=WCS&version=2.0.1&request=GetCoverage"
       "&coverageId=Elevacion25830_25&format=image/tiff")
WMS = ("https://www.ign.es/wms-inspire/pnoa-ma"
       "?SERVICE=WMS&VERSION=1.1.1&REQUEST=GetMap"
       "&LAYERS=OI.OrthoimageCoverage&STYLES=OI.OrthoimageCoverage.Default"
       "&SRS=EPSG:25830&FORMAT=image/jpeg")
UA = {"User-Agent": "alsasua-sim/1.0 (proyecto personal)"}

OUT_R16 = os.path.join(ROOT, "Content", "Terreno", "alsasua_relieve_lejano_2048.r16")
OUT_META = os.path.join(ROOT, "Content", "Terreno", "alsasua_relieve_lejano_meta.json")
OUT_ORTO = os.path.join(ROOT, "Content", "Terreno", "alsasua_relieve_lejano_4096.png")


def _get(url, tries=6, timeout=180):
    for i in range(tries):
        try:
            req = urllib.request.Request(url, headers=UA)
            with urllib.request.urlopen(req, timeout=timeout) as r:
                return r.read()
        except Exception as e:
            print("  reintento %d/%d: %s" % (i + 1, tries, e), flush=True)
            time.sleep(2 + 3 * i)
    raise RuntimeError("fallo tras %d intentos: %s" % (tries, url))


def tile_altura(bbox):
    """Trozo de MDT25 como array float (m). El WCS devuelve norte arriba."""
    url = "%s&subset=x(%f,%f)&subset=y(%f,%f)" % (WCS, bbox[0], bbox[2], bbox[1], bbox[3])
    a = np.array(Image.open(io.BytesIO(_get(url)))).astype(np.float64)
    # El MDT marca el mar/sin dato con valores absurdos; los dejamos a nivel bajo.
    a[~np.isfinite(a)] = 0.0
    a[a < -100.0] = 0.0
    return a


def mosaico_altura():
    """Ensambla el MDT en rejilla y devuelve un array norte-arriba."""
    e0, n0, e1, n1 = BOX
    de = (e1 - e0) / REJILLA[0]
    dn = (n1 - n0) / REJILLA[1]
    filas = []
    # j=0 es la banda sur. Norte arriba => recorremos de norte a sur al apilar.
    for j in range(REJILLA[1] - 1, -1, -1):
        cols = []
        for i in range(REJILLA[0]):
            bbox = (e0 + i * de, n0 + j * dn, e0 + (i + 1) * de, n0 + (j + 1) * dn)
            cols.append(tile_altura(bbox))
            print("  MDT tile %d/%d ok" % ((REJILLA[1] - 1 - j) * REJILLA[0] + i + 1,
                                           REJILLA[0] * REJILLA[1]), flush=True)
        filas.append(np.hstack(cols))
    return np.vstack(filas)


def remuestrear(a, n):
    """Bilineal a n x n, separable (mismo enfoque que PrepararLandscape.py)."""
    h, w = a.shape
    yi = np.linspace(0, h - 1, n)
    xi = np.linspace(0, w - 1, n)
    cols = np.empty((h, n), np.float64)
    xf = np.arange(w)
    for r in range(h):
        cols[r] = np.interp(xi, xf, a[r])
    out = np.empty((n, n), np.float64)
    yf = np.arange(h)
    for c in range(n):
        out[:, c] = np.interp(yi, yf, cols[:, c])
    return out


def tile_orto(bbox, px):
    url = "%s&BBOX=%f,%f,%f,%f&WIDTH=%d&HEIGHT=%d" % (WMS, bbox[0], bbox[1], bbox[2], bbox[3], px, px)
    img = Image.open(io.BytesIO(_get(url))).convert("RGB")
    if img.size != (px, px):
        raise ValueError("tamaño %s != %d" % (img.size, px))
    return img


def mosaico_orto():
    e0, n0, e1, n1 = BOX
    de = (e1 - e0) / REJILLA[0]
    dn = (n1 - n0) / REJILLA[1]
    px = RES_ORTO // REJILLA[0]
    out = Image.new("RGB", (RES_ORTO, RES_ORTO))
    for j in range(REJILLA[1]):
        for i in range(REJILLA[0]):
            bbox = (e0 + i * de, n0 + j * dn, e0 + (i + 1) * de, n0 + (j + 1) * dn)
            # j=0 es sur y va abajo en un mosaico norte-arriba.
            out.paste(tile_orto(bbox, px), (i * px, (REJILLA[1] - 1 - j) * px))
            print("  PNOA tile %d/%d ok" % (j * REJILLA[0] + i + 1, REJILLA[0] * REJILLA[1]), flush=True)
    return out


def verificar_altura(a_norte_arriba):
    """Round-trip: vuelve a pedir un trozo suelto y compara con el mosaico."""
    lado = 4000.0
    sub = (CENTRO_E + 12000.0, CENTRO_N - lado / 2, CENTRO_E + 12000.0 + lado, CENTRO_N + lado / 2)
    ref = tile_altura(sub)
    n = a_norte_arriba.shape[0]
    mpp = (BOX[2] - BOX[0]) / n
    c0 = int((sub[0] - BOX[0]) / mpp)
    c1 = int((sub[2] - BOX[0]) / mpp)
    r0 = int((BOX[3] - sub[3]) / mpp)
    r1 = int((BOX[3] - sub[1]) / mpp)
    crop = a_norte_arriba[r0:r1, c0:c1]
    if crop.size == 0:
        return 0.0
    a = np.array(Image.fromarray(crop).resize((128, 128), Image.BILINEAR), dtype=np.float64)
    b = np.array(Image.fromarray(ref).resize((128, 128), Image.BILINEAR), dtype=np.float64)
    a -= a.mean(); b -= b.mean()
    den = np.sqrt((a * a).sum() * (b * b).sum())
    return float((a * b).sum() / den) if den > 0 else 0.0


def main():
    t0 = time.time()
    os.makedirs(os.path.dirname(OUT_R16), exist_ok=True)

    print("[1/2] MDT25 de 60x60 km (%d tiles)..." % (REJILLA[0] * REJILLA[1]), flush=True)
    alt = mosaico_altura()
    print("  mosaico %dx%d, %.0f..%.0f m" % (alt.shape[1], alt.shape[0], alt.min(), alt.max()), flush=True)

    corr = verificar_altura(alt)
    print("  verify MDT: corr=%.4f" % corr, flush=True)
    if corr < 0.9:
        raise SystemExit("El mosaico de alturas no casa con el WCS (corr=%.3f). Revisa la orientación." % corr)

    alt = remuestrear(alt, RES_ALTURA)
    if alt.max() > DATUM_M + 65535 * PASO_M:
        raise SystemExit("Altura %.0f m fuera del rango codificable; sube PASO_M." % alt.max())

    # Fila 0 al SUR, como el heightmap principal.
    q = np.clip(np.rint((np.flipud(alt) - DATUM_M) / PASO_M), 0, 65535).astype("<u2")
    q.tofile(OUT_R16)
    print("  -> %s (%d^2, %.1f MB)" % (OUT_R16, RES_ALTURA, os.path.getsize(OUT_R16) / 1e6), flush=True)

    meta = {
        "archivo": os.path.basename(OUT_R16),
        "resolucion": RES_ALTURA,
        "lado_m": 2 * RADIO_M,
        "metrosPorMuestra": (2 * RADIO_M) / RES_ALTURA,
        "utm": {"centro_E": CENTRO_E, "centro_N": CENTRO_N, "box": list(BOX), "epsg": 25830},
        "mundo_cm": {
            "centroX": (CENTRO_E - 566033.0) * 100.0,
            "centroY": (CENTRO_N - 4741332.0) * 100.0,
            "semilado": RADIO_M * 100.0,
        },
        "codificacion": {"datum_m": DATUM_M, "paso_m": PASO_M, "orden": "uint16 little-endian",
                         "fila0": "sur"},
        "altura_m": {"min": float(alt.min()), "max": float(alt.max())},
        "ortofoto": {"archivo": os.path.basename(OUT_ORTO), "resolucion": RES_ORTO, "flip": "sur arriba"},
        "verify_mdt_corr": corr,
    }
    with open(OUT_META, "w", encoding="utf-8") as fh:
        json.dump(meta, fh, indent=1, ensure_ascii=False)
    print("  -> %s" % OUT_META, flush=True)

    print("[2/2] PNOA de 60x60 km a %d^2..." % RES_ORTO, flush=True)
    orto = mosaico_orto().transpose(Image.FLIP_TOP_BOTTOM)   # SUR arriba
    orto.save(OUT_ORTO, optimize=True)
    print("  -> %s (%.1f MB)" % (OUT_ORTO, os.path.getsize(OUT_ORTO) / 1e6), flush=True)

    print("Listo en %.0f s." % (time.time() - t0), flush=True)


if __name__ == "__main__":
    main()
