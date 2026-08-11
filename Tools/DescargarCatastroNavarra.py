"""
DescargarCatastroNavarra.py — Huellas catastrales reales de Alsasua desde IDENA.

Por qué IDENA y no el Catastro nacional: Navarra tiene catastro foral propio y
NO está en ovc.catastro.meh.es. Ese servicio responde, pero para Alsasua no
devuelve nada — es un callejón sin salida fácil de recorrer sin darse cuenta.
La fuente buena es IDENA (Infraestructura de Datos Espaciales de Navarra),
datos abiertos del Gobierno de Navarra.

Sirve para contrastar Content/Datos/buildings_final.json contra la geometría
oficial: comprobar que cada edificio del mundo está donde el catastro dice, con
la planta que dice, y encontrar los que sobran o faltan.

Tres cosas que costaron encontrar y que conviene no volver a averiguar:
  - Sin User-Agent el servidor cierra la conexión sin responder (curl da
    "Empty reply from server"). Con uno cualquiera, responde.
  - El orden de ejes del bbox es E,N (xmin,ymin,xmax,ymax,EPSG:25830).
    Con N,E devuelve numberMatched="0" sin error, que despista mucho.
  - WFS 2.0 usa `typenames` y `count`; el 1.1.0 usa `typename` y `maxFeatures`.

Uso:  python3 Tools/DescargarCatastroNavarra.py
Salida: Content/Datos/catastro_navarra_edificios.json
"""
import json
import math
import os
import re
import time
import urllib.request

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SALIDA = os.path.join(RAIZ, "Content", "Datos", "catastro_navarra_edificios.json")

WFS = "https://idena.navarra.es/ogc/wfs"
CAPA = "IDENA:CATAST_Pol_Edificacion"
UA = {"User-Agent": "alsasua-sim/1.0 (proyecto personal)"}

# Mismo marco que el resto del mundo: centro en Herriko Plaza, y el terreno
# jugable son 7200x7200 m (semilado 3600).
CENTRO_E, CENTRO_N = 567951.0, 4749902.0
SEMILADO_M = 3600.0

# world_cm = (UTM_m - ORIGEN) * 100, con X=este e Y=norte. Es el mismo origen
# LIDAR que usan GeoDataAlsasua y las ortofotos; no lo cambies por tu cuenta.
ORIGEN_E, ORIGEN_N = 566033.0, 4741332.0

# El servicio pagina; se baja por teselas para no pedir un GML gigante.
TESELAS = 6


def _get(url, tries=5, timeout=180):
    for i in range(tries):
        try:
            req = urllib.request.Request(url, headers=UA)
            with urllib.request.urlopen(req, timeout=timeout) as r:
                return r.read().decode("utf-8", "ignore")
        except Exception as e:
            print("  reintento %d/%d: %s" % (i + 1, tries, e), flush=True)
            time.sleep(2 + 3 * i)
    raise RuntimeError("IDENA no responde: %s" % url)


def pedir_tesela(e0, n0, e1, n1):
    url = ("%s?service=WFS&version=2.0.0&request=GetFeature&typenames=%s"
           "&bbox=%f,%f,%f,%f,EPSG:25830" % (WFS, CAPA, e0, n0, e1, n1))
    return _get(url)


def parsear(gml):
    """Saca (anillo_exterior, area, perimetro) de cada edificio del GML.

    Se hace con regex y no con un parser XML completo a propósito: el GML de
    IDENA es plano y regular, y así no se añade dependencia de lxml/gdal a un
    repo que hoy sólo necesita numpy y pillow.
    """
    out = []
    for feat in re.findall(r"<IDENA:CATAST_Pol_Edificacion[ >].*?</IDENA:CATAST_Pol_Edificacion>",
                           gml, re.S):
        area = re.search(r"<IDENA:GEOM_AREA>([\d.]+)", feat)
        peri = re.search(r"<IDENA:GEOM_PERI>([\d.]+)", feat)
        # posList del anillo exterior; si hay huecos, el primero es el contorno.
        pos = re.search(r"<gml:exterior>.*?<gml:posList[^>]*>([^<]+)</gml:posList>", feat, re.S)
        if not pos:
            pos = re.search(r"<gml:posList[^>]*>([^<]+)</gml:posList>", feat, re.S)
        if not pos:
            continue
        nums = [float(x) for x in pos.group(1).split()]
        pares = [(nums[i], nums[i + 1]) for i in range(0, len(nums) - 1, 2)]
        if len(pares) < 3:
            continue
        out.append({
            "anillo": pares,
            "area_m2": float(area.group(1)) if area else 0.0,
            "perimetro_m": float(peri.group(1)) if peri else 0.0,
        })
    return out


