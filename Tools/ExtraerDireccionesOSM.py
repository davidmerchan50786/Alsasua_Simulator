r"""
ExtraerDireccionesOSM.py — Saca las direcciones de OSM a Datos/direcciones_osm.json.

De dónde sale el dato
---------------------
La rama `pamplona` (el proyecto de Unity, historia aparte de main) tiene
Assets/OSMData/alsasua_edificios.json: 1078 edificios de OSM con addr:street,
addr:housenumber y building:levels. Los ids son ways de OSM, los mismos que usa
buildings_final.json, así que se unen sin ambigüedad: de los 463 con calle y 454
con portal, TODOS caen en un edificio que este proyecto construye.

Para conseguir el fichero de entrada:

    git show origin/pamplona:Assets/OSMData/alsasua_edificios.json > /tmp/osm.json
    python3 Tools/ExtraerDireccionesOSM.py /tmp/osm.json

Por qué el punto de calle se resuelve aquí y no en C++
------------------------------------------------------
Cada dirección lleva ya el punto de su calle más cercano al edificio (calle_x,
calle_z, en local relativo), buscado entre TODOS los tramos de roads_unity.json
cuyo name o name_eu casan por nombre normalizado. Así el runtime no vuelve a
parsear las 489 vías ni tiene que normalizar unicode, y la calidad del
emparejamiento se mide fuera del motor.

Sólo se emite el punto si la calle queda a menos de DISTANCIA_MAX del centro del
edificio. Más lejos el tag no sirve para orientar nada: hay direcciones cuyo
tramo más cercano con ese nombre está a 600 m, y el proyecto prefiere caer al
lado largo del edificio antes que poner la puerta mirando al otro extremo del
pueblo.

No pisa nada: sólo escribe Content/Datos/direcciones_osm.json.
"""
import collections
import json
import math
import os
import sys
import unicodedata

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATOS = os.path.join(BASE, "Content", "Datos")
SALIDA = os.path.join(DATOS, "direcciones_osm.json")


def normalizar(s):
    """Minúsculas, sin acentos y sin espacios repetidos."""
    s = unicodedata.normalize("NFKD", s or "").encode("ascii", "ignore").decode()
    return " ".join(s.lower().split())


def id_way(s):
    """'way/91927762' -> 91927762."""
    return int(str(s).split("/")[-1])


# Más allá de esto la calle tagueada no sirve para orientar la puerta (m).
DISTANCIA_MAX = 60.0


def tramos_por_nombre():
    """Nombre normalizado -> lista de polilíneas de roads_unity.json."""
    with open(os.path.join(DATOS, "roads_unity.json"), encoding="utf-8") as f:
        vias = json.load(f)

    por_nombre = collections.defaultdict(list)
    for via in vias:
        linea = [(p["x"], p["z"]) for p in (via.get("points") or [])]
        if len(linea) < 2:
            continue
        # Todos los tramos con ese nombre, no sólo el más largo: OSM parte las
        # calles en varios ways y el más largo puede quedar a 200 m del portal
        # (mediana 38,8 m con un tramo, 16,7 m con todos).
        for clave in ("name", "name_eu"):
            nombre = normalizar(via.get(clave))
            if nombre:
                por_nombre[nombre].append(linea)
    return por_nombre


def punto_mas_cercano(lineas, q):
    """Punto más cercano a q proyectando sobre los segmentos. -> (punto, dist)."""
    mejor = None
    mejor_d2 = float("inf")
    for linea in lineas:
        for i in range(len(linea) - 1):
            (ax, az), (bx, bz) = linea[i], linea[i + 1]
            abx, abz = bx - ax, bz - az
            l2 = abx * abx + abz * abz
            if l2 > 1e-8:
                t = max(0.0, min(1.0, ((q[0] - ax) * abx + (q[1] - az) * abz) / l2))
                p = (ax + abx * t, az + abz * t)
            else:
                p = (ax, az)
            d2 = (p[0] - q[0]) ** 2 + (p[1] - q[1]) ** 2
            if d2 < mejor_d2:
                mejor_d2, mejor = d2, p
    return mejor, math.sqrt(mejor_d2) if mejor else None


def main(ruta_osm):
    if not os.path.isfile(ruta_osm):
        print(f"[Direcciones] no existe {ruta_osm}")
        print("[Direcciones] saca el fichero con:")
        print("    git show origin/pamplona:Assets/OSMData/alsasua_edificios.json > /tmp/osm.json")
        return 1

    with open(ruta_osm, encoding="utf-8") as f:
        osm = json.load(f)["edificios"]

    with open(os.path.join(DATOS, "buildings_final.json"), encoding="utf-8") as f:
        edificios = {e["id"]: e for e in json.load(f)}

    tramos = tramos_por_nombre()

    salida = []
    sin_via = collections.Counter()
    lejos = 0
    distancias = []
    fuera = 0
    for edificio in osm:
        tags = edificio.get("tags") or {}
        calle = tags.get("addr:street")
        portal = tags.get("addr:housenumber")
        if not calle and not portal:
            continue

        bid = id_way(edificio["id"])
        if bid not in edificios:
            fuera += 1          # está en OSM pero este proyecto no lo construye
            continue

        entrada = {"id": bid}
        if calle:
            entrada["calle"] = calle
            lineas = tramos.get(normalizar(calle))
            if not lineas:
                sin_via[calle] += 1
            else:
                # Centro del footprint en local relativo, igual que lo calcula
                # el runtime a partir de buildings_final.json.
                vs = edificios[bid]["vertices"]
                centro = (sum(v["x"] for v in vs) / len(vs), sum(v["z"] for v in vs) / len(vs))
                punto, dist = punto_mas_cercano(lineas, centro)
                if dist is not None and dist <= DISTANCIA_MAX:
                    entrada["calle_x"] = round(punto[0], 2)
                    entrada["calle_z"] = round(punto[1], 2)
                    distancias.append(dist)
                else:
                    lejos += 1
        if portal:
            entrada["portal"] = str(portal)
        salida.append(entrada)

    salida.sort(key=lambda e: e["id"])
    with open(SALIDA, "w", encoding="utf-8") as f:
        json.dump(salida, f, ensure_ascii=False, indent=1)

    con_calle = sum(1 for e in salida if "calle" in e)
    con_punto = sum(1 for e in salida if "calle_x" in e)
    con_portal = sum(1 for e in salida if "portal" in e)
    print(f"[Direcciones] {len(salida)} edificios -> {os.path.relpath(SALIDA, BASE)}")
    print(f"[Direcciones] calle {con_calle} | punto de calle {con_punto} | portal {con_portal}")
    if distancias:
        distancias.sort()
        print("[Direcciones] distancia al eje de la calle: mediana %.1f m, máxima %.1f m"
              % (distancias[len(distancias) // 2], distancias[-1]))
    if lejos:
        print(f"[Direcciones] {lejos} sin punto: su calle queda a más de {DISTANCIA_MAX:.0f} m")
    if fuera:
        print(f"[Direcciones] {fuera} descartados: en OSM pero no en buildings_final.json")
    for calle, n in sin_via.most_common():
        print(f"[Direcciones] sin vía en roads_unity.json: {calle} ({n} edificios)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1] if len(sys.argv) > 1 else "/tmp/osm.json"))
