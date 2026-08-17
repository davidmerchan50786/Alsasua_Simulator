"""
AuditarSistemas.py — Qué sistemas de mundo no llama nadie, y con qué chocarían.

AlsasuaManifa/World tiene del orden de 78 clases de sistema. ADirectorArranque
menciona unas 40; el resto no aparece en la cadena de arranque ni lo llama ningún
otro fichero. Parte de eso es normal —componentes que se adjuntan, actores que se
colocan en el nivel, utilidades estáticas— y parte son sistemas escritos, nunca
enchufados y nunca ejecutados.

Lo que este script NO hace es decirte que los enchufes. Enchufar a ciegas ya salió
mal: al arreglar los cargadores rotos resultó que AlsasuaTreePlacer replantaba los
2783 árboles que UCargadorArboles ya siembra, y AlsasuaRoadSurfaceSystem ponía un
cubo sobre cada cinta que UCargadorCalles ya tenía puesta. Un sistema huérfano que
lee el mismo JSON que uno ya enchufado es sospechoso de duplicar trabajo, no de
faltar.

Así que por cada huérfano se dice qué datasets toca y quién más los toca estando
ya en la cadena. Con eso la decisión es informada en vez de adivinada.

Uso:  python3 Tools/AuditarSistemas.py
"""
import os
import re
from collections import defaultdict

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FUENTE = os.path.join(RAIZ, "Source")
DIRECTOR = os.path.join(FUENTE, "AlsasuaWorld", "Private", "DirectorArranque.cpp")
MUNDO_H = os.path.join(FUENTE, "AlsasuaManifa", "Public", "World")

RE_CLASE = re.compile(r'class ALSASUAMANIFA_API ([UA]\w+)\s*:\s*public\s+(\w+)')
RE_DATASET = re.compile(r'Datos/([A-Za-z_0-9]+\.json)')

# Puntos de entrada que llama el MOTOR, sin que nadie los invoque desde el
# proyecto. Un UWorldSubsystem que sobreescribe Initialize corre en todos los
# mundos —editor, PIE, cocción— aunque no aparezca en DirectorArranque; un
# UTickableWorldSubsystem que sobreescribe Tick corre en cada frame.
#
# Este script decía de ellos "existen, se compilan y no se ejecutan", y de los
# seis que listaba eso sólo era cierto de uno. Es peor que un falso positivo:
# invita a enchufar algo que ya está corriendo, y esconde a los que corren MAL.
# UAlsasuaLODManager ticaba a 60 Hz y barría el mundo entero cada 5 s para
# llenar dos listas que siempre salían vacías, y salía aquí como inofensivo.
ENTRADAS_MOTOR = ("Initialize", "Deinitialize", "Tick", "OnWorldBeginPlay",
                  "PostInitialize", "BeginPlay", "Deinitialize")

# Bases que no necesitan que el director las llame para existir: un componente lo
# adjunta su actor, un actor se coloca en el nivel, y un subsistema se crea solo.
# Aun así, si el subsistema sólo actúa cuando alguien invoca su Colocar*/Aplicar*,
# estar creado no es estar haciendo nada — por eso salen igual, marcados.
PASIVAS = ("UActorComponent", "USceneComponent", "UPrimitiveComponent", "AActor",
           "AVolume", "ACharacter", "APawn")


def cuerpo(clase):
    """Ruta del .cpp de una clase, si está donde toca."""
    p = os.path.join(FUENTE, "AlsasuaManifa", "Private", "World", clase[1:] + ".cpp")
    return p if os.path.exists(p) else None


