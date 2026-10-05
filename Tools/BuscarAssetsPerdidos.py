"""
BuscarAssetsPerdidos.py — Encuentra en los discos duros los assets que faltan al
cargar el proyecto, y (si se le pide) los copia a su sitio en Content/.

Qué cuenta como "falta":
  1. Paquetes /Game/... que el log de Unreal dice que no encuentra
     ("Failed to load", "Can't find file", "Couldn't find file for package"...).
  2. Ficheros de Content/ que son PUNTEROS de Git LFS (~130 bytes que empiezan
     por "version https://git-lfs"): existen, pero sin contenido. Así llegaron
     L_Alsasua.umap y L_Sakana.umap, y Unreal los da por inexistentes.

Cómo los busca: recorre los discos que se le digan (por defecto, todas las
unidades de Windows que existan) y se queda con los .uasset/.umap del mismo
nombre. Si hay varios candidatos, gana el que comparte más carpetas finales
con la ruta esperada (.../Content/Vehiculos/Coches/SM_Coche.uasset), luego el
que está dentro de una carpeta Content, y luego el más reciente. Nunca da por bueno otro puntero LFS.

Por defecto sólo informa. Con --copiar copia los encontrados a Content/,
creando las carpetas, y nunca pisa un fichero real que ya exista (un puntero
LFS sí se sustituye). Después hay que volver a abrir el editor: un asset
recuperado puede depender de otros que aparecerán en el siguiente log.

Uso (Python 3 normal, fuera del editor):
  python Tools/BuscarAssetsPerdidos.py
  python Tools/BuscarAssetsPerdidos.py --log Saved/Logs/AlsasuaSimulator.log --discos D E F H I J K
  python Tools/BuscarAssetsPerdidos.py --copiar
"""
import argparse
import os
import re
import shutil
import string
import sys
from collections import defaultdict

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CONTENT = os.path.join(RAIZ, "Content")
LOG_DEFECTO = os.path.join(RAIZ, "Saved", "Logs", "AlsasuaSimulator.log")
EXT = (".uasset", ".umap")

# Frases con las que Unreal dice que no encuentra un paquete.
RE_FALLO = re.compile(r"(Failed to load|Can't find file|Couldn't find file|Failed to find object|"
                      r"could not be found|Unable to load|Missing|does not exist|LoadPackage)", re.I)
RE_PAQUETE = re.compile(r"/Game/[A-Za-z0-9_\-/ ]+[A-Za-z0-9_\-]")

# Carpetas que no tiene sentido recorrer.
SALTAR = {"windows", "$recycle.bin", "system volume information", ".git",
          "intermediate", "deriveddatacache", "binaries", "saved", "node_modules"}
# ProgramData y AppData NO se saltan a propósito: ahí guardan los packs el
# Epic Launcher (VaultCache) y Fab, y es donde suele estar lo que se bajó.


def es_puntero_lfs(ruta):
    try:
        if os.path.getsize(ruta) > 1024:
            return False
        with open(ruta, "rb") as fh:
            return fh.read(40).startswith(b"version https://git-lfs")
    except OSError:
        return False


def paquetes_del_log(ruta_log):
    """{'/Game/A/B': línea de ejemplo} de las líneas del log que hablan de un fallo."""
    faltan = {}
    if not os.path.exists(ruta_log):
        print("  (no hay log en %s; sólo se buscarán punteros LFS)" % ruta_log)
        return faltan
    with open(ruta_log, encoding="utf-8", errors="ignore") as fh:
        for linea in fh:
            if not RE_FALLO.search(linea):
                continue
            for p in RE_PAQUETE.findall(linea):
                p = p.split(".")[0].rstrip("/ ")
                if p.count("/") >= 2:
                    faltan.setdefault(p, linea.strip()[:160])
    return faltan


def relativa_de_paquete(paquete):
    """'/Game/A/B' -> ['A', 'B'] (sin extensión)."""
    return paquete[len("/Game/"):].split("/")


def existe_en_content(partes):
    base = os.path.join(CONTENT, *partes)
    for e in EXT:
        if os.path.exists(base + e) and not es_puntero_lfs(base + e):
            return True
    return False


def punteros_lfs():
    """Ficheros de Content/ que son punteros LFS: [(partes sin ext, ext)]."""
    out = []
    for base, dirs, ficheros in os.walk(CONTENT):
        for f in ficheros:
            if f.lower().endswith(EXT) and es_puntero_lfs(os.path.join(base, f)):
                rel = os.path.relpath(os.path.join(base, f), CONTENT)
                stem, ext = os.path.splitext(rel)
                out.append((stem.replace("\\", "/").split("/"), ext))
    return out


