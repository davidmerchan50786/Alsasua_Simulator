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


if __name__ == "__main__":
    main()
