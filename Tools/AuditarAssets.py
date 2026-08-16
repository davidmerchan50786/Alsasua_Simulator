"""
AuditarAssets.py — Cruza lo que el código pide contra lo que el proyecto tiene.

El repo ha ido acumulando rutas /Game/... escritas a mano, y varias apuntan a
sitios donde no hay nada (packs que no se descargaron, carpetas renombradas,
copiar-pegar de otro sistema). Cuando eso pasa el juego no peta: LoadObject
devuelve null y la pieza se queda invisible o con una primitiva, así que el
fallo es silencioso y sólo se ve mirando el mundo.

Esto lo saca a la luz sin abrir el editor:

  1. Recoge toda ruta /Game/... que aparezca en Source/.
  2. Recoge el inventario real: Content/ en disco, asset_manifest.json (fuentes
     descargadas) y Content/ModelosDescargados (glTF CC0).
  3. Dice qué rutas del código no tienen nada detrás, y qué assets del
     inventario no los usa nadie.

No prueba que un uasset exista dentro del editor — eso sólo lo sabe Unreal —,
pero sí detecta el caso que más duele: rutas que no casan con ninguna carpeta
ni ningún asset conocido del proyecto.

Uso:  python3 Tools/AuditarAssets.py [--json]
"""
import json
import os
import re
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SOURCE = os.path.join(RAIZ, "Source")
CONTENT = os.path.join(RAIZ, "Content")
DATOS = os.path.join(CONTENT, "Datos")

# Rutas /Game/... dentro de TEXT("...") o de un literal cualquiera.
RE_GAME = re.compile(r'/Game/[A-Za-z0-9_/\.\-]+')
# Las de /Engine/ son del motor y siempre están; no se auditan.


def rutas_del_codigo():
    """{ruta: [fichero:linea, ...]} de todo lo que el C++ pide bajo /Game/."""
    out = {}
    for base, _, files in os.walk(SOURCE):
        for f in files:
            if not f.endswith((".cpp", ".h")):
                continue
            p = os.path.join(base, f)
            rel = os.path.relpath(p, RAIZ)
            try:
                texto = open(p, encoding="utf-8", errors="ignore").read().splitlines()
            except OSError:
                continue
            for n, linea in enumerate(texto, 1):
                if linea.lstrip().startswith("//"):
                    continue          # comentarios: documentan, no cargan
                for m in RE_GAME.findall(linea):
                    # "/Game/X/Y.Y" -> el paquete es /Game/X/Y
                    ruta = m.split(".")[0].rstrip("/")
                    out.setdefault(ruta, []).append("%s:%d" % (rel, n))
    return out


def carpetas_de_content():
    """Carpetas reales bajo Content/, como rutas /Game/..."""
    dirs = set()
    for base, subdirs, _ in os.walk(CONTENT):
        rel = os.path.relpath(base, CONTENT).replace("\\", "/")
        if rel == ".":
            continue
        if rel.startswith((".git", "Intermediate", "Saved")):
            continue
        dirs.add("/Game/" + rel)
    return dirs


def assets_en_disco():
    """Assets reales en disco (uasset, gltf, fbx, glb) como rutas /Game/..."""
    out = set()
    for base, _, files in os.walk(CONTENT):
        for f in files:
            if not f.endswith((".uasset", ".gltf", ".glb", ".fbx")):
                continue
            rel = os.path.relpath(os.path.join(base, f), CONTENT).replace("\\", "/")
            out.add("/Game/" + os.path.splitext(rel)[0])
    return out


def inventario_manifest():
    """Nombres de asset descargados según asset_manifest.json."""
    p = os.path.join(DATOS, "asset_manifest.json")
    if not os.path.exists(p):
        return set()
    txt = open(p, encoding="utf-8").read()
    nombres = set()
    for m in re.findall(r'"path"\s*:\s*"([^"]+)"', txt):
        base = os.path.basename(m.replace("\\\\", "/").replace("\\", "/"))
        nombres.add(os.path.splitext(base)[0].lower())
    return nombres


# Raíces que vienen de packs externos (Fab, Megascans, CitySample, importaciones).
# Que falten es legítimo: el proyecto tiene que arrancar sin ellos y degradar.
PACKS_EXTERNOS = ("/Game/Megascans", "/Game/Fab", "/Game/CitySample", "/Game/GASP",
                  "/Game/Road", "/Game/AssetsImportados", "/Game/Characters",
                  "/Game/MSPresets", "/Game/Material", "/Game/Textures/SurfaceFeature")


