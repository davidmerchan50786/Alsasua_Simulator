"""
VerificarModulos.py — Que el grafo de módulos que dice el código sea el que
declaran los .Build.cs, y que no aparezcan ciclos nuevos.

Son dos errores de compilación que aquí no ve nadie, porque el repo no compila en
Linux:

  1. Un #include a una cabecera de otro módulo sin que ese módulo esté en las
     dependencias del .Build.cs. UBT no encuentra la cabecera y falla. Pasa al
     mover una constante a un sitio común: al llevar la ruta del MPC a
     AlsasuaCore hubo que añadir "AlsasuaCore" a AlsasuaSimulator, que no lo
     tenía.

  2. Un ciclo nuevo entre módulos. UBT 5.8 los marca como ERROR, no como aviso.
     Hoy el grafo es acíclico: el único que hubo —AlsasuaManifa <-> AlsasuaUI,
     confesado con CircularlyReferencedDependentModules— murió con el módulo en
     la Fase 5. Cualquier aparición se canta; si algún día hay que convivir con
     uno confesado, se lista aquí la arista a ignorar.

Ojo con la dirección real de las dependencias, que es fácil de suponer al revés:
AlsasuaWorld depende de AlsasuaKernel, no al contrario. Un #include de
EdificioGenerado.h (World) desde un sistema de Manifa parece inocente y es un
ciclo.

Uso:  python3 Tools/VerificarModulos.py      (salida != 0 si encuentra algo)
"""
import os
import re
import sys
from collections import defaultdict

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FUENTE = os.path.join(RAIZ, "Source")

RE_INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.M)
RE_CADENA = re.compile(r'"(\w+)"')
RE_CICLO_OK = re.compile(r'CircularlyReferencedDependentModules\.Add\(\s*"(\w+)"\s*\)')


def modulos_del_proyecto():
    return sorted(d for d in os.listdir(FUENTE)
                  if os.path.isdir(os.path.join(FUENTE, d)))


def duenio_de_cabecera(modulos):
    """
    {ruta de include tal como se escribe} -> módulo que la exporta.

    UBT resuelve contra los Public/ y Private/ de cada módulo, así que
    "World/AlsasuaMuros.h" y "AlsasuaMuros.h" son las dos formas de la misma.
    """
    duenio = {}
    for m in modulos:
        for base, _, ficheros in os.walk(os.path.join(FUENTE, m)):
            for nombre in ficheros:
                if not nombre.endswith(".h"):
                    continue
                rel = os.path.relpath(os.path.join(base, nombre), os.path.join(FUENTE, m))
                partes = rel.replace(os.sep, "/").split("/")
                for i, p in enumerate(partes):
                    if p in ("Public", "Private"):
                        for j in range(i + 1, len(partes)):
                            duenio.setdefault("/".join(partes[j:]), m)
                        break
                duenio.setdefault(nombre, m)
    return duenio


def declaradas(modulos):
    """{módulo: (dependencias declaradas, ciclos confesados)}"""
    fuera = {}
    for m in modulos:
        ruta = os.path.join(FUENTE, m, "%s.Build.cs" % m)
        if not os.path.exists(ruta):
            continue
        texto = open(ruta, encoding="utf-8", errors="ignore").read()
        fuera[m] = (set(RE_CADENA.findall(texto)),
                    set(RE_CICLO_OK.findall(texto)))
    return fuera


def buscar_ciclos(grafo):
    """Ciclos dirigidos, como lista de rutas."""
    ciclos, estado, camino = [], {}, []

    def visitar(n):
        estado[n] = 1
        camino.append(n)
        for v in sorted(grafo.get(n, ())):
            if estado.get(v, 0) == 0:
                visitar(v)
            elif estado.get(v) == 1:
                ciclos.append(camino[camino.index(v):] + [v])
        camino.pop()
        estado[n] = 2

    for n in sorted(grafo):
        if estado.get(n, 0) == 0:
            visitar(n)
    return ciclos


def main():
    modulos = modulos_del_proyecto()
    duenio = duenio_de_cabecera(modulos)
    decl = declaradas(modulos)

    fallos = []
    usados = defaultdict(lambda: defaultdict(set))   # módulo -> otro -> includes

    for m in modulos:
        if m not in decl:
            continue
        for base, _, ficheros in os.walk(os.path.join(FUENTE, m)):
            for nombre in ficheros:
                if not nombre.endswith((".cpp", ".h")):
                    continue
                ruta = os.path.join(base, nombre)
                texto = open(ruta, encoding="utf-8", errors="ignore").read()
                for inc in RE_INCLUDE.findall(texto):
                    otro = duenio.get(inc) or duenio.get(inc.split("/")[-1])
                    if not otro or otro == m:
                        continue
                    usados[m][otro].add("%s -> %s" % (os.path.relpath(ruta, RAIZ), inc))

    # 1. Dependencias que se usan y no se declaran.
    for m in sorted(usados):
        deps, _ = decl[m]
        for otro in sorted(usados[m]):
            if otro in deps:
                continue
            ejemplos = sorted(usados[m][otro])
            fallos.append(
                "%s incluye cabeceras de %s y su .Build.cs no lo declara (%d includes).\n"
                "      UBT no las encontrará. Ejemplo: %s"
                % (m, otro, len(ejemplos), ejemplos[0]))

    # 2. Ciclos, sobre lo DECLARADO, que es lo que mira UBT.
    #
    # La arista confesada se QUITA del grafo antes de buscar, que es lo que
    # significa CircularlyReferencedDependentModules: "esta arista cierra un
    # lazo, ya lo sé, trátala aparte". Descartar sólo el ciclo de longitud 2 no
    # vale, porque el lazo real es más largo de lo que sugiere su nombre:
    # AlsasuaManifa -> AlsasuaUI es la arista confesada, pero AlsasuaUI baja
    # luego por Gameplay y por World, que a su vez suben a Manifa. Son tres
    # ciclos distintos y los tres pasan por la misma arista.
    grafo = {m: {d for d in decl[m][0] if d in decl} for m in decl}
    for m, (_, ciclos_ok) in decl.items():
        for otro in ciclos_ok:
            grafo.get(m, set()).discard(otro)

    for ciclo in buscar_ciclos(grafo):
        fallos.append("ciclo entre módulos: %s\n"
                      "      UBT 5.8 lo marca como ERROR. El único admitido es el\n"
                      "      heredado, y va declarado con CircularlyReferencedDependentModules"
                      % " -> ".join(ciclo))

    print("%d módulos, %d con .Build.cs.\n" % (len(modulos), len(decl)))
    if not fallos:
        print("El grafo declarado cuadra con el que usa el código, y no hay")
        print("ciclos aparte del confesado.")
        return 0

    for f in fallos:
        print("  " + f)
    print("\n%d hallazgo(s)." % len(fallos))
    return 1


if __name__ == "__main__":
    sys.exit(main())
