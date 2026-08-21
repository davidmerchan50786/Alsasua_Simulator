"""
VerificarCallesNavarra.py — Contrasta el trazado de calles contra el eje oficial.

Tercer contraste de la serie, después de los edificios (posición, mediana 3,6 m)
y sus alturas (~3 m de defecto, corregido): la red viaria.

roads_unity.json viene de OSM. IDENA publica CATAST_Lin_CalleEje, el eje de calle
del catastro foral de Navarra, que es el trazado oficial. Comparando punto a eje
se ve si las calles del mundo van por donde van de verdad.

Qué NO mide: el ancho. roads_unity.json trae "width" y el eje es una línea sin
anchura, así que eso no se puede contrastar por aquí.

Ojo con el alcance: el eje catastral cubre suelo urbano. Las autovías, pistas y
caminos de monte de OSM caen fuera y no tienen contra qué compararse — por eso el
informe va por tipo de vía y no da un único número global, que mezclaría churras
con merinas.

Uso:  python3 Tools/VerificarCallesNavarra.py
"""
import json
import math
import os
import re
import time
import urllib.request
from collections import defaultdict

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATOS = os.path.join(RAIZ, "Content", "Datos")
CACHE = os.path.join(DATOS, "calles_navarra_ejes.json")

WFS = "https://idena.navarra.es/ogc/wfs"
CAPA = "IDENA:CATAST_Lin_CalleEje"
UA = {"User-Agent": "alsasua-sim/1.0 (proyecto personal)"}

CENTRO_E, CENTRO_N = 567951.0, 4749902.0
SEMILADO_M = 3600.0
TESELAS = 6

# Mismo frame que el resto del mundo.
ORIGEN_E, ORIGEN_N = 566033.0, 4741332.0
OX, OZ = 1918.0, 8570.0          # roads_unity.json es RELATIVO


def _get(url, tries=4, timeout=180):
    for i in range(tries):
        try:
            req = urllib.request.Request(url, headers=UA)
            with urllib.request.urlopen(req, timeout=timeout) as r:
                return r.read().decode("utf-8", "ignore")
        except Exception as e:
            print("  reintento %d/%d: %s" % (i + 1, tries, e), flush=True)
            time.sleep(2 + 3 * i)
    raise RuntimeError("IDENA no responde")


def descargar_ejes():
    """Segmentos del eje de calle, en cm de mundo. Va por teselas: el servidor
    corta en 1000 features y devolvería sólo el primer bloque."""
    if os.path.exists(CACHE):
        return json.load(open(CACHE, encoding="utf-8"))["segmentos"]

    e0, n0 = CENTRO_E - SEMILADO_M, CENTRO_N - SEMILADO_M
    paso = (2 * SEMILADO_M) / TESELAS
    segs, vistos = [], set()

    for j in range(TESELAS):
        for i in range(TESELAS):
            bb = (e0 + i * paso, n0 + j * paso, e0 + (i + 1) * paso, n0 + (j + 1) * paso)
            url = ("%s?service=WFS&version=2.0.0&request=GetFeature&typenames=%s"
                   "&bbox=%f,%f,%f,%f,EPSG:25830&count=1000"
                   % (WFS, CAPA, bb[0], bb[1], bb[2], bb[3]))
            gml = _get(url)
            for pl in re.findall(r"<gml:posList[^>]*>([^<]+)</gml:posList>", gml):
                nums = [float(x) for x in pl.split()]
                pts = [(nums[k], nums[k + 1]) for k in range(0, len(nums) - 1, 2)]
                if len(pts) < 2:
                    continue
                clave = (round(pts[0][0], 2), round(pts[0][1], 2), len(pts))
                if clave in vistos:
                    continue
                vistos.add(clave)
                for a, b in zip(pts, pts[1:]):
                    segs.append([[(a[0] - ORIGEN_E) * 100.0, (a[1] - ORIGEN_N) * 100.0],
                                 [(b[0] - ORIGEN_E) * 100.0, (b[1] - ORIGEN_N) * 100.0]])
            print("  tesela %2d/%d: %d segmentos" % (j * TESELAS + i + 1, TESELAS * TESELAS,
                                                     len(segs)), flush=True)

    json.dump({"_fuente": "IDENA %s" % CAPA, "segmentos": segs},
              open(CACHE, "w", encoding="utf-8"))
    print("  -> %s (%d segmentos)" % (CACHE, len(segs)))
    return segs


def dist_punto_segmento(px, py, ax, ay, bx, by):
    vx, vy = bx - ax, by - ay
    L2 = vx * vx + vy * vy
    if L2 <= 1e-9:
        return math.hypot(px - ax, py - ay)
    t = max(0.0, min(1.0, ((px - ax) * vx + (py - ay) * vy) / L2))
    return math.hypot(px - (ax + t * vx), py - (ay + t * vy))


def main():
    print("Ejes de calle de IDENA...", flush=True)
    segs = descargar_ejes()
    if not segs:
        raise SystemExit("Sin segmentos; revisa la conexión.")

    # Rejilla de segmentos por celda para no comparar todo contra todo.
    CEL = 5000.0
    rej = defaultdict(list)
    for s in segs:
        (ax, ay), (bx, by) = s
        for cx in range(int(min(ax, bx) // CEL), int(max(ax, bx) // CEL) + 1):
            for cy in range(int(min(ay, by) // CEL), int(max(ay, by) // CEL) + 1):
                rej[(cx, cy)].append(s)

    vias = json.load(open(os.path.join(DATOS, "roads_unity.json"), encoding="utf-8"))
    if isinstance(vias, dict):
        vias = vias.get("roads", [])

    portipo = defaultdict(list)
    for v in vias:
        tipo = v.get("type", "?")
        for p in v.get("points", []):
            wx = (p["x"] + OX) * 100.0
            wy = (p["z"] + OZ) * 100.0
            cx, cy = int(wx // CEL), int(wy // CEL)
            mejor = float("inf")
            for dx in (-1, 0, 1):
                for dy in (-1, 0, 1):
                    for (ax, ay), (bx, by) in rej.get((cx + dx, cy + dy), ()):
                        d = dist_punto_segmento(wx, wy, ax, ay, bx, by)
                        if d < mejor:
                            mejor = d
            if mejor < float("inf"):
                portipo[tipo].append(mejor / 100.0)

    print("\nDistancia de cada punto de vía al eje de calle oficial más cercano.")
    print("Sólo cuentan los puntos con algún eje en la celda vecina: el eje")
    print("catastral es urbano, y autovías, pistas y caminos caen fuera.\n")
    print("  %-16s %7s %9s %9s" % ("tipo de vía", "puntos", "mediana", "p90"))
    for tipo, ds in sorted(portipo.items(), key=lambda k: -len(k[1])):
        ds.sort()
        print("  %-16s %7d %8.1f m %8.1f m"
              % (tipo, len(ds), ds[len(ds) // 2], ds[int(0.9 * len(ds))]))

    # Sin línea de total a propósito. Promediar calle urbana con autovía da un
    # número que no significa nada: las de tipo motorway no tienen eje catastral
    # contra el que medirse y su "distancia" es a la calle urbana que pillen al
    # lado. Lo que hay que leer son las filas de arriba, no un agregado.
    print("\nSe leen por tipo. Las residenciales y terciarias son las que tienen")
    print("eje catastral de verdad enfrente; motorway y path caen fuera de esa")
    print("capa y sus cifras son ruido, no error de trazado.")


if __name__ == "__main__":
    main()