def rutas_generadas():
    """Rutas que produce el propio proyecto: generadores de editor y scripts.

    Si una ruta aparece en AlsasuaEditor/ o en Tools/, es que algo la crea, así
    que no está rota aunque el uasset no exista todavía en un clon limpio.
    """
    out = set()
    for sub in (os.path.join(SOURCE, "AlsasuaEditor"), os.path.join(RAIZ, "Tools")):
        for base, _, files in os.walk(sub):
            for f in files:
                if not f.endswith((".cpp", ".h", ".py")):
                    continue
                try:
                    txt = open(os.path.join(base, f), encoding="utf-8", errors="ignore").read()
                except OSError:
                    continue
                for m in RE_GAME.findall(txt):
                    out.add(m.split(".")[0].rstrip("/"))
                # Los scripts componen rutas como f"{MATERIALS_PATH}/{name}";
                # con la carpeta basta para dar por generado lo que cuelgue de ella.
    return out


# El tipo no siempre llega a Resolver() como literal. AlsasuaRooftopDetailSystem
# lo pasa por un lambda —CrearItem(TEXT("antena"), ...) → CapaDe(Tipo, ...) →
# Resolver(Tipo, ...)—, así que mirando sólo Resolver( se le escapaban cuatro
# tipos sin entrada en la tabla, y con ellos la Antena_TV y la Placa_Solar que
# Meshy había generado para este pueblo. Se miran también los envoltorios.
RE_RESOLVER = re.compile(r'(?:Resolver|CrearItem|CapaDe)\(\s*TEXT\("([^"]+)"\)', re.S)
RE_TABLA = re.compile(r'\{\s*TEXT\("([^"]+)"\),\s*TEXT\(')


def tipos_sin_mapear():
    """Tipos que se piden a AlsasuaMallaFab y que su tabla de claves no conoce.

    Un tipo sin entrada no falla: BuscarEnFab sale sin candidato y la pieza cae a
    la forma básica. Es decir, se ve un cubo aunque la malla buena esté bajada.
    Se lee el fichero entero, no línea a línea, porque las llamadas se parten.
    """
    fab = os.path.join(SOURCE, "AlsasuaManifa", "Private", "World", "AlsasuaMallaFab.cpp")
    if not os.path.exists(fab):
        return {}
    tabla = set(RE_TABLA.findall(open(fab, encoding="utf-8", errors="ignore").read()))

    pedidos = {}
    for base, _, files in os.walk(SOURCE):
        for f in files:
            if not f.endswith(".cpp"):
                continue
            p = os.path.join(base, f)
            txt = open(p, encoding="utf-8", errors="ignore").read()
            for t in RE_RESOLVER.findall(txt):
                if t not in tabla:
                    pedidos.setdefault(t, []).append(os.path.relpath(p, RAIZ))
    return pedidos


def tipos_de_datos_sin_mapear():
    """Tipos de street_furniture.json que la tabla de claves no conoce."""
    fab = os.path.join(SOURCE, "AlsasuaManifa", "Private", "World", "AlsasuaMallaFab.cpp")
    sf = os.path.join(DATOS, "street_furniture.json")
    if not (os.path.exists(fab) and os.path.exists(sf)):
        return {}
    tabla = set(RE_TABLA.findall(open(fab, encoding="utf-8", errors="ignore").read()))
    cuenta = {}
    for e in json.load(open(sf, encoding="utf-8")):
        if not isinstance(e, dict):
            continue
        t = e.get("type") or e.get("tipo")
        if t and t not in tabla:
            cuenta[t] = cuenta.get(t, 0) + 1
    return cuenta


def mallas_sin_usar(top=25):
    """Mallas del inventario que ninguna clave de AlsasuaMallaFab reconoce.

    Es la pregunta al revés de todo lo demás: no "qué pide el código y no está",
    sino "qué está bajado y no lo usa nadie". Una malla que no casa con ninguna
    palabra clave no falla ni avisa — el sistema que la necesitaría cae a su
    forma básica y se ve un cubo con el modelo bueno en disco al lado.

    Se agrupa por carpeta del pack, porque lo accionable es "este pack entero no
    está mapeado", no cada fichero suelto.
    """
    fab = os.path.join(SOURCE, "AlsasuaManifa", "Private", "World", "AlsasuaMallaFab.cpp")
    p = os.path.join(DATOS, "asset_manifest.json")
    if not (os.path.exists(fab) and os.path.exists(p)):
        return {}, 0, 0

    # Todas las palabras clave de la tabla, en minúsculas.
    txt = open(fab, encoding="utf-8", errors="ignore").read()
    claves = set()
    for _, lista in re.findall(r'\{\s*TEXT\("([^"]+)"\),\s*TEXT\("([^"]+)"\)', txt):
        for c in lista.split("|"):
            c = c.strip().lower()
            if len(c) >= 3:
                claves.add(c)

    doc = json.load(open(p, encoding="utf-8"))
    porpack = {}
    total = casadas = 0
    for m in doc.get("meshes", []):
        ruta = str(m.get("path", "")).replace("\\\\", "/").replace("\\", "/")
        if not ruta:
            continue
        total += 1
        nombre = os.path.splitext(os.path.basename(ruta))[0].lower()
        # Casa si alguna clave aparece en el nombre del fichero o en su ruta.
        if any(c in nombre or c in ruta.lower() for c in claves):
            casadas += 1
            continue
        pack = ruta.split("/")[0] if "/" in ruta else "(raíz)"
        porpack.setdefault(pack, []).append(os.path.basename(ruta))

    return porpack, total, casadas