def discos_por_defecto():
    if os.name != "nt":
        return ["/"]
    return [d + ":\\" for d in string.ascii_uppercase if os.path.exists(d + ":\\")]


def indexar(raices, nombres):
    """{nombre_sin_ext_minúsculas: [ruta, ...]} de los .uasset/.umap que interesan."""
    idx = defaultdict(list)
    proyecto = os.path.normcase(os.path.abspath(RAIZ))
    for raiz in raices:
        print("  recorriendo %s ..." % raiz, flush=True)
        for base, dirs, ficheros in os.walk(raiz):
            dirs[:] = [d for d in dirs if d.lower() not in SALTAR]
            if os.path.normcase(os.path.abspath(base)).startswith(proyecto):
                dirs[:] = []
                continue
            for f in ficheros:
                stem, ext = os.path.splitext(f)
                if ext.lower() in EXT and stem.lower() in nombres:
                    idx[stem.lower()].append(os.path.join(base, f))
    return idx


def puntuar(candidato, partes):
    """Cuántas carpetas finales comparte con la ruta esperada."""
    trozos = os.path.normpath(candidato).replace("\\", "/").split("/")
    trozos[-1] = os.path.splitext(trozos[-1])[0]
    n = 0
    for a, b in zip(reversed(trozos), reversed(partes)):
        if a.lower() != b.lower():
            break
        n += 1
    return n


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--log", default=LOG_DEFECTO)
    ap.add_argument("--discos", nargs="*", help="unidades o carpetas a recorrer (por defecto, todas)")
    ap.add_argument("--copiar", action="store_true", help="copiar los encontrados a Content/")
    a = ap.parse_args()

    raices = [d if (len(d) > 2 or os.name != "nt") else d.rstrip(":\\") + ":\\" for d in (a.discos or discos_por_defecto())]

    buscados = {}   # tuple(partes) -> motivo
    for p, linea in paquetes_del_log(a.log).items():
        partes = relativa_de_paquete(p)
        if not existe_en_content(partes):
            buscados[tuple(partes)] = "log: " + linea
    for partes, ext in punteros_lfs():
        buscados.setdefault(tuple(partes), "puntero LFS sin contenido (%s)" % ext)

    print("\n%d asset(s) que faltan." % len(buscados))
    if not buscados:
        return 0

    idx = indexar(raices, {p[-1].lower() for p in buscados})
    encontrados, perdidos = [], []
    for partes, motivo in sorted(buscados.items()):
        cands = [c for c in idx.get(partes[-1].lower(), []) if not es_puntero_lfs(c)]
        if not cands:
            perdidos.append((partes, motivo))
            continue
        # Desempate: el que está dentro de una carpeta Content de proyecto, y luego el más reciente.
        mejor = max(cands, key=lambda c: (puntuar(c, partes),
                                          "/content/" in c.replace("\\", "/").lower(),
                                          os.path.getmtime(c)))
        encontrados.append((partes, mejor, puntuar(mejor, partes), len(cands)))

    print("\n== ENCONTRADOS (%d)" % len(encontrados))
    copiados = 0
    for partes, origen, punt, n in encontrados:
        destino = os.path.join(CONTENT, *partes) + os.path.splitext(origen)[1]
        print("  /Game/%s\n      <- %s  (coinciden %d carpetas; %d candidato(s))" % ("/".join(partes), origen, punt, n))
        if a.copiar:
            if os.path.exists(destino) and not es_puntero_lfs(destino):
                print("      ya existe un fichero real en destino: no se pisa")
                continue
            os.makedirs(os.path.dirname(destino), exist_ok=True)
            shutil.copy2(origen, destino)
            copiados += 1

    print("\n== NO ENCONTRADOS EN NINGÚN DISCO (%d)" % len(perdidos))
    for partes, motivo in perdidos:
        print("  /Game/%s   [%s]" % ("/".join(partes), motivo[:110]))

    if a.copiar:
        print("\n%d copiado(s) a Content/. Abre el editor otra vez: lo recuperado puede pedir más." % copiados)
    elif encontrados:
        print("\nRepite con --copiar para traerlos.")
    return 0 if not perdidos else 1


if __name__ == "__main__":
    sys.exit(main())
