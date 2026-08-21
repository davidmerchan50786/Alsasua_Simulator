"""
VerificarFuentes.py — Errores de sintaxis que en este repo no los caza nadie.

Esto es un proyecto UE de Windows sin CI: no hay forma de compilar desde Linux ni
desde un contenedor, así que un error de sintaxis puede entrar en main y quedarse
ahí hasta que alguien abra Visual Studio. Pasó: dos puntos y coma acabaron dentro
de un comentario en AlsasuaFoliagePainter.cpp

    Grass02.AssetPath = Base + TEXT("/ruta")   // no hay grass_02; era la textura;

y AlsasuaManifa —214 cpp, el módulo gordo— dejó de compilar entero.

Esto no es un compilador ni lo pretende. Son nueve comprobaciones baratas que
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
  7. Conversor de coordenadas usado EN LÍNEA como posición de mundo. Los tres
     —AbsLocalToUE5, RelLocalToUE5, UnityaUnreal— dejan la Z en el segundo
     componente de la entrada, que con el patrón habitual (X, 0, Z) es cero:
     cota cero del mundo, 531 m por debajo del pueblo. Metido directamente en
     un SpawnActor o un AddInstance no queda sitio donde apoyarlo en el
     terreno. Hay que pasarlo por una variable y ponerle la Z con
     AlturaSueloUE5, o usar RelLocalASueloUE5.
  8. #include de una cabecera del propio proyecto que ya no existe. Es un error
     de compilación de los que no se ven al leer el diff: al retirar
     AlsasuaLODManager quedó su include en DirectorArranque.cpp, en otro módulo
     y a 50 líneas de distancia de nada que lo mencionara. Se miran sólo los
     includes que resuelven contra Source/, no los del motor, y sólo cabeceras
     que empiecen por Alsasua: las carpetas del proyecto se llaman igual que las
     del motor (Components/, World/, Core/), así que por carpeta no se pueden
     distinguir.
  9. Parámetro de MPC que alguien escribe y que el creador no declara. Poner un
     escalar que no está en la colección sale por un warning y se queda ahí, así
     que no se nota: dieciséis parámetros —charcos, ventanas de noche, viento,
     grano de película, el pintado de humedad del RVT— se escribían contra
     MPC_AlsasuaGlobal, una colección que no crea nadie. Se contrastan los dos
     creadores, AsegurarMPCClima() de C++ y ESCALARES_MPC de SetupMaterials.py,
     que además tienen que decir lo mismo.

Lo que esto NO caza, y conviene saberlo: la comprobación 3 sólo salta cuando el
segundo argumento de UnityaUnreal NO es cero. El caso contrario —cero literal,
ejes bien, pero la Z de salida sin apoyar después— es legítimo a medias y no se
distingue por sintaxis; la 7 cubre sólo la variante en línea. A CargadorPOI se le
coló por ahí: UnityaUnreal(x, 0.0, z) con los ejes correctos y la Z a cero para
siempre.

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

# Conversor de coordenadas metido directamente donde se espera una posición de
# mundo. Ahí ya no hay dónde apoyar la cota: el valor entra tal cual, con la Z
# que le tocara. Se busca el uso y el conversor en la misma llamada.
RE_CONV_EN_LINEA = re.compile(
    r'(?P<uso>SpawnActor\s*<[^>]*>|AddInstance|SetWorldLocation|SetActorLocation|'
    r'SetRelativeLocation)\s*\([^;]{0,400}?'
    r'(?P<conv>AbsLocalToUE5|RelLocalToUE5|UnityaUnreal)\s*\(', re.S)


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


RE_INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.M)


def cabeceras_del_proyecto(raiz):
    """
    {ruta relativa tal como se escribe en un #include} -> True.

    UBT resuelve los includes contra los Public/ y Private/ de cada módulo, así
    que "World/AlsasuaMuros.h" y "AlsasuaMuros.h" son las dos formas válidas de
    la misma cabecera. Se indexan las dos, y también los sufijos intermedios.
    """
    validas = set()
    for base, _, ficheros in os.walk(raiz):
        for nombre in ficheros:
            if not nombre.endswith(".h"):
                continue
            rel = os.path.relpath(os.path.join(base, nombre), raiz).replace(os.sep, "/")
            partes = rel.split("/")
            # Desde el primer Public/ o Private/ hacia abajo es lo que ve UBT.
            for i, p in enumerate(partes):
                if p in ("Public", "Private"):
                    for j in range(i + 1, len(partes)):
                        validas.add("/".join(partes[j:]))
                    break
            else:
                validas.add(nombre)
            validas.add(nombre)
    return validas


def includes_rotos(rel, texto, validas, generadas):
    """Includes con comillas que no resuelven a ninguna cabecera de Source/.

    Sólo se acusa una cabecera cuyo nombre empieza por **Alsasua**, y ése es
    todo el criterio. El comentario decía además "o que cuelga de una carpeta
    que existe en Source/", y eso no se puede implementar aquí: las carpetas del
    proyecto se llaman Components/, World/, Core/, Materials/… igual que las del
    motor, así que la regla marcaba 227 includes perfectamente válidos
    —Components/StaticMeshComponent.h el primero—. Con el prefijo no hay
    ambigüedad, porque ninguna cabecera del motor se llama Alsasua*.

    Lo que esto NO caza, por tanto: un include roto a una cabecera del proyecto
    que no siga la convención de nombres. En este repo son contadas.
    """
    salida = []
    for m in RE_INCLUDE.finditer(texto):
        inc = m.group(1)
        if inc in validas or inc in generadas:
            continue
        hoja = inc.split("/")[-1]
        if hoja in validas:
            continue
        if hoja.endswith(".generated.h"):
            continue
        if not hoja.startswith("Alsasua"):
            continue
        num = texto.count("\n", 0, m.start()) + 1
        salida.append("%s:%d  #include \"%s\" y esa cabecera no está en Source/.\n"
                      "      Si el fichero se ha retirado, el include se retira con él"
                      % (rel, num, inc))
    return salida


# Sólo lo que se escribe en una INSTANCIA DE MPC. UMaterialInstanceDynamic tiene
# SetScalarParameterValue con la misma firma, y ahí un nombre nuevo es un
# parámetro del material, no de la colección: mirando la llamada a secas salían
# 37 hallazgos y 33 eran BaseColor, EmissiveColor y compañía, o sea MIDs.
RE_MPC_INST = re.compile(r'UMaterialParameterCollectionInstance\s*\*\s*(\w+)')
RE_MPC_ESCRIBE_TPL = r'\b%s->Set(Scalar|Vector)ParameterValue\(\s*FName\("([^"]+)"\)'
RE_MPC_CREA_CPP = re.compile(r'\b(Escalar|Vector)\(TEXT\("([^"]+)"\)')
RE_MPC_CREA_PY_ESC = re.compile(r'\(\s*"([^"]+)"\s*,\s*[-\d.]+\s*\)')

CREADOR_MPC = os.path.join(FUENTE, "AlsasuaEditor", "Private", "CreadorMaterialEdificio.cpp")
CREADOR_MPC_PY = os.path.join(RAIZ, "Tools", "SetupMaterials.py")


def parametros_mpc(raiz):
    """
    Parámetros que alguien ESCRIBE en un MPC y que ningún creador declara.

    Poner un escalar que no existe en la colección no falla: sale un warning y
    ahí se queda. Así estuvieron dieciséis escribiendo contra
    MPC_AlsasuaGlobal, que no crea nadie.
    """
    fallos = []
    try:
        with open(CREADOR_MPC, encoding="utf-8", errors="ignore") as fh:
            cpp = fh.read()
    except OSError:
        return fallos

    declarados = {m.group(2) for m in RE_MPC_CREA_CPP.finditer(cpp)}
    if not declarados:
        return fallos   # el creador cambió de forma; mejor callar que mentir

    # Los dos creadores tienen que decir lo mismo.
    try:
        with open(CREADOR_MPC_PY, encoding="utf-8", errors="ignore") as fh:
            py = fh.read()
        ini = py.find("ESCALARES_MPC = [")
        if ini >= 0:
            bloque = py[ini:py.index("]", ini)]
            en_py = {m.group(1) for m in RE_MPC_CREA_PY_ESC.finditer(bloque)}
            en_py |= set(re.findall(r'"([^"]+)"', py[py.find("VECTORES_MPC = ["):
                                                     py.find("]", py.find("VECTORES_MPC = ["))]
                                    if "VECTORES_MPC = [" in py else ""))
            for falta in sorted(declarados - en_py):
                fallos.append("Tools/SetupMaterials.py  no declara el parámetro de MPC '%s',\n"
                              "      que sí declara AsegurarMPCClima() en C++. Los dos creadores\n"
                              "      tienen que decir lo mismo" % falta)
    except (OSError, ValueError):
        pass

    for base, _, ficheros in os.walk(raiz):
        for nombre in sorted(ficheros):
            if not nombre.endswith(".cpp"):
                continue
            ruta = os.path.join(base, nombre)
            with open(ruta, encoding="utf-8", errors="ignore") as fh:
                texto = fh.read()
            for var in set(RE_MPC_INST.findall(texto)):
                for m in re.finditer(RE_MPC_ESCRIBE_TPL % re.escape(var), texto):
                    p = m.group(2)
                    if p in declarados:
                        continue
                    num = texto.count("\n", 0, m.start()) + 1
                    fallos.append("%s:%d  escribe el parámetro de MPC '%s' y ningún creador lo\n"
                                  "      declara: SetScalar/VectorParameterValue de un nombre que no\n"
                                  "      está sale por un warning y no hace nada"
                                  % (os.path.relpath(ruta, RAIZ), num, p))
    return fallos


def main():
    fallos = []
    revisados = 0
    validas = cabeceras_del_proyecto(FUENTE)
    # Las .generated.h las escribe UHT en Intermediate/, no están en Source/.
    generadas = {h.replace(".h", ".generated.h") for h in validas}

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

            # 7. Conversor en línea como posición de mundo.
            for m in RE_CONV_EN_LINEA.finditer(sin_comentarios_ni_cadenas(texto)):
                num = texto.count("\n", 0, m.start()) + 1
                fallos.append("%s:%d  %s() metido en %s(): la Z sale del segundo\n"
                              "      componente de la entrada, que con (X, 0, Z) es cero —cota\n"
                              "      cero del mundo, 531 m bajo el pueblo—. Guárdalo en una\n"
                              "      variable y ponle AlturaSueloUE5, o usa RelLocalASueloUE5"
                              % (rel, num, m.group("conv"), m.group("uso")))

            # 8. Includes que apuntan a una cabecera del proyecto que ya no está.
            fallos.extend(includes_rotos(rel, texto, validas, generadas))

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

    fallos.extend(parametros_mpc(FUENTE))

    for cvar, donde in cvars_sin_registrar(FUENTE):
        fallos.append("%s  escribe la CVar %s y no la registra nadie:\n"
                      "      FindConsoleVariable devuelve null y ese ajuste no hace nada"
                      % (donde, cvar))

    print("%d ficheros .cpp/.h revisados.\n" % revisados)
    if not fallos:
        print("Sin hallazgos. No garantiza que compile — sólo que no tiene")
        print("estos nueve fallos, que son los que se cuelan al editar a ciegas.")
        return 0

    for f in fallos:
        print("  " + f)
    print("\n%d hallazgo(s)." % len(fallos))
    return 1


if __name__ == "__main__":
    sys.exit(main())
