"""
VerificarDialogos.py — Réplica del cargador de Content/Dialogs/*.json.

Ahí hay tres árboles de diálogo escritos a mano —Alcalde, Guardia, Periodista—
que hasta ahora no leía nadie: UDialogoSubsystem::Iniciar sólo aceptaba un
UConversacionDialogo montado en C++, y el único que alguien monta es el tutorial
de MisionesSubsystem. Contenido autorado que el juego no podía cargar.

Ahora los carga UDialogoSubsystem::CargarConversacion. Este script hace el mismo
recorrido —mismo campo de inicio, mismo mapeo de nodo y opción, misma regla de
encadenado— y saca por cada fichero lo que saldría en el log del juego. Sin
compilador, es la forma de saber si un diálogo nuevo va a entrar entero o se va a
quedar a medias.

Qué mira:
  - que el nodo de inicio exista,
  - que ningún enlace apunte a un nodo que no está,
  - que no haya nodos inalcanzables desde el inicio,
  - cuántas opciones piden tirada de habilidad, que el cargador NO aplica.

Uso:  python3 Tools/VerificarDialogos.py      (salida != 0 si hay algo roto)
"""
import glob
import json
import os
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DIALOGOS = os.path.join(RAIZ, "Content", "Dialogs")


def revisar(ruta):
    """Devuelve (lineas_informe, num_problemas)."""
    nombre = os.path.splitext(os.path.basename(ruta))[0]
    with open(ruta, encoding="utf-8") as fh:
        doc = json.load(fh)

    nodos = doc.get("Nodes", [])
    if not nodos:
        return ["  %-14s SIN NODOS: el cargador lo descarta" % nombre], 1

    ids = {n.get("ID") for n in nodos}
    inicio = doc.get("StartNodeID", 0)
    problemas = []

    if inicio not in ids:
        problemas.append("el nodo de inicio %s no existe" % inicio)

    rotos, con_tirada, auto = 0, 0, 0
    # Mismo encadenado que el cargador: sin opciones y sin ser End_Conversation,
    # se sigue por ID+1.
    salidas = {}
    for n in nodos:
        nid = n.get("ID")
        opciones = n.get("Options", []) or []
        destinos = []
        for o in opciones:
            t = o.get("TargetNodeID", -1)
            if o.get("bRequiresSkillCheck"):
                con_tirada += 1
            if t is not None and t >= 0:
                if t not in ids:
                    rotos += 1
                else:
                    destinos.append(t)
        if not opciones and n.get("Type") != "End_Conversation":
            auto += 1
            if nid + 1 in ids:
                destinos.append(nid + 1)
            else:
                rotos += 1
        salidas[nid] = destinos

    # Alcanzables desde el inicio.
    vistos, pila = set(), [inicio] if inicio in ids else []
    while pila:
        n = pila.pop()
        if n in vistos:
            continue
        vistos.add(n)
        pila.extend(salidas.get(n, []))
    huerfanos = sorted(ids - vistos)

    if rotos:
        problemas.append("%d enlace(s) a nodos que no existen" % rotos)
    if huerfanos:
        problemas.append("nodos inalcanzables desde el inicio: %s"
                         % ", ".join(str(x) for x in huerfanos))

    linea = ("  %-14s inicio=%-3s nodos=%-3d auto=%-2d tiradas=%-2d  %s"
             % (nombre, inicio, len(nodos), auto, con_tirada,
                "ok" if not problemas else "; ".join(problemas)))
    return [linea], len(problemas)


def main():
    ficheros = sorted(glob.glob(os.path.join(DIALOGOS, "*.json")))
    if not ficheros:
        print("No hay ficheros en Content/Dialogs/.")
        return 0

    print("Conversaciones de Content/Dialogs (UDialogoSubsystem::CargarConversacion)\n")
    total = 0
    for f in ficheros:
        lineas, malos = revisar(f)
        for l in lineas:
            print(l)
        total += malos

    print("\n  'tiradas' son opciones con bRequiresSkillCheck en el dato. El")
    print("  cargador NO las aplica: FOpcionDialogo no tiene con qué, y gatear")
    print("  una opción por una tirada es decisión de diseño, no de un cargador.")
    print("  Se cuentan para que el hueco se vea en vez de desaparecer.")

    if total:
        print("\n  %d problema(s)." % total)
        return 1
    print("\n  Los %d diálogos entran enteros." % len(ficheros))
    return 0


if __name__ == "__main__":
    sys.exit(main())
