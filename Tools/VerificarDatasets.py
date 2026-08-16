"""
VerificarDatasets.py — Contrasta lo que el C++ espera de cada JSON contra el JSON.

Los 30 ficheros de Content/Datos/ no comparten forma. Unos son un array en la
raíz y otros envuelven el array en un objeto, y los nombres de campo no siguen
ningún criterio: "rails", "pois", "barrios", "edificios", "segmentos". Eso sería
llevadero si equivocarse doliera, pero en UE no duele: deserializar a TArray
contra una raíz de objeto —o a FJsonObject contra una raíz de array— devuelve
false y ya está. Ni excepción, ni aviso, ni nada en el log. El sistema hace
`return` y el arranque continúa tan tranquilo.

Así se perdieron, cada uno por su lado y durante quién sabe cuánto, la vía férrea
entera, las superficies de calle, los coches aparcados, las señales de tráfico y
las calles y los ríos del minimapa. Ninguno dejó rastro.

Qué hace esto: por cada .cpp saca los datasets que menciona y los nombres de
campo que pide por TryGet*/Get*Field, y avisa de los que no existen en ninguno de
esos datasets, ni en la raíz ni dentro de sus elementos. Un nombre que no está en
el dato es un `return` silencioso esperando.

Qué NO hace: no sabe qué campo va con qué fichero cuando un .cpp lee varios, ni
entiende nombres construidos en variables. Es un detector de bultos, no un
compilador. Los avisos hay que mirarlos uno a uno.

Uso:  python3 Tools/VerificarDatasets.py
"""
import json
import math
import os
import re
from collections import defaultdict

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATOS = os.path.join(RAIZ, "Content", "Datos")
FUENTE = os.path.join(RAIZ, "Source")

RE_DATASET = re.compile(r'Datos/([A-Za-z_0-9]+\.json)')
RE_CAMPO = re.compile(r'(?:TryGetArrayField|TryGetObjectField|TryGetStringField|'
                      r'TryGetNumberField|TryGetBoolField|GetArrayField|GetObjectField|'
                      r'GetStringField|GetNumberField|GetBoolField|HasField)'
                      r'\s*\(\s*TEXT\("([^"]+)"\)')

# Campos que se piden a propósito sabiendo que pueden no estar (metadatos,
# extensiones opcionales) o que vienen de otra fuente que no es Content/Datos.
IGNORAR = {"", "_comment", "_source", "_fuente", "_nota", "_metodo", "_fecha"}


def claves(valor, prof=0, acc=None):
    """Todas las claves que aparecen en la estructura, hasta 4 niveles."""
    if acc is None:
        acc = set()
    if prof > 4:
        return acc
    if isinstance(valor, dict):
        for k, v in valor.items():
            acc.add(k)
            claves(v, prof + 1, acc)
    elif isinstance(valor, list):
        # No hace falta recorrer los 2783 árboles para sacar {x,z,especie,altura},
        # pero tampoco vale mirar sólo los primeros: los campos anidados y
        # opcionales (tiendas_planta_baja de building_facades.json) no salen hasta
        # bastante entrada la lista, y sin ellos esto los daba por inexistentes.
        for v in valor[:200]:
            claves(v, prof + 1, acc)
    return acc


