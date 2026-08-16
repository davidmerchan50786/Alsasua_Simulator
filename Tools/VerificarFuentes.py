"""
VerificarFuentes.py — Errores de sintaxis que en este repo no los caza nadie.

Esto es un proyecto UE de Windows sin CI: no hay forma de compilar desde Linux ni
desde un contenedor, así que un error de sintaxis puede entrar en main y quedarse
ahí hasta que alguien abra Visual Studio. Pasó: dos puntos y coma acabaron dentro
de un comentario en AlsasuaFoliagePainter.cpp

    Grass02.AssetPath = Base + TEXT("/ruta")   // no hay grass_02; era la textura;

y AlsasuaManifa —214 cpp, el módulo gordo— dejó de compilar entero.

Esto no es un compilador ni lo pretende. Son seis comprobaciones baratas que
cazan justo lo que se cuela cuando se edita a ciegas:

  1. Sentencia cuyo punto y coma se lo ha tragado un comentario de línea.
  2. Llaves, paréntesis o corchetes descuadrados en un fichero.
  3. UnityaUnreal con los ejes cambiados (ver abajo).
  4. CVars propias (g.*) que alguien escribe y no registra nadie.
  5. Cabecera con tipo reflejado (UCLASS/USTRUCT/UENUM) y su .generated.h
     ausente o sin ser el último include. UHT lo pide con error, no con aviso:
     sin él no se llega ni al compilador. AlsasuaInputIDs.h llevaba un
     UENUM(BlueprintType) sin el include.
  6. Rutas /Engine/EngineMeshes/, que no existen: las formas básicas del motor
     están en /Engine/BasicShapes/. LoadObject devuelve null y la pieza se queda
     sin malla —invisible, pero ocupando su sitio en el log y en la lista de
     fases—. Lo tenían las antenas, los depósitos y las placas solares de
     AlsasuaRooftopDetailSystem, y las cinco fuentes del pueblo.

Lo tercero es otra que costó cara. UAlsasuaGeoData::UnityaUnreal espera
(este, arriba, norte) y devuelve (este_cm, norte_cm, arriba_cm). Media docena de
sistemas le pasaban (este, norte, 0): la coordenada norte acababa en el eje
vertical y la Y del mundo en cero, así que colocaban todo alineado sobre la
línea norte=0 y flotando a la altura de su propia coordenada norte —Alsasua está
sobre los 8570 en local, o sea más de ochocientos metros en el aire—. Compila
perfecto y no avisa de nada.

Uso:  python3 Tools/VerificarFuentes.py      (salida != 0 si encuentra algo)
"""
import os
import re
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FUENTE = os.path.join(RAIZ, "Source")

# UnityaUnreal(FVector(este, arriba, norte)). Se mira el argumento del medio: si
# no es un cero literal, lo que hay ahí es casi seguro la coordenada norte.
RE_UNITY = re.compile(
    r'UnityaUnreal\s*\(\s*FVector\('
    r'(?P<este>[^,()]*(?:\([^()]*\))?[^,()]*),'
    r'(?P<arriba>[^,()]*(?:\([^()]*\))?[^,()]*),'
    r'(?P<norte>[^()]*(?:\([^()]*\))?[^()]*)\)', re.S)
RE_CERO = re.compile(r'\s*0(\.0*)?[fF]?\s*')


def cegar_literales(linea):
    """Devuelve la línea con el contenido de los literales sustituido por
    espacios, conservando las posiciones. Hace falta para localizar el '//' de
    verdad: la línea del fallo original llevaba TEXT("/multi_stylized_grass/…"),
    y buscar la primera barra a pelo se paraba dentro de la cadena."""
    fuera = list(linea)
    i, n, comilla = 0, len(linea), None
    while i < n:
        c = linea[i]
        if comilla:
            if c == '\\':
                fuera[i] = ' '
                if i + 1 < n:
                    fuera[i + 1] = ' '
                i += 2
                continue
            if c == comilla:
                comilla = None
            else:
                fuera[i] = ' '
        elif c in '"\'':
            comilla = c
        i += 1
    return ''.join(fuera)


def sin_comentarios_ni_cadenas(texto):
    """Quita comentarios y literales para que no descuadren el recuento."""
    fuera = []
    i, n = 0, len(texto)
    while i < n:
        c = texto[i]
        if c == '/' and i + 1 < n and texto[i + 1] == '/':
            while i < n and texto[i] != '\n':
                i += 1
        elif c == '/' and i + 1 < n and texto[i + 1] == '*':
            i += 2
            while i + 1 < n and not (texto[i] == '*' and texto[i + 1] == '/'):
                i += 1
            i += 2
        elif c in '"\'':
            comilla = c
            i += 1
            while i < n and texto[i] != comilla:
                i += 2 if texto[i] == '\\' else 1
            i += 1
        else:
            fuera.append(c)
            i += 1
    return ''.join(fuera)


# CVars del proyecto: se escriben con FindConsoleVariable y se registran con
# TAutoConsoleVariable. Si nadie las registra, FindConsoleVariable devuelve null
# y quien las escribe no hace nada — y no se entera. Las tres del menú de
# opciones (sensibilidad de ratón, invertir Y, vibración de cámara) estuvieron
# así: las barras se movían y no cambiaban nada.
RE_CVAR_USO = re.compile(r'FindConsoleVariable\(\s*TEXT\("(g\.[^"]+)"\)')
RE_CVAR_REG = re.compile(r'TAutoConsoleVariable<[^>]+>\s*\w+\(\s*\n?\s*TEXT\("(g\.[^"]+)"\)')