def a_mundo_cm(e, n):
    return ((e - ORIGEN_E) * 100.0, (n - ORIGEN_N) * 100.0)


def main():
    t0 = time.time()
    e0, n0 = CENTRO_E - SEMILADO_M, CENTRO_N - SEMILADO_M
    paso = (2 * SEMILADO_M) / TESELAS

    vistos = set()
    edificios = []
    for j in range(TESELAS):
        for i in range(TESELAS):
            bb = (e0 + i * paso, n0 + j * paso, e0 + (i + 1) * paso, n0 + (j + 1) * paso)
            gml = pedir_tesela(*bb)
            nuevos = 0
            for ed in parsear(gml):
                # Las teselas comparten borde y un edificio puede salir dos
                # veces; la clave es su primer vértice redondeado al cm.
                clave = (round(ed["anillo"][0][0], 2), round(ed["anillo"][0][1], 2))
                if clave in vistos:
                    continue
                vistos.add(clave)
                cx = sum(p[0] for p in ed["anillo"]) / len(ed["anillo"])
                cy = sum(p[1] for p in ed["anillo"]) / len(ed["anillo"])
                wx, wy = a_mundo_cm(cx, cy)
                edificios.append({
                    "centro_utm": [round(cx, 2), round(cy, 2)],
                    "centro_mundo_cm": [round(wx, 1), round(wy, 1)],
                    "area_m2": ed["area_m2"],
                    "perimetro_m": ed["perimetro_m"],
                    "anillo_mundo_cm": [[round(v, 1) for v in a_mundo_cm(x, y)]
                                        for x, y in ed["anillo"]],
                })
                nuevos += 1
            print("  tesela %2d/%d: %d edificios nuevos (total %d)"
                  % (j * TESELAS + i + 1, TESELAS * TESELAS, nuevos, len(edificios)), flush=True)

    doc = {
        "_fuente": "IDENA - Gobierno de Navarra, capa %s (datos abiertos)" % CAPA,
        "_nota": ("Navarra tiene catastro foral: NO está en el Catastro nacional. "
                  "Coordenadas en UTM30N/EPSG:25830 y en cm de mundo "
                  "((UTM - (%.0f, %.0f)) * 100)." % (ORIGEN_E, ORIGEN_N)),
        "_bbox_utm": [e0, n0, e0 + 2 * SEMILADO_M, n0 + 2 * SEMILADO_M],
        "edificios": edificios,
    }
    with open(SALIDA, "w", encoding="utf-8") as fh:
        json.dump(doc, fh, ensure_ascii=False)

    area = sum(e["area_m2"] for e in edificios)
    print("\n%d edificios -> %s" % (len(edificios), SALIDA))
    print("superficie construida total: %.0f m2 (media %.0f m2)"
          % (area, area / max(1, len(edificios))))
    print("hecho en %.0f s" % (time.time() - t0))


CAPA_ALTURAS = "IDENA:CARTO1_Txt_20AlturaEd"
SALIDA_ALTURAS = os.path.join(RAIZ, "Content", "Datos", "catastro_navarra_alturas.json")

