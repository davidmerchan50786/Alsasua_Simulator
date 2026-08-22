"""
VerificarRedViaria.py — El grafo de calzada, contrastado contra el dato.

UAlsasuaRedViaria construye en runtime un grafo de nodos y tramos a partir de
roads_unity.json. Este script hace lo mismo en Python y saca los números, que es
la única forma de comprobar el grafo sin motor: si el C++ dice que ha cargado
402 vías y aquí salen 247, alguien está filtrando de más.

Antes del grafo, cada sistema leía el JSON por su cuenta y se quedaba una lista
de polilíneas sueltas. Un coche recorría UNA calle y al llegar al final volvía
de un salto al punto 0. No había por dónde girar porque no había cruces.

Uso:  python3 Tools/VerificarRedViaria.py      (salida != 0 si algo no cuadra)
"""
import json
import os
import sys
from collections import Counter, defaultdict

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATOS = os.path.join(RAIZ, "Content", "Datos", "roads_unity.json")

# Tiene que coincidir con EsConducible() de AlsasuaRedViaria.cpp.
CONDUCIBLE = {"motorway", "motorway_link", "trunk", "trunk_link",
              "primary", "secondary", "tertiary", "residential",
              "service", "unclassified", "living_street"}
# Y esto con EsPeatonal().
PEATONAL = {"pedestrian", "path", "footway", "steps", "track", "cycleway"}

# Redondeo con el que se identifican los nodos. Igual que el de C++: los datos
# vienen de OSM y los cruces comparten coordenada exacta, así que con milímetros
# sobra y no hace falta encaje difuso.
DEC = 3


def nodo(p):
    return (round(p["x"], DEC), round(p["z"], DEC))


def main():
    with open(DATOS, encoding="utf-8") as fh:
        vias = json.load(fh)

    conducibles = [v for v in vias if v.get("type") in CONDUCIBLE]
    peatonales = [v for v in vias if v.get("type") in PEATONAL]
    sin_clasificar = [v for v in vias
                      if v.get("type") not in CONDUCIBLE and v.get("type") not in PEATONAL]

    nodos = {}
    grado = Counter()
    tramos = 0
    unidireccionales = 0
    longitud = 0.0

    for v in conducibles:
        pts = v.get("points", [])
        if len(pts) < 2:
            continue
        if v.get("oneway"):
            unidireccionales += 1
        for i in range(len(pts) - 1):
            a, b = nodo(pts[i]), nodo(pts[i + 1])
            if a == b:
                continue
            nodos.setdefault(a, len(nodos))
            nodos.setdefault(b, len(nodos))
            grado[a] += 1
            grado[b] += 1
            tramos += 1
            longitud += ((a[0] - b[0]) ** 2 + (a[1] - b[1]) ** 2) ** 0.5

    cruces = {n for n, g in grado.items() if g > 2}
    extremos = {n for n, g in grado.items() if g == 1}

    # Componentes conexas: un grafo partido en trozos es un grafo por el que no
    # se puede circular de verdad.
    ady = defaultdict(set)
    for v in conducibles:
        pts = v.get("points", [])
        for i in range(len(pts) - 1):
            a, b = nodo(pts[i]), nodo(pts[i + 1])
            if a != b:
                ady[a].add(b)
                ady[b].add(a)

    vistos, componentes = set(), []
    for n in ady:
        if n in vistos:
            continue
        pila, comp = [n], 0
        vistos.add(n)
        while pila:
            x = pila.pop()
            comp += 1
            for y in ady[x]:
                if y not in vistos:
                    vistos.add(y)
                    pila.append(y)
        componentes.append(comp)
    componentes.sort(reverse=True)

    print("Red viaria de roads_unity.json\n")
    print("  vías totales            %4d" % len(vias))
    print("  conducibles             %4d   (las que usa el tráfico)" % len(conducibles))
    print("  peatonales              %4d   (aceras, sendas, escaleras)" % len(peatonales))
    if sin_clasificar:
        tipos = Counter(v.get("type") for v in sin_clasificar)
        print("  SIN CLASIFICAR          %4d   %s" % (len(sin_clasificar), dict(tipos)))
    print()
    print("  nodos                   %4d" % len(nodos))
    print("  tramos dirigidos        %4d" % tramos)
    print("  cruces (grado > 2)      %4d" % len(cruces))
    print("  finales de línea        %4d" % len(extremos))
    print("  vías de sentido único   %4d" % unidireccionales)
    print("  longitud total          %.1f km" % (longitud / 1000.0))
    print()
    print("  componentes conexas     %4d   mayor: %d nodos (%.0f%%)"
          % (len(componentes), componentes[0], componentes[0] / len(nodos) * 100))

    fallos = []
    if sin_clasificar:
        fallos.append("%d vías con un 'type' que no está ni en CONDUCIBLE ni en PEATONAL.\n"
                      "      El grafo las ignora en silencio; añádelas a la lista que toque"
                      % len(sin_clasificar))
    if len(componentes) > 1 and componentes[0] / len(nodos) < 0.80:
        fallos.append("la componente mayor sólo tiene el %.0f%% de los nodos: el grafo está\n"
                      "      partido y un coche no puede llegar de un trozo a otro"
                      % (componentes[0] / len(nodos) * 100))
    if len(cruces) < 50:
        fallos.append("sólo %d cruces para %d vías: si los tramos no comparten coordenada\n"
                      "      exacta, el grafo no conecta y hace falta encaje por proximidad"
                      % (len(cruces), len(conducibles)))

    if fallos:
        print()
        for f in fallos:
            print("  " + f)
        print("\n%d hallazgo(s)." % len(fallos))
        return 1

    print("\n  El grafo conecta. Estos son los números que tiene que dar el log de")
    print("  UAlsasuaRedViaria al arrancar; si no coinciden, el C++ filtra distinto.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
