"""
DescargarOrtofotoPNOA.py — Ortos reales de Alsasua desde el WMS PNOA del IGN.

Genera dos texturas georreferenciadas al mismo grid UTM30N del mundo Unreal
(UE5 X=east, Y=north; world cm = (UTM_m - 566033, UTM_m - 4741332) * 100):

  1. Cobertura total del terreno (7200x7200 m)  -> Content/Terreno/alsasua_satelite_pnoa_8192.png
  2. Detalle urbano plaza (2750x2750 m)          -> Content/Textures/ortofoto_pnoa_plaza_8192.png

Convención de flip: el material del terreno mapea v=0 -> world Y=Ymin (sur) y en
UE v=0 es la fila superior, luego la textura lleva el SUR arriba. El WMS PNOA
devuelve norte-arriba, así que se hace flip TOP_BOTTOM.

Uso:  python3 Tools/DescargarOrtofotoPNOA.py
Requiere: pip install pillow numpy
"""
import io
import json
import os
import sys
import time
import urllib.request

from PIL import Image
import numpy as np

WMS = ("https://www.ign.es/wms-inspire/pnoa-ma"
       "?SERVICE=WMS&VERSION=1.1.1&REQUEST=GetMap"
       "&LAYERS=OI.OrthoimageCoverage&STYLES=OI.OrthoimageCoverage.Default"
       "&SRS=EPSG:25830&FORMAT=image/jpeg")
UA = {"User-Agent": "alsasua-sim/1.0 (proyecto personal)"}

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))  # UnrealProject/

# Box completo del terreno: centro = Herriko Plaza = UTM (567951, 4749902).
# MitadMundo = (4033-1)*178.5714*0.5 = 3600 m.
BOX_FULL = (564351.0, 4746302.0, 571551.0, 4753502.0)   # 7200x7200 m
BOX_TOWN = (566576.0, 4748527.0, 569326.0, 4751277.0)   # 2750x2750 m, centrado plaza

OUT_FULL = os.path.join(ROOT, "Content", "Terreno", "alsasua_satelite_pnoa_8192.png")
OUT_TOWN = os.path.join(ROOT, "Content", "Textures", "ortofoto_pnoa_plaza_8192.png")

GRID = (4, 4)
TILE = 2048


def fetch_tile(bbox, tries=6):
    url = "%s&BBOX=%f,%f,%f,%f&WIDTH=%d&HEIGHT=%d" % (WMS, bbox[0], bbox[1], bbox[2], bbox[3], TILE, TILE)
    for i in range(tries):
        try:
            req = urllib.request.Request(url, headers=UA)
            with urllib.request.urlopen(req, timeout=120) as r:
                img = Image.open(io.BytesIO(r.read())).convert("RGB")
            if img.size != (TILE, TILE):
                raise ValueError("tamano %s != %d" % (img.size, TILE))
            return img
        except Exception as e:
            print("  tile retry %d/%d: %s" % (i + 1, tries, e), flush=True)
            time.sleep(2 + 3 * i)
    raise RuntimeError("tile fallo despues de %d intentos" % tries)


def mosaic(box, grid):
    e0, n0, e1, n1 = box
    de = (e1 - e0) / grid[0]
    dn = (n1 - n0) / grid[1]
    out = Image.new("RGB", (grid[0] * TILE, grid[1] * TILE))
    total = grid[0] * grid[1]
    for j in range(grid[1]):
        for i in range(grid[0]):
            bbox = (e0 + i * de, n0 + j * dn, e0 + (i + 1) * de, n0 + (j + 1) * dn)
            # Los tiles WMS vienen norte-arriba: banda j=0 (sur) al fondo,
            # banda j=grid-1 (norte) arriba -> mosaico norte-arriba.
            out.paste(fetch_tile(bbox), (i * TILE, (grid[1] - 1 - j) * TILE))
            print("  tile %d/%d ok" % (j * grid[0] + i + 1, total), flush=True)
    return out


def verify(out, box, sub, label):
    """Comprueba que el mosaico (flip SUR arriba) coincide con un fetch PNOA fresco.

    Ambos se voltean igual (SUR arriba) y el crop usa la misma convención que el
    material del terreno: fila = (N1 - N)/dn. corr ~ +1 si el mosaico es correcto.
    """
    e0, n0, e1, n1 = box
    de = (box[2] - box[0]) / out.width
    dn = (box[3] - box[1]) / out.height
    x0, y0, x1, y1 = sub
    crop = out.crop((int((x0 - e0) / de), int((n1 - y1) / dn),
                     int((x1 - e0) / de), int((n1 - y0) / dn))).resize((256, 256), Image.BILINEAR)
    ref = fetch_tile(sub, tries=3).transpose(Image.FLIP_TOP_BOTTOM)
    ref = ref.resize((256, 256), Image.BILINEAR)
    a = np.asarray(crop).astype(np.float32)
    b = np.asarray(ref).astype(np.float32)
    a = a - a.mean(); b = b - b.mean()
    denom = np.sqrt((a * a).sum() * (b * b).sum())
    corr = float((a * b).sum() / denom) if denom > 0 else 0.0
    print("  verify %s: corr=%.3f" % (label, corr), flush=True)
    return corr


def main():
    t0 = time.time()
    report = {"box_full": BOX_FULL, "box_town": BOX_TOWN, "tiles": {}}

    print("[1/2] Cobertura total 7200m (16 tiles de 2048^2)...", flush=True)
    full = mosaic(BOX_FULL, GRID)
    full = full.transpose(Image.FLIP_TOP_BOTTOM)          # SUR arriba
    full.save(OUT_FULL)
    print("  -> %s (%dx%d)" % (OUT_FULL, full.width, full.height), flush=True)
    report["full"] = {"file": OUT_FULL, "size": [full.width, full.height]}

    print("[2/2] Detalle urbano 2750m...", flush=True)
    town = mosaic(BOX_TOWN, GRID)
    town = town.transpose(Image.FLIP_TOP_BOTTOM)
    town.save(OUT_TOWN)
    print("  -> %s (%dx%d)" % (OUT_TOWN, town.width, town.height), flush=True)
    report["town"] = {"file": OUT_TOWN, "size": [town.width, town.height]}

    print("Verificacion (round-trip contra WMS):", flush=True)
    sub = (566951.0, 4748902.0, 568951.0, 4750902.0)  # 2x2 km centrado en la plaza
    report["verify_full"] = verify(full, BOX_FULL, sub, "full/plaza")
    report["verify_town"] = verify(town, BOX_TOWN, sub, "town/plaza")
    report["seconds"] = round(time.time() - t0, 1)

    with open(os.path.join(ROOT, "Content", "Terreno", "ortofoto_pnoa_report.json"), "w") as f:
        json.dump(report, f, indent=1)
    print("Listo en %.1fs. Reporte: Content/Terreno/ortofoto_pnoa_report.json" % report["seconds"])
    return 0


if __name__ == "__main__":
    sys.exit(main())