# Notación cartográfica de alturas de IDENA (campo CADTEXT). Los romanos son
# plantas sobre rasante, que es lo que se ve en fachada y lo que manda en cuántas
# filas de ventanas hay. "S+"/"SS+" son sótanos: no suman altura visible. "Por" y
# "Pr" son porches, sin plantas. "+T" es remate de torreón sobre la última.
ROMANOS = {"I": 1, "II": 2, "III": 3, "IV": 4, "V": 5, "VI": 6, "VII": 7, "VIII": 8}


def plantas_de_cadtext(txt):
    """Plantas sobre rasante que declara un CADTEXT, o None si no las declara."""
    t = (txt or "").strip().upper()
    if not t or t.startswith(("POR", "PR")):
        return None
    t = re.sub(r"^S{1,2}\s*\+\s*", "", t)      # quita sótanos
    t = re.sub(r"^P\s*\+\s*", "", t)           # porche + plantas
    t = re.sub(r"\s*\+\s*T$", "", t)           # remate de torreón
    t = t.strip()
    if t in ROMANOS:
        return ROMANOS[t]
    m = re.match(r"^(\d+)$", t)
    return int(m.group(1)) if m else None


def descargar_alturas():
    """Baja los puntos de altura de edificio y los deja con sus plantas ya leídas.

    Va por teselas como la descarga de edificios, y no de una sola petición: el
    servidor corta en 1000 features y devuelve el primer bloque en su orden de
    almacenamiento, que resultó ser otra hoja (Olaza) en vez del casco de
    Alsasua. Se detecta enseguida si se miran los rangos de coordenadas, y no se
    detecta en absoluto si uno se fía del número de features devueltas.
    """
    e0, n0 = CENTRO_E - SEMILADO_M, CENTRO_N - SEMILADO_M
    paso = (2 * SEMILADO_M) / TESELAS

    puntos, vistos = [], set()
    for j in range(TESELAS):
        for i in range(TESELAS):
            bb = (e0 + i * paso, n0 + j * paso, e0 + (i + 1) * paso, n0 + (j + 1) * paso)
            url = ("%s?service=WFS&version=2.0.0&request=GetFeature&typenames=%s"
                   "&bbox=%f,%f,%f,%f,EPSG:25830&count=1000"
                   % (WFS, CAPA_ALTURAS, bb[0], bb[1], bb[2], bb[3]))
            gml = _get(url)
            for feat in re.findall(
                    r"<IDENA:CARTO1_Txt_20AlturaEd[ >].*?</IDENA:CARTO1_Txt_20AlturaEd>",
                    gml, re.S):
                pos = re.search(r"<gml:pos[^>]*>([^<]+)</gml:pos>", feat)
                cad = re.search(r"<IDENA:CADTEXT>([^<]*)</IDENA:CADTEXT>", feat)
                alt = re.search(r"<IDENA:ALTITUD>([\d.]+)", feat)
                if not pos:
                    continue
                xs = pos.group(1).split()
                e, n = float(xs[0]), float(xs[1])
                clave = (round(e, 2), round(n, 2))
                if clave in vistos:
                    continue
                vistos.add(clave)
                wx, wy = a_mundo_cm(e, n)
                puntos.append({
                    "cadtext": (cad.group(1).strip() if cad else ""),
                    "plantas": plantas_de_cadtext(cad.group(1) if cad else ""),
                    "altitud_m": float(alt.group(1)) if alt else None,
                    "mundo_cm": [round(wx, 1), round(wy, 1)],
                })
            print("  tesela %2d/%d: %d puntos acumulados"
                  % (j * TESELAS + i + 1, TESELAS * TESELAS, len(puntos)), flush=True)

    doc = {
        "_fuente": "IDENA - Gobierno de Navarra, capa %s (datos abiertos)" % CAPA_ALTURAS,
        "_nota": ("CADTEXT en notación cartográfica: romanos = plantas sobre rasante, "
                  "S+/SS+ = sótanos (no suman altura visible), Por/Pr = porche, "
                  "+T = remate. 'plantas' ya viene interpretado."),
        "puntos": puntos,
    }
    with open(SALIDA_ALTURAS, "w", encoding="utf-8") as fh:
        json.dump(doc, fh, ensure_ascii=False)

    con = sum(1 for p in puntos if p["plantas"])
    print("%d puntos de altura (%d con plantas legibles) -> %s"
          % (len(puntos), con, SALIDA_ALTURAS))
    return puntos