def main():
    datasets = {}
    for f in sorted(os.listdir(DATOS)):
        if not f.endswith(".json"):
            continue
        try:
            with open(os.path.join(DATOS, f), encoding="utf-8") as fh:
                d = json.load(fh)
        except Exception as e:
            print("  %-32s ILEGIBLE: %s" % (f, e))
            continue
        datasets[f] = {
            "raiz": "array" if isinstance(d, list) else "objeto",
            "claves": claves(d),
            "n": len(d) if isinstance(d, (list, dict)) else 0,
        }

    print("Forma de raíz de cada dataset\n")
    for f, info in datasets.items():
        print("  %-32s %-7s %4d" % (f, info["raiz"], info["n"]))

    # --- Campos pedidos que no existen -------------------------------------
    print("\n\nCampos que el C++ pide y el dato no tiene\n")
    sospechas = defaultdict(list)
    for base, _, ficheros in os.walk(FUENTE):
        for nombre in ficheros:
            if not nombre.endswith((".cpp", ".h")):
                continue
            ruta = os.path.join(base, nombre)
            with open(ruta, encoding="utf-8", errors="ignore") as fh:
                texto = fh.read()

            usados = [d for d in RE_DATASET.findall(texto) if d in datasets]
            if not usados:
                continue

            disponibles = set()
            for d in usados:
                disponibles |= datasets[d]["claves"]

            for campo in set(RE_CAMPO.findall(texto)):
                if campo in IGNORAR or campo in disponibles:
                    continue
                sospechas[os.path.relpath(ruta, RAIZ)].append((campo, usados))

    if not sospechas:
        print("  Ninguno. Todo lo que se pide existe en el dato.")
    else:
        for ruta, items in sorted(sospechas.items()):
            print("  %s" % ruta)
            for campo, usados in sorted(items):
                print('      "%s"  —  no está en %s' % (campo, ", ".join(usados)))
        print("\n  Cada línea es un TryGet/Get que nunca casa. Si de él depende un")
        print("  `return`, ese sistema no hace nada y no lo dice. Míralos uno a uno:")
        print("  puede ser un campo opcional legítimo o un sistema entero muerto.")

    marcos(datasets)


# Hueco mínimo en la coordenada norte para sospechar de dos marcos. Un dataset en
# un solo marco reparte sus piezas por el pueblo sin dejar kilómetros vacíos; dos
# marcos superpuestos dejan un salto del orden de OZ=8570.
#
# Un umbral fijo NO vale, y conviene recordar por qué: cortar por z>6000 marcaba
# también trees_unity.json (2783 árboles repartidos de forma continua, con un
# "salto" de 1 m justo en el corte) y signage_data.json. Los dos están en un solo
# marco; era el umbral el que partía una distribución continua por la mitad.
HUECO_MINIMO_M = 2000.0


def marcos(datasets):
    """Avisa de los datasets que mezclan local relativo y local absoluto.

    street_furniture.json lo hace: 191 de sus 220 piezas están en relativo y 29
    en absoluto, porque las escribieron dos generadores distintos. Convertirlo
    entero con un solo marco manda un grupo u otro a 8,6 km del pueblo, que es lo
    que le pasaba al mobiliario. Y no se nota: las piezas existen, tienen malla y
    están colocadas, sólo que fuera del terreno.

    Esto es un indicio, no un veredicto: mide el mayor hueco en la coordenada
    norte. Míralo antes de creértelo.
    """
    print("\n\nDatasets sospechosos de mezclar marcos (hueco en la coordenada norte)\n")
    hallazgos = 0
    for f, info in datasets.items():
        if "x" not in info["claves"] or "z" not in info["claves"]:
            continue
        with open(os.path.join(DATOS, f), encoding="utf-8") as fh:
            doc = json.load(fh)
        elems = doc if isinstance(doc, list) else next(
            (v for v in doc.values() if isinstance(v, list)), [])

        zs = sorted(e["z"] for e in elems
                    if isinstance(e, dict) and isinstance(e.get("z"), (int, float)))
        if len(zs) < 4:
            continue

        hueco, corte = 0.0, None
        for a, b in zip(zs, zs[1:]):
            if b - a > hueco:
                hueco, corte = b - a, (a, b)
        if hueco < HUECO_MINIMO_M:
            continue

        bajos = sum(1 for z in zs if z <= corte[0])
        hallazgos += 1
        print("  %-30s %d por debajo y %d por encima de un hueco de %.0f m"
              % (f, bajos, len(zs) - bajos, hueco))
        print("      el salto va de z=%.1f a z=%.1f; OZ vale 8570" % corte)

    if not hallazgos:
        print("  Ninguno: cada dataset posicional está en un solo marco.")
    else:
        print("\n  Si son dos marcos, quien lo lea tiene que decidirlo por elemento.")
        print("  UAlsasuaGeoData::MobiliarioAUE5 hace justo eso para el mobiliario.")

    fuera_del_mundo(datasets)
    latlon_contra_xz(datasets)