def cvars_sin_registrar(raiz):
    usadas, registradas = {}, set()
    for base, _, ficheros in os.walk(raiz):
        for nombre in ficheros:
            if not nombre.endswith((".cpp", ".h")):
                continue
            ruta = os.path.join(base, nombre)
            with open(ruta, encoding="utf-8", errors="ignore") as fh:
                texto = fh.read()
            for c in RE_CVAR_USO.findall(texto):
                usadas.setdefault(c, os.path.relpath(ruta, RAIZ))
            registradas |= set(RE_CVAR_REG.findall(texto))
    return [(c, f) for c, f in sorted(usadas.items()) if c not in registradas]


RE_MACRO_REFLEJADA = re.compile(r'^\s*(UCLASS|USTRUCT|UENUM|UINTERFACE)\s*\(')


def generated_mal_puesto(ruta, texto):
    """UHT exige el .generated.h, y como ÚLTIMO include del fichero."""
    lineas = texto.splitlines()
    if not any(RE_MACRO_REFLEJADA.match(l) for l in lineas):
        return None
    incluidos = [(i, l) for i, l in enumerate(lineas) if l.strip().startswith("#include")]
    generados = [(i, l) for i, l in incluidos if ".generated.h" in l]
    if not generados:
        return "tipo reflejado sin #include del .generated.h"
    if incluidos and generados[-1][0] != incluidos[-1][0]:
        return "el .generated.h no es el último include"
    return None


def main():
    fallos = []
    revisados = 0

    for base, _, ficheros in os.walk(FUENTE):
        for nombre in sorted(ficheros):
            if not nombre.endswith((".cpp", ".h")):
                continue
            ruta = os.path.join(base, nombre)
            rel = os.path.relpath(ruta, RAIZ)
            with open(ruta, encoding="utf-8", errors="ignore") as fh:
                texto = fh.read()
            revisados += 1

            # 1. Punto y coma comido por el comentario.
            for num, linea in enumerate(texto.splitlines(), 1):
                cegada = cegar_literales(linea)
                pos = cegada.find('//')
                if pos < 0:
                    continue
                comentario = linea[pos + 2:].rstrip()
                if not comentario.endswith(';'):
                    continue
                codigo = linea[:pos].rstrip()
                if not codigo or codigo.endswith((';', '{', '}', ',', ':', '\\')):
                    continue
                # Una sentencia de verdad lleva asignación o llamada.
                if '=' not in codigo and '(' not in codigo:
                    continue
                fallos.append("%s:%d  falta ';' — se lo ha llevado el comentario\n"
                              "      %s" % (rel, num, linea.strip()))

            # 3. UnityaUnreal con la coordenada norte en el eje vertical.
            lineas = texto.splitlines()
            for m in RE_UNITY.finditer(texto):
                arriba = m.group("arriba").strip()
                if RE_CERO.fullmatch(arriba):
                    continue
                num = texto.count("\n", 0, m.start()) + 1
                # Hay un caso legítimo: los datasets con pts planos [x,y,z,...]
                # ya traen la vertical en medio. Se marca con "// ejes ok" en las
                # dos líneas de arriba, que es lo bastante incómodo como para que
                # nadie lo ponga sin mirarlo.
                if any("ejes ok" in l for l in lineas[max(0, num - 3):num]):
                    continue
                fallos.append("%s:%d  UnityaUnreal con los ejes cambiados: el 2º\n"
                              "      argumento es 'arriba', no 'norte'  →  %s"
                              % (rel, num, " ".join(m.group(0).split())))

            # 6. Rutas /Engine/EngineMeshes/, que no existen.
            for num, linea in enumerate(texto.splitlines(), 1):
                if "/Engine/EngineMeshes/" not in linea:
                    continue
                # Vale mencionarla en un comentario para explicar por qué se
                # cambió; lo que no vale es cargarla.
                if cegar_literales(linea).lstrip().startswith(("//", "*")):
                    continue
                fallos.append("%s:%d  /Engine/EngineMeshes/ no existe: las formas\n"
                              "      básicas están en /Engine/BasicShapes/. LoadObject\n"
                              "      devuelve null y la pieza se queda sin malla, invisible\n"
                              "      →  %s" % (rel, num, linea.strip()))

            # 5. .generated.h de las cabeceras reflejadas.
            if nombre.endswith(".h"):
                problema = generated_mal_puesto(ruta, texto)
                if problema:
                    fallos.append("%s  %s" % (rel, problema))

            # 2. Delimitadores descuadrados.
            limpio = sin_comentarios_ni_cadenas(texto)
            for abre, cierra, que in (('{', '}', 'llaves'),
                                      ('(', ')', 'paréntesis'),
                                      ('[', ']', 'corchetes')):
                d = limpio.count(abre) - limpio.count(cierra)
                if d != 0:
                    fallos.append("%s  %s descuadrados: %+d" % (rel, que, d))

    for cvar, donde in cvars_sin_registrar(FUENTE):
        fallos.append("%s  escribe la CVar %s y no la registra nadie:\n"
                      "      FindConsoleVariable devuelve null y ese ajuste no hace nada"
                      % (donde, cvar))

    print("%d ficheros .cpp/.h revisados.\n" % revisados)
    if not fallos:
        print("Sin hallazgos. No garantiza que compile — sólo que no tiene")
        print("estos seis fallos, que son los que se cuelan al editar a ciegas.")
        return 0

    for f in fallos:
        print("  " + f)
    print("\n%d hallazgo(s)." % len(fallos))
    return 1


if __name__ == "__main__":
    sys.exit(main())
