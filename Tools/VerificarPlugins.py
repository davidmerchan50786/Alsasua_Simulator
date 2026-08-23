"""
VerificarPlugins.py - Que el interruptor de los plugins GF_* siga conectado.

Los 21 plugins GF_* compilan siempre (UBT los pilla por estar en Plugins/), pero
compilar no es vivir: si nadie los activa via GameFeatures, sus subsistemas no se
instancian NUNCA y todo lo que hacen es decorativo. El interruptor es
UAlsasuaCargadorPlugins (Kernel) + la lista PluginsActivables de DefaultGame.ini,
y sus fallos son de los peores: silenciosos en tiempo de compilacion y visibles
solo en el log de arranque que nadie lee.

Tres formas de romperlo que aqui se cantan:

  1. Renombrar/borrar un plugin y dejar su entrada en PluginsActivables. La
     activacion falla con un Warning en el log y el resto sigue: medio pueblo
     sin sistemas y nadie se entera hasta pisar la zona.

  2. Dejar el .uplugin roto (JSON invalido): el plugin deja de montar entero.

  3. Vaciar o renombrar las llamadas del cargador (LoadAndActivateGameFeaturePlugin
     / DeactivateGameFeaturePlugin / UnloadGameFeaturePlugin), o perder la
     dependencia "GameFeatures" del Build.cs de Kernel. Compila igual; simplemente
     deja de hacer su trabajo.

Los plugins GF_* presentes en Plugins/ pero AUSENTES de la lista NO son un error:
dormir uno quitando su linea es la forma prevista de apagarlo. Se listan como
informativo para que el silencio sea una decision, no un descuido.

Uso:  python3 Tools/VerificarPlugins.py      (salida != 0 si encuentra algo)
"""
import json
import os
import re
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

RE_ACTIVABLE = re.compile(r'^\s*\+?PluginsActivables=(\S+)\s*$', re.M)
RE_LLAMADAS = {
    "activacion": re.compile(r'LoadAndActivateGameFeaturePlugin\s*\('),
    "desactivacion": re.compile(r'DeactivateGameFeaturePlugin\s*\('),
    "descarga": re.compile(r'UnloadGameFeaturePlugin\s*\('),
}


def _ini_de_config(raiz):
    ruta = os.path.join(raiz, "Config", "DefaultGame.ini")
    if not os.path.exists(ruta):
        return None, []
    texto = open(ruta, encoding="utf-8", errors="ignore").read()
    return texto, RE_ACTIVABLE.findall(texto)


def activables_inexistentes(raiz):
    """Entradas de PluginsActivables sin directorio/.uplugin que respalde."""
    fallos = []
    _, activables = _ini_de_config(raiz)
    for nombre in sorted(set(activables)):
        uplugin = os.path.join(raiz, "Plugins", nombre, "%s.uplugin" % nombre)
        if not os.path.exists(uplugin):
            fallos.append("PluginsActivables lista '%s' pero no existe %s"
                          % (nombre, os.path.relpath(uplugin, raiz)))
            continue
        try:
            json.loads(open(uplugin, encoding="utf-8-sig", errors="ignore").read())
        except (ValueError, OSError) as e:
            fallos.append("%s no parsea como JSON (%s); el plugin no montaria"
                          % (os.path.relpath(uplugin, raiz), e))
    return fallos


def cargador_roto(raiz):
    """El cargador sin sus tres llamadas, o Kernel sin la dependencia."""
    fallos = []
    cpp = os.path.join(raiz, "Source", "AlsasuaKernel", "Private",
                       "Plugins", "AlsasuaCargadorPlugins.cpp")
    if not os.path.exists(cpp):
        return ["no existe %s: nada activa los plugins GF_*"
                % os.path.relpath(cpp, raiz)]
    cuerpo = open(cpp, encoding="utf-8", errors="ignore").read()
    for nombre, patron in sorted(RE_LLAMADAS.items()):
        if not patron.search(cuerpo):
            fallos.append("AlsasuaCargadorPlugins.cpp perdio la llamada de %s "
                          "(%s): los plugins GF_* se quedarian dormidos"
                          % (nombre, patron.pattern))

    build = os.path.join(raiz, "Source", "AlsasuaKernel", "AlsasuaKernel.Build.cs")
    if os.path.exists(build):
        if not re.search(r'"GameFeatures"', open(build, encoding="utf-8",
                                                 errors="ignore").read()):
            fallos.append('AlsasuaKernel.Build.cs no declara "GameFeatures": '
                          "el cargador no enlazaria")
    return fallos


def plugins_dormidos(raiz):
    """Informativo: GF_* compilados que la lista NO activa. No es fallo."""
    _, activables = _ini_de_config(raiz)
    puestos = set(activables)
    carpeta = os.path.join(raiz, "Plugins")
    if not os.path.isdir(carpeta):
        return []
    return sorted(d for d in os.listdir(carpeta)
                  if d.startswith("GF_") and d not in puestos)


def main():
    fallos = activables_inexistentes(RAIZ) + cargador_roto(RAIZ)
    dormidos = plugins_dormidos(RAIZ)

    print("%d plugins activables, %d GF_* compilados." % (len(set(_ini_de_config(RAIZ)[1])), 21 + len(dormidos)))
    if dormidos:
        print("\nCompilados pero dormidos (sin entrada en PluginsActivables):")
        for d in dormidos:
            print("  %s" % d)
    if not fallos:
        print("\nLa lista, el disco y el cargador estan de acuerdo.")
        return 0
    for f in fallos:
        print("  %s" % f)
    print("\n%d hallazgo(s)." % len(fallos))
    return 1


if __name__ == "__main__":
    sys.exit(main())