def verificar_alturas():
    """Contrasta las plantas reales contra el campo levels de buildings_final."""
    import statistics
    from collections import Counter, defaultdict

    if not os.path.exists(SALIDA_ALTURAS):
        raise SystemExit("Falta %s: ejecuta antes --alturas." % SALIDA_ALTURAS)
    puntos = [p for p in json.load(open(SALIDA_ALTURAS, encoding="utf-8"))["puntos"]
              if p["plantas"]]
    bf = json.load(open(os.path.join(RAIZ, "Content", "Datos", "buildings_final.json"),
                        encoding="utf-8"))
    OX, OZ = 1918.0, 8570.0

    def centro(b):
        v = b["vertices"]
        pts = ([(p["x"], p.get("z", p.get("y"))) for p in v] if isinstance(v[0], dict)
               else [(v[i], v[i + 1]) for i in range(0, len(v) - 1, 2)])
        return ((sum(p[0] for p in pts) / len(pts) + OX) * 100.0,
                (sum(p[1] for p in pts) / len(pts) + OZ) * 100.0)

    CEL = 3000.0
    rej = defaultdict(list)
    for i, b in enumerate(bf):
        x, y = centro(b)
        rej[(int(x // CEL), int(y // CEL))].append((i, x, y))

    difs, pares = [], 0
    reales, nuestras = Counter(), Counter()
    for p in puntos:
        px, py = p["mundo_cm"]
        mejor, md = None, float("inf")
        cx, cy = int(px // CEL), int(py // CEL)
        for ax in (-1, 0, 1):
            for ay in (-1, 0, 1):
                for i, bx, by in rej.get((cx + ax, cy + ay), ()):
                    d = math.hypot(bx - px, by - py)
                    if d < md:
                        md, mejor = d, i
        if mejor is None or md > 2000.0:      # más de 20 m: no es su edificio
            continue
        lv = bf[mejor].get("levels")
        if not lv:
            continue
        pares += 1
        difs.append(lv - p["plantas"])
        reales[p["plantas"]] += 1
        nuestras[lv] += 1

    if not pares:
        # No es el marco de coordenadas: está comprobado contra la capa catastral,
        # que empareja con mediana 3,6 m. Es cobertura. Se dice con los números
        # delante para que nadie vuelva a perseguir un fallo que no existe.
        ys = [p["mundo_cm"][1] for p in puntos]
        print("Ninguna pareja. NO es el marco de coordenadas:")
        print("  puntos de altura   Y %.0f..%.0f cm" % (min(ys), max(ys)))
        print("  Herriko Plaza está en Y 857000 cm")
        print("  La serie CARTO1 (cartografía 1:1000) no cubre el casco de Alsasua:")
        print("  sobre la plaza +-1 km devuelve 0 features, mientras que")
        print("  CATAST_Pol_Edificacion devuelve 3796. Para plantas reales sobre el")
        print("  pueblo hay que ir por LiDAR (DSM - DTM sobre las huellas), no por aquí.")
        return
    exactos = sum(1 for d in difs if d == 0)
    cerca = sum(1 for d in difs if abs(d) <= 1)
    print("Puntos de altura emparejados con edificio: %d" % pares)
    print("  plantas exactas:        %4d  (%.1f%%)" % (exactos, 100.0 * exactos / pares))
    print("  con +-1 planta:         %4d  (%.1f%%)" % (cerca, 100.0 * cerca / pares))
    print("  sesgo medio (nuestro - real): %+.2f plantas   mediana %+.1f"
          % (statistics.mean(difs), statistics.median(difs)))
    print("  reparto real   :", dict(sorted(reales.items())))
    print("  reparto nuestro:", dict(sorted(nuestras.items())))


def verificar():
    """Contrasta buildings_final.json contra el catastro ya descargado.

    Resultado de la primera pasada (4627 edificios de IDENA contra los 1030 del
    mundo): mediana 3,6 m, 86,5% a menos de 10 m, 93,9% a menos de 20 m.

    Y un resultado negativo que conviene no perder: el desplazamiento medio es
    de 0,96 m frente a una desviación típica de 2,8-3,0 m, o sea que NO hay
    sesgo sistemático. Esos 3,6 m son la diferencia inherente entre huellas
    derivadas de OSM y geometría catastral, no un offset. Aplicar una corrección
    global al mundo no mejoraría la fidelidad; lo que sí la mejoraría es
    sustituir huella por huella donde haya pareja fiable.
    """
    import statistics
    from collections import defaultdict

    if not os.path.exists(SALIDA):
        raise SystemExit("Falta %s: ejecuta antes la descarga." % SALIDA)
    cat = json.load(open(SALIDA, encoding="utf-8"))["edificios"]
    bf = json.load(open(os.path.join(RAIZ, "Content", "Datos", "buildings_final.json"),
                        encoding="utf-8"))

    # buildings_final.json viene en marco RELATIVO: hay que sumar OX/OZ.
    OX, OZ = 1918.0, 8570.0

    def centro(b):
        v = b["vertices"]
        pts = ([(p["x"], p.get("z", p.get("y"))) for p in v] if isinstance(v[0], dict)
               else [(v[i], v[i + 1]) for i in range(0, len(v) - 1, 2)])
        return ((sum(p[0] for p in pts) / len(pts) + OX) * 100.0,
                (sum(p[1] for p in pts) / len(pts) + OZ) * 100.0)

    C = [tuple(e["centro_mundo_cm"]) for e in cat]
    CEL = 5000.0
    rej = defaultdict(list)
    for i, (x, y) in enumerate(C):
        rej[(int(x // CEL), int(y // CEL))].append(i)

    dists, dxs, dys, huerfanos = [], [], [], 0
    for b in bf:
        x, y = centro(b)
        mejor, md = None, float("inf")
        cx, cy = int(x // CEL), int(y // CEL)
        for ax in (-1, 0, 1):
            for ay in (-1, 0, 1):
                for i in rej.get((cx + ax, cy + ay), ()):
                    d = math.hypot(C[i][0] - x, C[i][1] - y)
                    if d < md:
                        md, mejor = d, i
        if mejor is None:
            huerfanos += 1
            continue
        dists.append(md / 100.0)
        if md < 1000.0:                      # pareja fiable: menos de 10 m
            dxs.append((C[mejor][0] - x) / 100.0)
            dys.append((C[mejor][1] - y) / 100.0)

    dists.sort()
    n = len(dists)
    print("Edificios del mundo: %d   catastro IDENA: %d" % (len(bf), len(cat)))
    for u in (5, 10, 20, 50):
        c = sum(1 for d in dists if d < u)
        print("  a menos de %2d m: %4d  (%.1f%%)" % (u, c, 100.0 * c / max(1, n)))
    if n:
        print("  mediana %.1f m   p90 %.1f m" % (statistics.median(dists), dists[int(0.9 * n)]))
    if huerfanos:
        print("  sin ninguna pareja cerca: %d" % huerfanos)
    if dxs:
        mx, my = statistics.mean(dxs), statistics.mean(dys)
        sx, sy = statistics.pstdev(dxs), statistics.pstdev(dys)
        print("  desplazamiento medio dE %+.2f m dN %+.2f m  (sigma %.2f / %.2f)"
              % (mx, my, sx, sy))
        print("  sesgo sistematico %.2f m -> %s"
              % (math.hypot(mx, my),
                 "corregible" if math.hypot(mx, my) > max(sx, sy) else
                 "no: es dispersion, una correccion global no ayuda"))


if __name__ == "__main__":
    import sys
    if "--alturas" in sys.argv:
        descargar_alturas()
        verificar_alturas()
    elif "--verificar" in sys.argv:
        verificar()
    else:
        main()