def main():
    with open(DIRECTOR, encoding="utf-8") as fh:
        director = fh.read()

    # Datasets que ya toca alguien enchufado en la cadena.
    clases = {}
    for nombre in sorted(os.listdir(MUNDO_H)):
        if not nombre.endswith(".h"):
            continue
        with open(os.path.join(MUNDO_H, nombre), encoding="utf-8", errors="ignore") as fh:
            texto = fh.read()
        for m in RE_CLASE.finditer(texto):
            clases[m.group(1)] = m.group(2)

    datasets_de = {}
    entradas_de = {}
    for c in clases:
        p = cuerpo(c)
        datasets_de[c] = set()
        entradas_de[c] = []
        if p:
            with open(p, encoding="utf-8", errors="ignore") as fh:
                texto = fh.read()
            datasets_de[c] = set(RE_DATASET.findall(texto))
            for e in ENTRADAS_MOTOR:
                if re.search(r'\b%s::%s\s*\(' % (re.escape(c), e), texto):
                    if e not in entradas_de[c]:
                        entradas_de[c].append(e)

    enchufados = [c for c in clases if c in director]
    huerfanos = [c for c in clases if c not in director]

    # Quién más lee cada dataset, contando también los cargadores de World; y qué
    # otro fichero nombra cada clase, que es lo que distingue un componente que
    # alguien adjunta de uno que no existe en ninguna parte.
    lectores = defaultdict(set)
    nombradores = defaultdict(set)
    for base, _, ficheros in os.walk(FUENTE):
        for nombre in ficheros:
            if not nombre.endswith((".cpp", ".h")):
                continue
            ruta = os.path.join(base, nombre)
            propio = os.path.splitext(nombre)[0]
            with open(ruta, encoding="utf-8", errors="ignore") as fh:
                texto = fh.read()
            if nombre.endswith(".cpp"):
                for d in RE_DATASET.findall(texto):
                    lectores[d].add(propio)
            for c in clases:
                # El .h y el .cpp de la propia clase no cuentan como que alguien
                # la use: lo que se busca es un tercero que la nombre.
                if propio == c[1:]:
                    continue
                if re.search(r'\b%s\b' % re.escape(c), texto):
                    nombradores[c].add(propio)

    print("%d clases en AlsasuaManifa/World: %d en la cadena de arranque, %d fuera.\n"
          % (len(clases), len(enchufados), len(huerfanos)))

    conflicto, limpios, pasivos = [], [], []
    for c in sorted(huerfanos):
        base = clases[c]
        ds = datasets_de[c]
        # ¿Alguno de sus datasets lo lee ya alguien que sí está en la cadena?
        choque = set()
        for d in ds:
            for otro in lectores[d]:
                cand = "U" + otro
                if cand in director or otro in ("CargadorArboles", "CargadorCalles",
                                                "CargadorVias", "CargadorEdificios",
                                                "CargadorPoligonos", "CargadorPOI",
                                                "CargadorPuentes", "AlsasuaVegetationSpawner"):
                    if otro != c[1:]:
                        choque.add("%s (%s)" % (otro, d))
        if choque:
            conflicto.append((c, sorted(choque)))
        elif base in PASIVAS:
            pasivos.append((c, base))
        else:
            limpios.append((c, base, sorted(ds)))

    print("=" * 74)
    print("  RIESGO DE DUPLICADO — leen un dataset que ya trabaja otro")
    print("=" * 74)
    if not conflicto:
        print("  Ninguno.")
    for c, ch in conflicto:
        print("  %s" % c)
        for x in ch:
            print("        ya lo trabaja %s" % x)
    print("\n  Antes de enchufar uno de estos, mira qué construye el otro. Puede que")
    print("  su aportación no sea colocar nada, sino publicar un dato para que lo")
    print("  aplique quien ya tiene la geometría puesta.")

    print("\n" + "=" * 74)
    print("  PASIVOS — componentes, actores y volúmenes")
    print("=" * 74)
    print("  No los llama el director porque no se llaman: se adjuntan o se colocan.")
    print("  Pero eso hay que comprobarlo, no suponerlo: un componente que nadie")
    print("  adjunta está tan muerto como un subsistema que nadie invoca, y aquí")
    print("  se daba por bueno sin mirar. Se marca quién lo nombra.\n")
    for c, b in pasivos:
        quien = sorted(nombradores.get(c, set()))
        if quien:
            print("  %-40s : %-22s lo nombra %s" % (c, b, ", ".join(quien[:3])))
        else:
            print("  %-40s : %-22s NADIE LO ADJUNTA" % (c, b))
    sueltos = [c for c, _ in pasivos if not nombradores.get(c)]
    if sueltos:
        print("\n  %d sin un solo fichero que los nombre. No cuestan nada en tiempo de"
              % len(sueltos))
        print("  ejecución —no existen—, pero tampoco hacen lo que dice su nombre.")

    print("\n  OJO con el punto ciego de esta sección: \"lo nombra X\" quiere decir")
    print("  que hay un NewObject en X, no que ese NewObject llegue a ejecutarse.")
    print("  Las cuatro fases nocturnas del director —estilos de barrio, ventanas")
    print("  emissivas, luz interior y control de farolas— tenían su NewObject en")
    print("  DirectorArranque y adjuntaban CERO componentes: recorrían")
    print("  GetAllActorsOfClass(AStaticMeshActor) buscando los edificios, que son")
    print("  AEdificioGenerado y derivan de AActor. El bucle no daba una vuelta y")
    print("  el log decía que estaban puestos. Un componente \"nombrado\" merece que")
    print("  se mire sobre qué lista itera quien lo nombra.")

    # "Fuera de la cadena" no es "no se ejecuta": el motor arranca solo a todo
    # subsistema que sobreescriba Initialize, y tica solo al que sobreescriba
    # Tick. Separarlos es la diferencia entre "nunca se enchufó" y "lleva
    # corriendo todo este tiempo y nadie lo estaba mirando".
    solos = [(c, b, ds) for c, b, ds in limpios if entradas_de[c]]
    inertes = [(c, b, ds) for c, b, ds in limpios if not entradas_de[c]]

    print("\n" + "=" * 74)
    print("  ARRANCAN SOLOS — el motor los llama, el director no")
    print("=" * 74)
    if not solos:
        print("  Ninguno.")
    for c, b, ds in solos:
        quien = sorted(nombradores.get(c, set()))
        print("  %-40s : %s" % (c, b))
        print("        %-32s corre por %s" % ("", ", ".join(entradas_de[c])))
        if quien:
            print("        %-32s su API la usa %s" % ("", ", ".join(quien[:3])))
        if ds:
            print("        %-32s lee %s" % ("", ", ".join(ds)))
    print("\n  Estos SÍ se ejecutan, en todos los mundos y sin pasar por el director.")
    print("  No hay que enchufarlos: hay que mirar qué hacen al arrancar, porque lo")
    print("  hacen también en el editor, en cada PIE y durante la cocción. Un")
    print("  Initialize que aplica un perfil gráfico, o un Tick que barre el mundo,")
    print("  no es código dormido: es código que corre y que nadie ha revisado.")

    print("\n" + "=" * 74)
    print("  INERTES — sin llamante y sin punto de entrada del motor")
    print("=" * 74)
    if not inertes:
        print("  Ninguno.")
    for c, b, ds in inertes:
        quien = sorted(nombradores.get(c, set()))
        print("  %-40s : %-26s %s" % (c, b, ", ".join(ds) if ds else ""))
        if quien:
            print("        lo nombra %s" % ", ".join(quien[:3]))
    print("\n  Estos existen, se compilan y de verdad no se ejecutan. Unos están")
    print("  apagados a propósito (los semáforos, por perfilado) y otros")
    print("  simplemente nunca se enchufaron.")


if __name__ == "__main__":
    main()