def main():
    codigo = rutas_del_codigo()
    dirs = carpetas_de_content()
    disco = assets_en_disco()
    manifest = inventario_manifest()
    generadas = rutas_generadas()
    carpetas_generadas = {r.rsplit("/", 1)[0] for r in generadas}

    # Nombres de asset que algún generador nombra literalmente. Sirve para
    # separar "la carpeta la genera el proyecto" de "y además crea ESTE asset".
    #
    # No basta con partir las rutas /Game/: los generadores casi nunca escriben
    # la ruta entera, pasan el nombre y la carpeta por separado —
    # CrearMalla(TEXT("SM_Iglesia"), TEXT("/Game/Landmarks"))— y en Python van en
    # variables. Así que se recogen también los literales sueltos con prefijo de
    # asset. Sin esto salían como huecos SM_Iglesia, SM_Banco o M_Terreno_Acera,
    # que sí los crea alguien.
    nombres_generados = {r.rsplit("/", 1)[-1] for r in generadas if "/" in r}
    RE_NOMBRE = re.compile(r'["\'](SM_[A-Za-z0-9_]+|M_[A-Za-z0-9_]+|MI_[A-Za-z0-9_]+|'
                           r'MPC_[A-Za-z0-9_]+|P_[A-Za-z0-9_]+|NS_[A-Za-z0-9_]+)["\']')
    for sub in (os.path.join(SOURCE, "AlsasuaEditor"), os.path.join(RAIZ, "Tools")):
        for base, _, files in os.walk(sub):
            for f in files:
                if not f.endswith((".cpp", ".h", ".py")):
                    continue
                try:
                    txt = open(os.path.join(base, f), encoding="utf-8", errors="ignore").read()
                except OSError:
                    continue
                nombres_generados |= set(RE_NOMBRE.findall(txt))

    rotas, ok, externas, gen, sin_nombre = {}, [], [], [], []
    for ruta, usos in sorted(codigo.items()):
        carpeta = ruta.rsplit("/", 1)[0]
        if ruta in disco or carpeta in dirs:
            ok.append(ruta); continue
        if os.path.basename(ruta).lower() in manifest:
            ok.append(ruta); continue
        # La carpeta cuenta como generada si el generador la nombra a ella
        # ("/Game/Mobiliario" en CreadorMallaMobiliario) aunque componga el
        # nombre del asset aparte. Antes sólo se miraba carpetas_generadas, que
        # son los PADRES de las rutas generadas, así que /Game/Mobiliario/SM_Banco
        # no casaba con nada y salían 57 alarmas falsas. Una auditoría que grita
        # sin motivo es peor que no tenerla: se deja de mirar.
        if ruta in generadas or carpeta in generadas or carpeta in carpetas_generadas:
            # Aceptar por carpeta hace falta —los generadores componen el nombre
            # del asset en tiempo de ejecución, "/Game/Mobiliario/SM_%s"— pero se
            # traga también lo que nadie crea con ese nombre exacto. Eso no está
            # roto (todos los cargadores comprueban el null y siguen), pero es una
            # función visual que nunca va a verse: se cuenta aparte en vez de
            # darla por buena.
            # Un nombre acabado en "_" es el prefijo de una ruta que el código
            # compone con Printf, no un asset; se descarta como en el caso de
            # abajo.
            if (nombres_generados and not ruta.endswith("_")
                    and os.path.basename(ruta) not in nombres_generados):
                sin_nombre.append(ruta)
            gen.append(ruta); continue
        if ruta.startswith(PACKS_EXTERNOS):
            externas.append(ruta); continue
        if ruta.endswith("_"):
            # Prefijo de una ruta que el código compone en tiempo de ejecución,
            # como "/Game/Meshes/Arboles/SM_%s" con Printf. No es una ruta real,
            # así que marcarla como rota es dar la alarma en falso.
            ok.append(ruta); continue
        rotas[ruta] = usos

    if "--json" in sys.argv:
        print(json.dumps({"rotas": rotas, "ok": ok, "generadas": gen,
                          "externas": externas}, indent=1, ensure_ascii=False))
        return

    print("=" * 74)
    print("  AUDITORÍA DE ASSETS — rutas /Game/ que pide el código")
    print("=" * 74)
    if sin_nombre:
        print()
        print("=" * 74)
        print("  LA CARPETA SÍ, EL ASSET NO — %d rutas" % len(sin_nombre))
        print("=" * 74)
        print("  Cuelgan de una carpeta que genera el proyecto, pero ningún")
        print("  generador crea un asset con ese nombre. No está roto: quien las")
        print("  carga comprueba el null y sigue. Lo que pasa es que esa función")
        print("  visual no se ve nunca, y por carpeta la auditoría las daba por")
        print("  buenas.")
        porcarpeta = {}
        for r in sorted(sin_nombre):
            porcarpeta.setdefault(r.rsplit("/", 1)[0], []).append(r.rsplit("/", 1)[-1])
        for c, ns in sorted(porcarpeta.items()):
            print("\n  %s  (%d)" % (c, len(ns)))
            for n in ns:
                print("       %s" % n)
        print()

    print("  referenciadas %d = en disco %d + las genera el proyecto %d"
          % (len(codigo), len(ok), len(gen)))
    print("                  + pack externo opcional %d + SIN EXPLICACIÓN %d"
          % (len(externas), len(rotas)))
    print()
    if not rotas:
        print("  Ninguna ruta huérfana. Todo lo que pide el código lo produce el")
        print("  proyecto, está en disco, o es un pack externo con fallback.")
        return

    # Agrupamos por la raíz del pack para ver de un vistazo qué falta entero.
    porpack = {}
    for ruta, usos in rotas.items():
        pack = "/".join(ruta.split("/")[:3])
        porpack.setdefault(pack, []).append((ruta, usos))

    for pack in sorted(porpack, key=lambda k: -len(porpack[k])):
        entradas = porpack[pack]
        print("── %s  (%d rutas)" % (pack, len(entradas)))
        for ruta, usos in sorted(entradas)[:6]:
            print("     %-58s %s" % (ruta, usos[0]))
        if len(entradas) > 6:
            print("     ... y %d más" % (len(entradas) - 6))
        print()

    print("Nota: 'sin respaldo' = ni el asset ni su carpeta ni el inventario de")
    print("asset_manifest.json lo conocen. Suele ser un pack no descargado o una")
    print("ruta copiada de otro sistema. Lo segundo es lo que hay que arreglar.")
    resumen_mallafab()


