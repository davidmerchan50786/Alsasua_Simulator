"""
InventarioAssetsDiscos.py — Qué contenido de Unreal hay repartido por los discos
y qué parte ya está en el proyecto.

Busca carpetas Content/ de cualquier proyecto o pack (Fab, Megascans, proyectos
viejos, copias) y las agrupa por pack: la carpeta de primer nivel dentro de
Content/ (Content/Megascans, Content/UnrealDrive_CitySample...). De cada pack
da cuántos .uasset/.umap tiene, cuánto ocupa, en qué proyecto está y si el
proyecto ACTIVO ya tiene un pack con ese nombre.

Sólo lee y escribe un informe; no copia nada. Para traer un pack, lo correcto
es MIGRAR desde el editor del proyecto origen (clic derecho en la carpeta >
Asset Actions > Migrate), que arrastra sus dependencias y respeta las rutas
/Game/...; copiar .uasset sueltos entre proyectos rompe referencias si la ruta
no coincide.

Uso (Python 3 normal):
  python Tools/InventarioAssetsDiscos.py                 # todas las unidades
  python Tools/InventarioAssetsDiscos.py --discos D E F
Deja el informe en Saved/InventarioAssets.md
"""
import argparse
import os
import string
import sys
from collections import defaultdict

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CONTENT = os.path.join(RAIZ, "Content")
SALIDA = os.path.join(RAIZ, "Saved", "InventarioAssets.md")
EXT = (".uasset", ".umap")
SALTAR = {"windows", "$recycle.bin", "system volume information", "programdata",
          "intermediate", "deriveddatacache", "binaries", "saved", ".git",
          "node_modules", "appdata"}


def discos(lista):
    if lista:
        return [d if len(d) > 2 or os.name != "nt" else d.rstrip(":\\") + ":\\" for d in lista]
    if os.name != "nt":
        return ["/"]
    return [d + ":\\" for d in string.ascii_uppercase if os.path.exists(d + ":\\")]


def recorrer(raices):
    """{(raíz_content, pack): [n_ficheros, bytes]}"""
    packs = defaultdict(lambda: [0, 0])
    activo = os.path.normcase(os.path.abspath(RAIZ))
    for raiz in raices:
        print("  recorriendo %s ..." % raiz, flush=True)
        for base, dirs, ficheros in os.walk(raiz):
            dirs[:] = [d for d in dirs if d.lower() not in SALTAR]
            norm = os.path.normcase(os.path.abspath(base))
            if norm.startswith(activo):
                dirs[:] = []
                continue
            trozos = base.replace("\\", "/").split("/")
            if "Content" not in trozos:
                continue
            i = len(trozos) - 1 - trozos[::-1].index("Content")
            raiz_content = "/".join(trozos[:i + 1])
            pack = trozos[i + 1] if len(trozos) > i + 1 else "(raíz de Content)"
            for f in ficheros:
                if f.lower().endswith(EXT):
                    try:
                        tam = os.path.getsize(os.path.join(base, f))
                    except OSError:
                        continue
                    e = packs[(raiz_content, pack)]
                    e[0] += 1
                    e[1] += tam
    return packs


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--discos", nargs="*")
    a = ap.parse_args()
    ya = {d.lower() for d in os.listdir(CONTENT)} if os.path.isdir(CONTENT) else set()
    packs = recorrer(discos(a.discos))

    porpack = defaultdict(list)
    for (rc, pack), (n, b) in packs.items():
        porpack[pack].append((rc, n, b))

    filas = sorted(porpack.items(), key=lambda kv: -max(x[2] for x in kv[1]))
    os.makedirs(os.path.dirname(SALIDA), exist_ok=True)
    total = sum(b for v in porpack.values() for _, _, b in v)
    with open(SALIDA, "w", encoding="utf-8") as fh:
        fh.write("# Inventario de assets en los discos\n\n")
        fh.write("%d packs distintos, %.1f GB en total (contando copias repetidas).\n\n"
                 % (len(porpack), total / 1e9))
        fh.write("| Pack | ¿En el proyecto? | Copias | Mayor copia | Ficheros | Dónde |\n|---|---|---|---|---|---|\n")
        for pack, copias in filas:
            rc, n, b = max(copias, key=lambda x: x[2])
            fh.write("| %s | %s | %d | %.2f GB | %d | %s |\n"
                     % (pack, "sí" if pack.lower() in ya else "**no**", len(copias), b / 1e9, n, rc))
    print("\n%d packs, %.1f GB. Informe: %s" % (len(porpack), total / 1e9, SALIDA))
    for pack, copias in filas[:25]:
        rc, n, b = max(copias, key=lambda x: x[2])
        print("  %-40s %-3s %7.2f GB  %6d fich.  %d copia(s)  %s"
              % (pack[:40], "sí" if pack.lower() in ya else "NO", b / 1e9, n, len(copias), rc))
    return 0


if __name__ == "__main__":
    sys.exit(main())