def caja_del_terreno():
    """Caja del terreno jugable, leída de GeoDataAlsasua.h.

    En metros locales absolutos, que es como vienen los JSON. El C++ la tiene en
    centímetros de mundo (CentroTerrenoXCm/YCm y SemiTerrenoCm) y ahí es donde
    manda: si alguien reajusta el terreno, este script se entera solo en vez de
    seguir midiendo contra una caja que ya no existe.

    Si no se puede leer el header se cae a los valores de hoy, para que el script
    siga sirviendo en un clon incompleto, pero avisando.
    """
    cabecera = os.path.join(RAIZ, "Source", "AlsasuaCore", "Public", "GeoDataAlsasua.h")
    try:
        with open(cabecera, encoding="utf-8") as fh:
            src = fh.read()
        semi_cm = float(re.search(r"SemiTerrenoCm\s*=\s*([0-9.]+)", src).group(1))
        ox = float(re.search(r"\bOX\s*=\s*([0-9.]+)", src).group(1))
        oz = float(re.search(r"\bOZ\s*=\s*([0-9.]+)", src).group(1))
        return ox, oz, semi_cm / 100.0
    except (OSError, AttributeError, ValueError):
        print("  (aviso: no pude leer la caja de GeoDataAlsasua.h; uso 7200x7200 m)")
        return 1918.0, 8570.0, 3600.0


OX, OZ, SEMILADO_M = caja_del_terreno()


def fuera_del_mundo(datasets):
    """Elementos con coordenadas fuera del terreno, en cualquiera de los marcos.

    signage_data.json trae 30 de sus 126 señales así: hasta 123 km al oeste y 215
    km al sur, todas de los tipos que el generador situaba por dirección. Nadie se
    entera, porque colocar una señal a 200 km no falla: el actor se crea, su trazo
    de suelo no encuentra terreno y cae a la cota de la plaza. Sólo estira los
    límites del mundo y se lleva un actor por delante.
    """
    print("\n\nElementos fuera del terreno jugable, en los dos marcos\n")
    hallazgos = 0
    for f, info in datasets.items():
        if "x" not in info["claves"] or "z" not in info["claves"]:
            continue
        with open(os.path.join(DATOS, f), encoding="utf-8") as fh:
            doc = json.load(fh)
        elems = doc if isinstance(doc, list) else next(
            (v for v in doc.values() if isinstance(v, list)), [])

        fuera, total, peor = 0, 0, 0.0
        for e in elems:
            if not isinstance(e, dict):
                continue
            x, z = e.get("x"), e.get("z")
            if not isinstance(x, (int, float)) or not isinstance(z, (int, float)):
                continue
            total += 1
            # Se da por bueno si cabe leyéndolo como absoluto O como relativo:
            # sólo se cuenta lo que no encaja de ninguna de las dos formas.
            dentro = any(abs(ax - OX) <= SEMILADO_M and abs(az - OZ) <= SEMILADO_M
                         for ax, az in ((x, z), (x + OX, z + OZ)))
            if not dentro:
                fuera += 1
                peor = max(peor, min(abs(x - OX), abs(x)) / 1000.0,
                           min(abs(z - OZ), abs(z)) / 1000.0)
        if fuera:
            hallazgos += 1
            print("  %-30s %d de %d fuera (el más lejano, a unos %.0f km)"
                  % (f, fuera, total, peor))

    if not hallazgos:
        print("  Ninguno.")
    else:
        print("\n  Colocarlos no falla y no avisa. Quien los lea debería filtrarlos")
        print("  diciendo cuántos, con UAlsasuaGeoData::DentroDelTerreno, que es")
        print("  lo que hacen AlsasuaSignPlacer y AlsasuaContainerSystem.")