def resumen_mallafab():
    print()
    print("=" * 74)
    print("  MALLAFAB — tipos que se piden y la tabla de claves no mapea")
    print("=" * 74)
    sin_cod = tipos_sin_mapear()
    sin_dat = tipos_de_datos_sin_mapear()
    if not sin_cod:
        print("  código: todos los Resolver() tienen entrada.")
    for t, fs in sorted(sin_cod.items()):
        print("  código: %-24s pedido en %s" % (t, fs[0]))
    if not sin_dat:
        print("  datos:  todos los tipos de street_furniture.json tienen entrada.")
    for t, c in sorted(sin_dat.items(), key=lambda x: -x[1]):
        print("  datos:  %-24s %d piezas caen a primitiva" % (t, c))

    porpack, total, casadas = mallas_sin_usar()
    print()
    print("=" * 74)
    print("  INVENTARIO SIN USAR — mallas bajadas que ninguna clave reconoce")
    print("=" * 74)
    if not total:
        print("  Sin asset_manifest.json: no hay inventario contra el que cruzar.")
        return
    print("  %d mallas en el inventario, %d las reconoce alguna clave (%.0f%%)"
          % (total, casadas, 100.0 * casadas / total))
    if not porpack:
        print("  Todas están mapeadas.")
        return
    print("  %d sin mapear, por pack:\n" % (total - casadas))
    for pack, fs in sorted(porpack.items(), key=lambda k: -len(k[1]))[:15]:
        print("  ── %-28s %4d mallas" % (pack, len(fs)))
        print("       %s%s" % (", ".join(fs[:4]), " ..." if len(fs) > 4 else ""))
    print()
    print("  Un pack entero aquí = está bajado y no se usa. Si te interesa, añade")
    print("  su palabra clave a ClavesPorTipo en AlsasuaMallaFab.cpp y entra solo.")
    print()
    print("  OJO: esto cruza contra asset_manifest.json, que es una foto del")
    print("  inventario. Si has bajado cosas después, regenera el manifiesto en la")
    print("  máquina que tiene el Content completo (python3 Tools/asset_manifest.py)")
    print("  antes de fiarte de este listado.")


if __name__ == "__main__":
    main()