def latlon_contra_xz(datasets):
    """Datasets que traen lat/lon Y x/z, y no dicen lo mismo.

    Dos ficheros —landmarks_real.json y poi_data.json— llevan las dos cosas, y
    salieron descuadradas: medidas entre elementos, las distancias por x/z son
    diez veces más pequeñas que por coordenada geográfica, y la z va al revés
    (el factor sale -10, no +10). Colocar por x/z apiña los 19 landmarks en
    121x134 m, a 120 m de mediana del edificio más cercano y sin que ninguno
    caiga sobre uno.

    No hay nada que falle al leerlo: los dos campos son números válidos y quien
    elija mal se lleva el pueblo entero movido. Esto compara los dos marcos por
    pares de elementos y saca el factor real, que es lo que delata el problema.

    El veredicto se cierra fuera: lo que decide cuál de los dos sirve es si los
    elementos caen sobre los footprints de buildings_final.json, y eso se mide
    aparte. Aquí se avisa de que discrepan.
    """
    print("\n\nCoherencia entre lat/lon y x/z\n")

    R = 6371000.0
    hallazgos = 0
    for f in sorted(datasets):
        with open(os.path.join(DATOS, f), encoding="utf-8") as fh:
            doc = json.load(fh)
        elems = doc if isinstance(doc, list) else next(
            (v for v in doc.values() if isinstance(v, list)), [])
        con = [e for e in elems
               if isinstance(e, dict) and {"lat", "lon", "x", "z"} <= set(e)]
        if len(con) < 3:
            continue

        lat0 = sum(e["lat"] for e in con) / len(con)
        lon0 = sum(e["lon"] for e in con) / len(con)
        x0 = sum(e["x"] for e in con) / len(con)
        z0 = sum(e["z"] for e in con) / len(con)
        cosl = math.cos(math.radians(lat0))

        fx, fz = [], []
        for e in con:
            este = math.radians(e["lon"] - lon0) * R * cosl
            norte = math.radians(e["lat"] - lat0) * R
            dx, dz = e["x"] - x0, e["z"] - z0
            if abs(dx) > 1.0:
                fx.append(este / dx)
            if abs(dz) > 1.0:
                fz.append(norte / dz)
        if not fx or not fz:
            continue

        fx.sort()
        fz.sort()
        mx, mz = fx[len(fx) // 2], fz[len(fz) // 2]

        # Coherente = la geográfica y la local miden lo mismo y en el mismo
        # sentido: factor +1 en los dos ejes, con holgura para el datum.
        ok_x = abs(mx - 1.0) < 0.1
        ok_z = abs(mz - 1.0) < 0.1
        if ok_x and ok_z:
            print("  %-30s %d elementos: los dos marcos coinciden" % (f, len(con)))
            continue

        hallazgos += 1
        print("  %-30s %d elementos: NO coinciden" % (f, len(con)))
        print("       metro geográfico por unidad de x: %+.2f   de z: %+.2f" % (mx, mz))
        if abs(abs(mx) - abs(mz)) < 0.5 and abs(mx) > 1.5:
            print("       → x/z vienen a escala 1:%.0f" % abs(mx))
        if mz < 0:
            print("       → la z crece hacia el SUR; en el resto del proyecto crece al norte")

    if hallazgos:
        print("\n  Quien lea uno de estos tiene que elegir marco, y elegir mal")
        print("  mueve el pueblo entero sin que falle nada. CargadorPOI y")
        print("  AlsasuaFacadeGenerator usan lat/lon cuando está, que es la que")
        print("  cae sobre los footprints de buildings_final.json.")
    else:
        print("\n  Ningún dataset mezcla los dos marcos de forma incoherente.")

if __name__ == "__main__":
    main()
